#include "graphics/rhi/vulkan/vk_buffer.h"

#include "graphics/rhi/vulkan/vk_command_buffer.h"
#include "graphics/rhi/vulkan/vk_command_pool.h"
#include "graphics/rhi/vulkan/vk_device.h"
#include <cstring>

namespace golias {

    namespace {

        VkBufferUsageFlags GetUsageFlags(BufferType type) {
            switch (type) {
            case BufferType::Vertex:
                return VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            case BufferType::Index:
                return VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            case BufferType::Uniform:
                return VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            case BufferType::Storage:
                return VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
            }

            LOG_FATAL("Unknown buffer type!");
            return 0;
        }

        VkMemoryPropertyFlags GetMemoryProperties(BufferUsage usage) {
            switch (usage) {
            case BufferUsage::Static:
                return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
            case BufferUsage::Dynamic:
            case BufferUsage::Stream:
                return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            }

            LOG_FATAL("Unknown buffer usage!");
            return 0;
        }

    } // namespace


    size_t VulkanBuffer::GetSize() const {
        return mSize;
    }

    BufferType VulkanBuffer::GetType() const {
        return mType;
    }

    VkBuffer VulkanBuffer::GetHandle() const {
        return mBuffer;
    }

    VkDeviceMemory VulkanBuffer::GetMemory() const {
        return mMemory;
    }


    VulkanBuffer::VulkanBuffer(Ref<VulkanDevice> device, const BufferDesc& desc)
        : mDevice(device), mType(desc.type), mUsage(desc.usage), mSize(desc.size), mMemoryProperties(GetMemoryProperties(mUsage)) {

        VkBufferCreateInfo bufferInfo = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .pNext       = nullptr,
            .size        = mSize,
            .usage       = GetUsageFlags(mType),
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VK_CHECK_RESULT(vkCreateBuffer(mDevice->GetHandle(), &bufferInfo, nullptr, &mBuffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(mDevice->GetHandle(), mBuffer, &memRequirements);

        uint32_t memoryTypeIndex = mDevice->FindMemoryType(memRequirements.memoryTypeBits, mMemoryProperties);

        VkMemoryAllocateInfo allocInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };

        VK_CHECK_RESULT(vkAllocateMemory(mDevice->GetHandle(), &allocInfo, nullptr, &mMemory));
        VK_CHECK_RESULT(vkBindBufferMemory(mDevice->GetHandle(), mBuffer, mMemory, 0));

        LOG_INFO("Created buffer: type={} usage={} size={} memory={:#x}",
                 static_cast<int>(mType),
                 static_cast<int>(mUsage),
                 mSize,
                 reinterpret_cast<uint64_t>(mMemory));

        if (desc.data != nullptr) {
            SetData(desc.data, desc.size, 0);
        }
    }

    VulkanBuffer::~VulkanBuffer() {
        if (mMemory != VK_NULL_HANDLE) {
            vkFreeMemory(mDevice->GetHandle(), mMemory, nullptr);
            mMemory = VK_NULL_HANDLE;
        }

        if (mBuffer != VK_NULL_HANDLE) {
            vkDestroyBuffer(mDevice->GetHandle(), mBuffer, nullptr);
            mBuffer = VK_NULL_HANDLE;
        }
    }

    void VulkanBuffer::SetData(const void* data, size_t size, size_t offset) {
        if (data == nullptr || size == 0) {
            return;
        }

        if (offset + size > mSize) {
            LOG_ERROR("Buffer write out of bounds! offset={} size={} bufferSize={}", offset, size, mSize);
            return;
        }

        if (mMemoryProperties & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT) {
            void* mapped = nullptr;
            VK_CHECK_RESULT(vkMapMemory(mDevice->GetHandle(), mMemory, offset, size, 0, &mapped));
            std::memcpy(mapped, data, size);
            vkUnmapMemory(mDevice->GetHandle(), mMemory);
            return;
        }

        VkBufferCreateInfo stagingInfo = {
            .sType       = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .size        = size,
            .usage       = VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
        };

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VK_CHECK_RESULT(vkCreateBuffer(mDevice->GetHandle(), &stagingInfo, nullptr, &stagingBuffer));

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(mDevice->GetHandle(), stagingBuffer, &memRequirements);

        uint32_t memoryTypeIndex = mDevice->FindMemoryType(memRequirements.memoryTypeBits,
                                                           VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        VkMemoryAllocateInfo allocInfo = {
            .sType           = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
            .allocationSize  = memRequirements.size,
            .memoryTypeIndex = memoryTypeIndex,
        };

        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;
        VK_CHECK_RESULT(vkAllocateMemory(mDevice->GetHandle(), &allocInfo, nullptr, &stagingMemory));
        VK_CHECK_RESULT(vkBindBufferMemory(mDevice->GetHandle(), stagingBuffer, stagingMemory, 0));

        void* mapped = nullptr;
        VK_CHECK_RESULT(vkMapMemory(mDevice->GetHandle(), stagingMemory, 0, size, 0, &mapped));
        std::memcpy(mapped, data, size);
        vkUnmapMemory(mDevice->GetHandle(), stagingMemory);

        auto commandPool   = std::make_shared<VulkanCommandPool>(mDevice);
        auto commandBuffer = std::make_shared<VulkanCommandBuffer>(mDevice, commandPool);

        commandBuffer->Begin(VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
        commandBuffer->CopyBuffer(stagingBuffer, mBuffer, size, 0, offset);
        commandBuffer->End();

        commandBuffer->Submit(mDevice->GetGraphicsQueue());
        commandBuffer->Wait(mDevice->GetGraphicsQueue());

        vkFreeMemory(mDevice->GetHandle(), stagingMemory, nullptr);
        vkDestroyBuffer(mDevice->GetHandle(), stagingBuffer, nullptr);
    }

} // namespace golias
