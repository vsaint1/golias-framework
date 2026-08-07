#include "graphics/rhi/vulkan/vk_pipeline.h"

#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_renderpass.h"
#include "graphics/rhi/vulkan/vk_shader.h"

namespace golias {

    VulkanPipeline::VulkanPipeline(Ref<VulkanDevice> device, Ref<VulkanRenderPass> renderPass) : mDevice(device), mRenderPass(renderPass) {
    }

    void VulkanPipeline::CreateGraphicsPipeline(const VulkanGraphicsPipelineDesc& desc) {
        mShaders = desc.Shaders; // Store the shaders in the member variable

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = desc.Layout;
        if (vkCreatePipelineLayout(mDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS) {
            LOG_FATAL("Failed to create pipeline layout!");
        }

        VkPipelineColorBlendStateCreateInfo colorBlendState = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .pNext           = nullptr,
            .flags           = 0,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &desc.ColorBlendAttachment,
            .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f}
        };

        VkPipelineViewportStateCreateInfo viewportState = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .pNext         = nullptr,
            .flags         = 0,
            .viewportCount = 1,
            .pViewports    = &desc.Viewport,
            .scissorCount  = 1,
            .pScissors     = &desc.Scissor,
        };

        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType      = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount = static_cast<uint32_t>(desc.Shaders.size()),
        };
        
        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .pNext             = nullptr,
            .flags             = 0,
            .dynamicStateCount = 2,
            .pDynamicStates    = dynamicStates,
        };


        std::vector<VkPipelineShaderStageCreateInfo> shaderStages(desc.Shaders.size());

        for (size_t i = 0; i < desc.Shaders.size(); ++i) {
            shaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage  = desc.Shaders[i]->GetStage();
            shaderStages[i].module = desc.Shaders[i]->GetHandle();
            shaderStages[i].pName  = desc.Shaders[i]->GetEntryPoint();
        }

        pipelineInfo.stageCount = static_cast<uint32_t>(shaderStages.size());
        pipelineInfo.pStages    = shaderStages.data();

        pipelineInfo.pVertexInputState   = &desc.VertexInput;
        pipelineInfo.pInputAssemblyState = &desc.InputAssembly;
        pipelineInfo.pTessellationState  = nullptr;
        pipelineInfo.pViewportState      = &viewportState;
        pipelineInfo.pRasterizationState = &desc.Rasterizer;
        pipelineInfo.pMultisampleState   = &desc.Multisampling;
        pipelineInfo.pDepthStencilState  = &desc.DepthStencil;
        pipelineInfo.pColorBlendState    = &colorBlendState;
        pipelineInfo.pDynamicState = &dynamicState;

        pipelineInfo.layout     = mPipelineLayout;
        pipelineInfo.renderPass = mRenderPass->GetHandle();
        pipelineInfo.subpass    = 0;

        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
        pipelineInfo.basePipelineIndex  = -1;

        if (vkCreateGraphicsPipelines(mDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
            LOG_FATAL("Failed to create graphics pipeline!");
        }

        LOG_INFO("Created graphics pipeline: {} | Layout: {}", (void*) mPipeline, (void*) mPipelineLayout);
    }

    const std::vector<Ref<VulkanShader>>& VulkanPipeline::GetShaders() const {
        return mShaders;
    }

    VkPipeline VulkanPipeline::GetHandle() const {
        return mPipeline;
    }

    VkPipelineLayout VulkanPipeline::GetLayout() const {
        return mPipelineLayout;
    }


    VulkanPipeline::~VulkanPipeline() {
        if (mPipeline != VK_NULL_HANDLE) {
            vkDestroyPipeline(mDevice->GetHandle(), mPipeline, nullptr);
            mPipeline = VK_NULL_HANDLE;
        }

        if (mPipelineLayout != VK_NULL_HANDLE) {
            vkDestroyPipelineLayout(mDevice->GetHandle(), mPipelineLayout, nullptr);
            mPipelineLayout = VK_NULL_HANDLE;
        }
    }

} // namespace golias
