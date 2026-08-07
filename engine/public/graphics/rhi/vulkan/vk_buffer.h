#pragma once
#include "graphics/rhi/rhi_buffer.h"
#include "vk_common.h"

namespace golias {
    class VulkanDevice;


    class VulkanBuffer : public Buffer {
    public:
        VulkanBuffer(Ref<VulkanDevice> device, const BufferDesc& desc);

        ~VulkanBuffer() override;

        void SetData(const void* data, size_t size, size_t offset = 0) override;

        size_t GetSize() const override;
        
        BufferType GetType() const override;

        VkBuffer GetHandle() const;

        VkDeviceMemory GetMemory() const;

    private:
        Ref<VulkanDevice> mDevice;

        VkBuffer mBuffer       = VK_NULL_HANDLE;
        VkDeviceMemory mMemory = VK_NULL_HANDLE;

        BufferType mType;
        BufferUsage mUsage;
        size_t mSize = 0;

        VkMemoryPropertyFlags mMemoryProperties = 0;
    };


} // namespace golias
