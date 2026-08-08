#include "graphics/rhi/vulkan/vk_graphics_pipeline.h"

#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_renderpass.h"
#include "graphics/rhi/vulkan/vk_shader.h"
#include "graphics/rhi/vulkan/vk_uniform_buffer.h"

namespace golias {

    namespace {

        VkFormat ConvertVertexFormat(VertexFormat format) {
            switch (format) {
            case VertexFormat::Float:
                return VK_FORMAT_R32_SFLOAT;
            case VertexFormat::Float2:
                return VK_FORMAT_R32G32_SFLOAT;
            case VertexFormat::Float3:
                return VK_FORMAT_R32G32B32_SFLOAT;
            case VertexFormat::Float4:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            }

            LOG_FATAL("Unknown vertex format!");
            return VK_FORMAT_UNDEFINED;
        }

        VkPrimitiveTopology ConvertPrimitiveTopology(PrimitiveTopology topology) {
            switch (topology) {
            case PrimitiveTopology::TriangleList:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            case PrimitiveTopology::TriangleStrip:
                return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
            case PrimitiveTopology::LineList:
                return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
            case PrimitiveTopology::LineStrip:
                return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
            case PrimitiveTopology::PointList:
                return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
            }

            LOG_FATAL("Unknown primitive topology!");
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }

        VkCullModeFlags ConvertCullMode(CullMode cullMode) {
            switch (cullMode) {
            case CullMode::None:
                return VK_CULL_MODE_NONE;
            case CullMode::Front:
                return VK_CULL_MODE_FRONT_BIT;
            case CullMode::Back:
                return VK_CULL_MODE_BACK_BIT;
            }

            LOG_FATAL("Unknown cull mode!");
            return VK_CULL_MODE_NONE;
        }

        VkFrontFace ConvertFrontFace(FrontFace frontFace) {
            switch (frontFace) {
            case FrontFace::CounterClockwise:
                return VK_FRONT_FACE_COUNTER_CLOCKWISE;
            case FrontFace::Clockwise:
                return VK_FRONT_FACE_CLOCKWISE;
            }

            LOG_FATAL("Unknown front face!");
            return VK_FRONT_FACE_CLOCKWISE;
        }

        VkPolygonMode ConvertPolygonMode(PolygonMode polygonMode) {
            switch (polygonMode) {
            case PolygonMode::Fill:
                return VK_POLYGON_MODE_FILL;
            case PolygonMode::Line:
                return VK_POLYGON_MODE_LINE;
            case PolygonMode::Point:
                return VK_POLYGON_MODE_POINT;
            }

            LOG_FATAL("Unknown polygon mode!");
            return VK_POLYGON_MODE_FILL;
        }

    } // namespace

    VulkanGraphicsPipeline::VulkanGraphicsPipeline(Ref<VulkanDevice> device, Ref<VulkanRenderPass> renderPass)
        : mDevice(device), mRenderPass(renderPass) {
    }

    void VulkanGraphicsPipeline::Build(const GraphicsPipelineStateDesc& desc) {
        mUniformBufferSet = desc.UniformBuffers;

        // ---------------------------------------------------------
        // Vertex input state
        // ---------------------------------------------------------
        std::vector<VkVertexInputBindingDescription> bindingDescriptions;
        std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

        bindingDescriptions.reserve(desc.VertexLayouts.size());
        for (size_t i = 0; i < desc.VertexLayouts.size(); ++i) {
            const auto& layout = desc.VertexLayouts[i];

            VkVertexInputBindingDescription bindingDescription = {
                .binding   = static_cast<uint32_t>(i),
                .stride    = layout.Stride,
                .inputRate = layout.InputRate == VertexInputRate::PerInstance ? VK_VERTEX_INPUT_RATE_INSTANCE : VK_VERTEX_INPUT_RATE_VERTEX,
            };

            bindingDescriptions.push_back(bindingDescription);

            for (const auto& attribute : layout.Attributes) {
                VkVertexInputAttributeDescription attributeDescription = {
                    .location = attribute.Location,
                    .binding  = static_cast<uint32_t>(i),
                    .format   = ConvertVertexFormat(attribute.Format),
                    .offset   = static_cast<uint32_t>(attribute.Offset),
                };

                attributeDescriptions.push_back(attributeDescription);
            }
        }

        VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
            .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
            .vertexBindingDescriptionCount   = static_cast<uint32_t>(bindingDescriptions.size()),
            .pVertexBindingDescriptions      = bindingDescriptions.data(),
            .vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size()),
            .pVertexAttributeDescriptions    = attributeDescriptions.data(),
        };

        VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
            .topology               = ConvertPrimitiveTopology(desc.Topology),
            .primitiveRestartEnable = VK_FALSE,
        };

        // ---------------------------------------------------------
        // Rasterization
        // ---------------------------------------------------------
        VkPipelineRasterizationStateCreateInfo rasterizer = {
            .sType                   = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
            .depthClampEnable        = VK_FALSE,
            .rasterizerDiscardEnable = VK_FALSE,
            .polygonMode             = ConvertPolygonMode(desc.FillMode),
            .cullMode                = ConvertCullMode(desc.Cull),
            .frontFace               = ConvertFrontFace(desc.Winding),
            .depthBiasEnable         = VK_FALSE,
            .depthBiasConstantFactor = 0.0f,
            .depthBiasClamp          = 0.0f,
            .depthBiasSlopeFactor    = 0.0f,
            .lineWidth               = 1.0f,
        };

        VkPipelineMultisampleStateCreateInfo multisampling = {
            .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
            .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
            .sampleShadingEnable  = VK_FALSE,
        };

        VkPipelineDepthStencilStateCreateInfo depthStencil = {
            .sType                 = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
            .depthTestEnable       = desc.DepthTest ? VK_TRUE : VK_FALSE,
            .depthWriteEnable      = desc.DepthWrite ? VK_TRUE : VK_FALSE,
            .depthCompareOp        = VK_COMPARE_OP_LESS,
            .depthBoundsTestEnable = VK_FALSE,
            .stencilTestEnable     = VK_FALSE,
        };

        VkPipelineColorBlendAttachmentState colorBlendAttachment = {
            .blendEnable    = VK_FALSE,
            .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
        };

        VkPipelineColorBlendStateCreateInfo colorBlendState = {
            .sType           = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
            .logicOpEnable   = VK_FALSE,
            .logicOp         = VK_LOGIC_OP_COPY,
            .attachmentCount = 1,
            .pAttachments    = &colorBlendAttachment,
            .blendConstants  = {0.0f, 0.0f, 0.0f, 0.0f},
        };

        VkPipelineViewportStateCreateInfo viewportState = {
            .sType         = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
            .viewportCount = 1,
            .scissorCount  = 1,
        };

        VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

        VkPipelineDynamicStateCreateInfo dynamicState = {
            .sType             = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
            .dynamicStateCount = 2,
            .pDynamicStates    = dynamicStates,
        };

        // ---------------------------------------------------------
        // Pipeline layout (includes the UBO descriptor set layout)
        // ---------------------------------------------------------
        VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
        if (mUniformBufferSet != nullptr) {
            descriptorSetLayout = std::static_pointer_cast<VulkanUniformBufferSet>(mUniformBufferSet)->GetDescriptorSetLayout();
        }

        VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
            .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
            .setLayoutCount         = descriptorSetLayout != VK_NULL_HANDLE ? 1u : 0u,
            .pSetLayouts            = descriptorSetLayout != VK_NULL_HANDLE ? &descriptorSetLayout : nullptr,
            .pushConstantRangeCount = 0,
            .pPushConstantRanges    = nullptr,
        };

        if (vkCreatePipelineLayout(mDevice->GetHandle(), &pipelineLayoutInfo, nullptr, &mPipelineLayout) != VK_SUCCESS) {
            LOG_FATAL("Failed to create pipeline layout!");
        }

        // ---------------------------------------------------------
        // Shader stages
        // ---------------------------------------------------------
        std::vector<VkPipelineShaderStageCreateInfo> shaderStages(desc.Shaders.size());

        for (size_t i = 0; i < desc.Shaders.size(); ++i) {
            auto vkShader = std::static_pointer_cast<VulkanShader>(desc.Shaders[i]);

            shaderStages[i].sType  = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
            shaderStages[i].stage  = vkShader->GetStageFlagBits();
            shaderStages[i].module = vkShader->GetHandle();
            shaderStages[i].pName  = vkShader->GetEntryPoint();
        }

        // ---------------------------------------------------------
        // Graphics pipeline
        // ---------------------------------------------------------
        VkGraphicsPipelineCreateInfo pipelineInfo = {
            .sType               = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
            .stageCount          = static_cast<uint32_t>(shaderStages.size()),
            .pStages             = shaderStages.data(),
            .pVertexInputState   = &vertexInputInfo,
            .pInputAssemblyState = &inputAssembly,
            .pTessellationState  = nullptr,
            .pViewportState      = &viewportState,
            .pRasterizationState = &rasterizer,
            .pMultisampleState   = &multisampling,
            .pDepthStencilState  = &depthStencil,
            .pColorBlendState    = &colorBlendState,
            .pDynamicState       = &dynamicState,
            .layout              = mPipelineLayout,
            .renderPass          = mRenderPass->GetHandle(),
            .subpass             = 0,
            .basePipelineHandle  = VK_NULL_HANDLE,
            .basePipelineIndex   = -1,
        };

        if (vkCreateGraphicsPipelines(mDevice->GetHandle(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &mPipeline) != VK_SUCCESS) {
            LOG_FATAL("Failed to create graphics pipeline!");
        }

        LOG_INFO("Created graphics pipeline: {} | Layout: {}", (void*) mPipeline, (void*) mPipelineLayout);
    }

    VkPipeline VulkanGraphicsPipeline::GetHandle() const {
        return mPipeline;
    }

    VkPipelineLayout VulkanGraphicsPipeline::GetLayout() const {
        return mPipelineLayout;
    }

    const Ref<UniformBufferSet>& VulkanGraphicsPipeline::GetUniformBufferSet() const {
        return mUniformBufferSet;
    }

    VulkanGraphicsPipeline::~VulkanGraphicsPipeline() {
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
