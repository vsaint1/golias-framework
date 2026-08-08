#pragma once
#include "vk_common.h"
#include "graphics/rhi/rhi_graphics_pipeline.h"

namespace golias {

    class VulkanDevice;
    class VulkanRenderPass;

    class VulkanGraphicsPipeline : public GraphicsPipeline {
    public:
        VulkanGraphicsPipeline(Ref<VulkanDevice> device, Ref<VulkanRenderPass> renderPass);
        ~VulkanGraphicsPipeline() override;

        VulkanGraphicsPipeline(const VulkanGraphicsPipeline&)            = delete;
        VulkanGraphicsPipeline& operator=(const VulkanGraphicsPipeline&) = delete;

        void Build(const GraphicsPipelineStateDesc& desc);

        VkPipeline GetHandle() const;

        VkPipelineLayout GetLayout() const;

        const Ref<UniformBufferSet>& GetUniformBufferSet() const;

    private:
        Ref<VulkanDevice> mDevice = nullptr;

        VkPipeline mPipeline             = VK_NULL_HANDLE;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;

        Ref<VulkanRenderPass> mRenderPass = nullptr;

        Ref<UniformBufferSet> mUniformBufferSet = nullptr;
    };

} // namespace golias
