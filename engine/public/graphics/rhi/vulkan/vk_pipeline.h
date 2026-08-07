#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanDevice;
    class VulkanRenderPass;
    class VulkanShader;


    struct VulkanGraphicsPipelineDesc {
        std::vector<Ref<VulkanShader>> Shaders;

        VkPipelineVertexInputStateCreateInfo VertexInput     = {};
        VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};

        VkViewport Viewport = {};
        VkRect2D Scissor    = {};

        VkPipelineRasterizationStateCreateInfo Rasterizer = {};

        VkPipelineMultisampleStateCreateInfo Multisampling = {};

        VkPipelineColorBlendAttachmentState ColorBlendAttachment = {};

        VkPipelineDepthStencilStateCreateInfo DepthStencil = {};

        VkPipelineLayoutCreateInfo Layout = {};
    };


    class VulkanPipeline {
    public:
        VulkanPipeline(Ref<VulkanDevice> device, Ref<VulkanRenderPass> renderPass);
        ~VulkanPipeline();

        VkPipeline GetHandle() const;

        VkPipelineLayout GetLayout() const;
        
        void CreateGraphicsPipeline(const VulkanGraphicsPipelineDesc& desc);

        const std::vector<Ref<VulkanShader>>& GetShaders() const;
    private:
        Ref<VulkanDevice> mDevice = nullptr;

        VkPipeline mPipeline             = VK_NULL_HANDLE;
        VkPipelineLayout mPipelineLayout = VK_NULL_HANDLE;

        Ref<VulkanRenderPass> mRenderPass = nullptr;

        std::vector<Ref<VulkanShader>> mShaders;
    };


} // namespace golias
