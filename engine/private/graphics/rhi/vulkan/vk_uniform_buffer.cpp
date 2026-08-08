#include "graphics/rhi/vulkan/vk_uniform_buffer.h"

#include "graphics/rhi/vulkan/vk_buffer.h"
#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_shader.h"

namespace golias {


    VulkanUniformBuffer::VulkanUniformBuffer(Ref<VulkanDevice> device, const UniformBufferDesc& desc, uint32_t frameCount)
        : mDevice(device), mDesc(desc), mFrameCount(frameCount) {

        mBuffers.reserve(mFrameCount);

        for (uint32_t i = 0; i < mFrameCount; ++i) {
            BufferDesc bufferDesc;
            bufferDesc.type  = BufferType::Uniform;
            bufferDesc.usage = BufferUsage::Dynamic;
            bufferDesc.size  = mDesc.Size;

            mBuffers.push_back(std::make_shared<VulkanBuffer>(mDevice, bufferDesc));
        }
    }

    void VulkanUniformBuffer::SetData(uint32_t frameIndex, const void* data, size_t size, size_t offset) {
        GOLIAS_ASSERT(frameIndex < mFrameCount);
        mBuffers[frameIndex]->SetData(data, size, offset);
    }

    const std::string& VulkanUniformBuffer::GetName() const {
        return mDesc.Name;
    }

    uint32_t VulkanUniformBuffer::GetBinding() const {
        return mDesc.Binding;
    }

    size_t VulkanUniformBuffer::GetSize() const {
        return mDesc.Size;
    }

    ShaderStage VulkanUniformBuffer::GetStage() const {
        return mDesc.Stage;
    }

    Ref<VulkanBuffer> VulkanUniformBuffer::GetBuffer(uint32_t frameIndex) const {
        GOLIAS_ASSERT(frameIndex < mFrameCount);
        return mBuffers[frameIndex];
    }

    VulkanUniformBufferSet::VulkanUniformBufferSet(Ref<VulkanDevice> device, uint32_t frameCount)
        : mDevice(device), mFrameCount(frameCount) {
    }

    VulkanUniformBufferSet::~VulkanUniformBufferSet() {
        Destroy();
    }

    Ref<VulkanUniformBuffer> VulkanUniformBufferSet::AddUniformBuffer(const UniformBufferDesc& desc) {
        GOLIAS_ASSERT_MSG(mDescriptorSetLayout == VK_NULL_HANDLE, "Cannot add uniform buffers after Build()");

        auto uniformBuffer = std::make_shared<VulkanUniformBuffer>(mDevice, desc, mFrameCount);

        mBuffers.push_back(uniformBuffer);
        mBuffersByName[desc.Name] = uniformBuffer;

        return uniformBuffer;
    }

    Ref<UniformBuffer> VulkanUniformBufferSet::Get(const std::string& name) const {
        auto it = mBuffersByName.find(name);
        if (it == mBuffersByName.end()) {
            LOG_ERROR("Uniform buffer '{}' not found.", name);
            return nullptr;
        }

        return it->second;
    }

    void VulkanUniformBufferSet::SetData(const std::string& name, uint32_t frameIndex, const void* data, size_t size, size_t offset) {
        auto it = mBuffersByName.find(name);
        if (it == mBuffersByName.end()) {
            LOG_ERROR("Uniform buffer '{}' not found.", name);
            return;
        }

        it->second->SetData(frameIndex, data, size, offset);
    }

    uint32_t VulkanUniformBufferSet::GetFrameCount() const {
        return mFrameCount;
    }

    VkDescriptorSetLayout VulkanUniformBufferSet::GetDescriptorSetLayout() const {
        return mDescriptorSetLayout;
    }

    VkDescriptorSet VulkanUniformBufferSet::GetDescriptorSet(uint32_t frameIndex) const {
        GOLIAS_ASSERT(frameIndex < mDescriptorSets.size());
        return mDescriptorSets[frameIndex];
    }

    void VulkanUniformBufferSet::Destroy() {
        if (mDescriptorPool != VK_NULL_HANDLE) {
            vkDestroyDescriptorPool(mDevice->GetHandle(), mDescriptorPool, nullptr);
            mDescriptorPool = VK_NULL_HANDLE;
        }

        if (mDescriptorSetLayout != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(mDevice->GetHandle(), mDescriptorSetLayout, nullptr);
            mDescriptorSetLayout = VK_NULL_HANDLE;
        }

        mDescriptorSets.clear();
    }

    void VulkanUniformBufferSet::Build() {
        Destroy();

        // Descriptor set layout (one binding per UBO)
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings;
        layoutBindings.reserve(mBuffers.size());

        for (const auto& uniformBuffer : mBuffers) {
            VkDescriptorSetLayoutBinding layoutBinding = {};
            layoutBinding.binding                      = uniformBuffer->GetBinding();
            layoutBinding.descriptorType               = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBinding.descriptorCount              = 1;
            layoutBinding.stageFlags                   = ConvertShaderStage(uniformBuffer->GetStage());
            layoutBinding.pImmutableSamplers           = nullptr;

            layoutBindings.push_back(layoutBinding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {.sType        = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
                                                      .bindingCount = static_cast<uint32_t>(layoutBindings.size()),
                                                      .pBindings    = layoutBindings.data()};

        if (vkCreateDescriptorSetLayout(mDevice->GetHandle(), &layoutInfo, nullptr, &mDescriptorSetLayout) != VK_SUCCESS) {
            LOG_FATAL("Failed to create descriptor set layout for uniform buffer set!");
        }

        VkDescriptorPoolSize poolSize = {
            .type            = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            .descriptorCount = static_cast<uint32_t>(mBuffers.size()) * mFrameCount,
        };

        VkDescriptorPoolCreateInfo poolInfo = {
            .sType         = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
            .maxSets       = mFrameCount,
            .poolSizeCount = 1,
            .pPoolSizes    = &poolSize,
        };

        if (vkCreateDescriptorPool(mDevice->GetHandle(), &poolInfo, nullptr, &mDescriptorPool) != VK_SUCCESS) {
            LOG_FATAL("Failed to create descriptor pool for uniform buffer set!");
        }

        // Allocate descriptor sets (one per frame)
        std::vector<VkDescriptorSetLayout> layouts(mFrameCount, mDescriptorSetLayout);

        VkDescriptorSetAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool     = mDescriptorPool,
            .descriptorSetCount = mFrameCount,
            .pSetLayouts        = layouts.data(),
        };

        mDescriptorSets.resize(mFrameCount);
        if (vkAllocateDescriptorSets(mDevice->GetHandle(), &allocInfo, mDescriptorSets.data()) != VK_SUCCESS) {
            LOG_FATAL("Failed to allocate descriptor sets for uniform buffer set!");
        }

        // Bind each descriptor set to its frame's buffers 
        for (uint32_t frame = 0; frame < mFrameCount; ++frame) {
            std::vector<VkDescriptorBufferInfo> bufferInfos;
            bufferInfos.reserve(mBuffers.size());

            for (const auto& uniformBuffer : mBuffers) {
                VkDescriptorBufferInfo bufferInfo = {
                    .buffer = uniformBuffer->GetBuffer(frame)->GetHandle(),
                    .offset = 0,
                    .range  = uniformBuffer->GetSize(),
                };

                bufferInfos.push_back(bufferInfo);
            }

            std::vector<VkWriteDescriptorSet> writes;
            writes.reserve(mBuffers.size());

            for (size_t i = 0; i < mBuffers.size(); ++i) {
                VkWriteDescriptorSet write = {
                    .sType           = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                    .dstSet          = mDescriptorSets[frame],
                    .dstBinding      = mBuffers[i]->GetBinding(),
                    .dstArrayElement = 0,
                    .descriptorCount = 1,
                    .descriptorType  = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
                    .pBufferInfo     = &bufferInfos[i],
                };

                writes.push_back(write);
            }

            vkUpdateDescriptorSets(mDevice->GetHandle(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
        }

        LOG_INFO("Built uniform buffer set with {} UBO(s) and {} frame(s).", mBuffers.size(), mFrameCount);
    }

} // namespace golias
