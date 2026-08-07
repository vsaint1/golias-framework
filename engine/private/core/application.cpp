#include "core/application.h"

#include "graphics/rhi/vulkan/vk_command_buffer.h"
#include "graphics/rhi/vulkan/vk_command_pool.h"
#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_instance.h"
#include "graphics/rhi/vulkan/vk_pipeline.h"
#include "graphics/rhi/vulkan/vk_renderpass.h"
#include "graphics/rhi/vulkan/vk_shader.h"
#include "graphics/rhi/vulkan/vk_swapchain.h"
#include "graphics/rhi/vulkan/vk_window_surface.h"

namespace golias {


    Application::Application(const ApplicationConfig& config) : mConfig(config) {
    }

    Application::~Application() {
    }

    void Application::Run() {
        if (!Initialize()) {
            LOG_ERROR("Failed to initialize application.");
            return;
        }

        MainLoop();
        Shutdown();
    }

    bool Application::Initialize() {
        if (!glfwInit()) {
            LOG_ERROR("Failed to initialize the Windowing system.");
            return false;
        }

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

        mWindow = std::make_shared<Window>(mConfig.Width, mConfig.Height, mConfig.Title);

        mWindow->OnResize = [this](int width, int height) {
            mConfig.Width  = width;
            mConfig.Height = height;
            LOG_INFO("Resized to {}x{}", width, height);
        };

        mInstance = std::make_shared<VulkanInstance>();

        mWindowSurface = std::make_shared<VulkanWindowSurface>(mInstance, mWindow);

        mDevice = std::make_shared<VulkanDevice>(mInstance, mWindowSurface);

        mCommandPool = std::make_shared<VulkanCommandPool>(mDevice);

        mSwapchain = std::make_shared<VulkanSwapchain>(mDevice, mWindowSurface, mWindow);

        {
            auto rp                                 = std::make_shared<VulkanRenderPass>(mDevice);
            VkAttachmentDescription colorAttachment = {
                .format         = mSwapchain->GetImageFormat(),
                .samples        = VK_SAMPLE_COUNT_1_BIT,
                .loadOp         = VK_ATTACHMENT_LOAD_OP_CLEAR,
                .storeOp        = VK_ATTACHMENT_STORE_OP_STORE,
                .stencilLoadOp  = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
                .stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
                .initialLayout  = VK_IMAGE_LAYOUT_UNDEFINED,
                .finalLayout    = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            };

            VkAttachmentReference colorAttachmentRef = {
                .attachment = 0,
                .layout     = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            };

            VulkanSubPass subpass;
            subpass.AddColorAttachment(colorAttachmentRef);

            rp->AddSubPass(subpass);
            rp->AddAttachment(colorAttachment);

            VkSubpassDependency dependency = {
                .srcSubpass    = VK_SUBPASS_EXTERNAL,
                .dstSubpass    = 0,
                .srcStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .dstStageMask  = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                .srcAccessMask = 0,
                .dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            };

            rp->AddDependency(dependency);
            rp->Build();

            mRenderPass = rp;
            mSwapchain->CreateFramebuffer(rp);
        }

        {
            auto pipeline                                        = std::make_shared<VulkanPipeline>(mDevice, mRenderPass);
            VkPipelineVertexInputStateCreateInfo vertexInputInfo = {
                .sType                           = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
                .vertexBindingDescriptionCount   = 0,
                .pVertexBindingDescriptions      = nullptr,
                .vertexAttributeDescriptionCount = 0,
                .pVertexAttributeDescriptions    = nullptr,
            };


            VkPipelineInputAssemblyStateCreateInfo inputAssembly = {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
                .topology               = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
                .primitiveRestartEnable = VK_FALSE,
            };

            VkViewport viewport = {
                .x        = 0.0f,
                .y        = 0.0f,
                .width    = static_cast<float>(mSwapchain->GetExtent().width),
                .height   = static_cast<float>(mSwapchain->GetExtent().height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = mSwapchain->GetExtent(),
            };

            VkPipelineRasterizationStateCreateInfo rasterizer = {.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
                                                                 .depthClampEnable        = VK_FALSE,
                                                                 .rasterizerDiscardEnable = VK_FALSE,
                                                                 .polygonMode             = VK_POLYGON_MODE_FILL,
                                                                 .cullMode                = VK_CULL_MODE_BACK_BIT,
                                                                 .frontFace               = VK_FRONT_FACE_CLOCKWISE,
                                                                 .depthBiasEnable         = VK_FALSE,
                                                                 .lineWidth               = 1.0f};

            VkPipelineMultisampleStateCreateInfo multisampling = {
                .sType                = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
                .pNext                = nullptr,
                .flags                = 0,
                .rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
                .sampleShadingEnable  = VK_FALSE,
            };

            VkPipelineColorBlendAttachmentState colorBlendAttachment = {
                .blendEnable    = VK_FALSE,
                .colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
            };

            VkPipelineLayoutCreateInfo pipelineLayoutInfo = {
                .sType                  = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
                .setLayoutCount         = 0,
                .pSetLayouts            = nullptr,
                .pushConstantRangeCount = 0,
                .pPushConstantRanges    = nullptr,
            };

            VulkanGraphicsPipelineDesc pipelineDesc;
            pipelineDesc.VertexInput          = vertexInputInfo;
            pipelineDesc.InputAssembly        = inputAssembly;
            pipelineDesc.Viewport             = viewport;
            pipelineDesc.Scissor              = scissor;
            pipelineDesc.Rasterizer           = rasterizer;
            pipelineDesc.Multisampling        = multisampling;
            pipelineDesc.ColorBlendAttachment = colorBlendAttachment;
            pipelineDesc.Layout               = pipelineLayoutInfo;
            pipelineDesc.Shaders              = {
                std::make_shared<VulkanShader>(mDevice, "res/internal/shaders/vulkan/test.spv", "vertex_main", VK_SHADER_STAGE_VERTEX_BIT),
                std::make_shared<VulkanShader>(mDevice, "res/internal/shaders/vulkan/test.spv", "fragment_main", VK_SHADER_STAGE_FRAGMENT_BIT),
            };

            pipeline->CreateGraphicsPipeline(pipelineDesc);
            mPipeline = pipeline;
        }

        RecordCmdBuffers();

        return true;
    }

    void Application::MainLoop() {
        while (!mWindow->ShouldClose()) {
            mWindow->PollEvents();
            RenderFrame();
        }

        vkDeviceWaitIdle(mDevice->GetHandle());
    }

    void Application::RecordCmdBuffers() {
        auto framebuffers = mSwapchain->GetSwapchainFramebuffers();

        auto extent = mSwapchain->GetExtent();

        uint32_t imageCount = mSwapchain->GetImageCount();

        mCommandBuffers.resize(imageCount);

        for (size_t i = 0; i < imageCount; ++i) {
            mCommandBuffers[i] = std::make_shared<VulkanCommandBuffer>(mDevice, mCommandPool);

            VkCommandBufferBeginInfo beginInfo = {
                .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
                .flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
            };

            mCommandBuffers[i]->Begin(beginInfo.flags);

            VkClearValue clearColor = {{{0.0f, 0.0f, 0.5f, 1.0f}}};

            VkRenderPassBeginInfo renderPassInfo = {
                .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
                .renderPass      = mRenderPass->GetHandle(),
                .framebuffer     = framebuffers[i],
                .renderArea      = {{0, 0}, extent},
                .clearValueCount = 1,
                .pClearValues    = &clearColor,
            };

            mCommandBuffers[i]->BeginRenderPass(renderPassInfo);
            mCommandBuffers[i]->BindGraphicsPipeline(mPipeline);

            VkViewport viewport = {
                .x        = 0.0f,
                .y        = 0.0f,
                .width    = static_cast<float>(extent.width),
                .height   = static_cast<float>(extent.height),
                .minDepth = 0.0f,
                .maxDepth = 1.0f,
            };

            vkCmdSetViewport(mCommandBuffers[i]->GetHandle(), 0, 1, &viewport);

            VkRect2D scissor = {
                .offset = {0, 0},
                .extent = extent,
            };

            vkCmdSetScissor(mCommandBuffers[i]->GetHandle(), 0, 1, &scissor);

            mCommandBuffers[i]->Draw(3);
            mCommandBuffers[i]->EndRenderPass();

            mCommandBuffers[i]->End();
        }
    }

    void Application::RenderFrame() {
        uint32_t imageIndex = 0;
        VkResult result =
            vkAcquireNextImageKHR(mDevice->GetHandle(), mSwapchain->GetHandle(), UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

        VkCommandBuffer commandBuffer = mCommandBuffers[imageIndex]->GetHandle();

        VkSubmitInfo submitInfo = {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &commandBuffer,
        };

        VkQueue graphicsQueue = mDevice->GetGraphicsQueue();

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
            LOG_ERROR("Failed to submit draw command buffer!");
        }
        
        vkQueueWaitIdle(graphicsQueue);

        VkSwapchainKHR swapchain = mSwapchain->GetHandle();

        VkPresentInfoKHR presentInfo = {
            .sType          = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount = 1,
            .pSwapchains    = &swapchain,
            .pImageIndices  = &imageIndex,
        };

        vkQueuePresentKHR(mDevice->GetPresentQueue(), &presentInfo);
    }

    void Application::Shutdown() {
        mCommandBuffers.clear();
        mPipeline.reset();
        mRenderPass.reset();
        mSwapchain.reset();
        mCommandPool.reset();
        mDevice.reset();
        mWindowSurface.reset();
        mInstance.reset();
        mWindow.reset();
    }


} // namespace golias
