#pragma once
#include "vk_common.h"
#include "graphics/rhi/rhi_uniform_buffer.h"

namespace golias {

    class VulkanDevice;
    class VulkanBuffer;

    // One shader uniform buffer (UBO), e.g. "Camera" at binding 0.
    class VulkanUniformBuffer : public UniformBuffer {
    public:
        VulkanUniformBuffer(Ref<VulkanDevice> device, const UniformBufferDesc& desc, uint32_t frameCount);
        ~VulkanUniformBuffer() override = default;

        void SetData(uint32_t frameIndex, const void* data, size_t size, size_t offset = 0);

        const std::string& GetName() const override;

        uint32_t GetBinding() const override;

        size_t GetSize() const override;

        ShaderStage GetStage() const override;

        Ref<VulkanBuffer> GetBuffer(uint32_t frameIndex) const;

    private:
        Ref<VulkanDevice> mDevice = nullptr;

        UniformBufferDesc mDesc = {};
        uint32_t mFrameCount    = 1;

        std::vector<Ref<VulkanBuffer>> mBuffers = {};
    };

    // A set of UBOs grouped into a single descriptor set (Vulkan) / binding
    // group (other backends). Internally owns the descriptor set layout, pool
    // and per-frame descriptor sets so callers never interact with those.
    class VulkanUniformBufferSet : public UniformBufferSet {
    public:
        VulkanUniformBufferSet(Ref<VulkanDevice> device, uint32_t frameCount);
        ~VulkanUniformBufferSet() override;

        VulkanUniformBufferSet(const VulkanUniformBufferSet&)            = delete;
        VulkanUniformBufferSet& operator=(const VulkanUniformBufferSet&) = delete;

        Ref<VulkanUniformBuffer> AddUniformBuffer(const UniformBufferDesc& desc);

        void Build();

        Ref<UniformBuffer> Get(const std::string& name) const override;

        void SetData(const std::string& name, uint32_t frameIndex, const void* data, size_t size, size_t offset = 0) override;

        uint32_t GetFrameCount() const;

        VkDescriptorSetLayout GetDescriptorSetLayout() const;

        VkDescriptorSet GetDescriptorSet(uint32_t frameIndex) const;

    private:
        void Destroy();

    private:
        Ref<VulkanDevice> mDevice = nullptr;

        uint32_t mFrameCount = 1;

        std::vector<Ref<VulkanUniformBuffer>> mBuffers    = {};
        std::unordered_map<std::string, Ref<VulkanUniformBuffer>> mBuffersByName = {};

        VkDescriptorSetLayout mDescriptorSetLayout = VK_NULL_HANDLE;
        VkDescriptorPool mDescriptorPool           = VK_NULL_HANDLE;
        std::vector<VkDescriptorSet> mDescriptorSets = {};
    };

} // namespace golias
