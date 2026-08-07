#include "core/application.h"

#include "graphics/rhi/vulkan/vk_command_pool.h"
#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_instance.h"
#include "graphics/rhi/vulkan/vk_renderpass.h"
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

        return true;
    }

    void Application::MainLoop() {
        while (!mWindow->ShouldClose()) {
            mWindow->PollEvents();
            RenderFrame();
        }
        
        vkDeviceWaitIdle(mDevice->GetHandle());
    }

    void Application::RenderFrame() {
        uint32_t imageIndex = 0;
        VkResult result =
            vkAcquireNextImageKHR(mDevice->GetHandle(), mSwapchain->GetHandle(), UINT64_MAX, VK_NULL_HANDLE, VK_NULL_HANDLE, &imageIndex);

        VkCommandBufferAllocateInfo allocInfo = {
            .sType              = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
            .commandPool        = mCommandPool->GetHandle(),
            .level              = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
            .commandBufferCount = 1,
        };

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(mDevice->GetHandle(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo = {
            .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
            .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
        };

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.5f, 1.0f}}};

        VkRenderPassBeginInfo renderPassInfo = {
            .sType           = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
            .renderPass      = mRenderPass->GetHandle(),
            .framebuffer     = mSwapchain->GetSwapchainFramebuffer(imageIndex),
            .renderArea      = {{0, 0}, mSwapchain->GetExtent()},
            .clearValueCount = 1,
            .pClearValues    = &clearColor,
        };


        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        // todo
        vkCmdEndRenderPass(commandBuffer);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo = {
            .sType              = VK_STRUCTURE_TYPE_SUBMIT_INFO,
            .commandBufferCount = 1,
            .pCommandBuffers    = &commandBuffer,
        };


        VkQueue graphicsQueue = mDevice->GetGraphicsQueue();
        vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(graphicsQueue);

        vkFreeCommandBuffers(mDevice->GetHandle(), mCommandPool->GetHandle(), 1, &commandBuffer);

        VkSwapchainKHR sc = mSwapchain->GetHandle();
        VkPresentInfoKHR presentInfo = {
            .sType              = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
            .swapchainCount     = 1,
            .pSwapchains        = &sc,
            .pImageIndices      = &imageIndex,
        };

        vkQueuePresentKHR(graphicsQueue, &presentInfo);
    }

    void Application::Shutdown() {
        mCommandPool.reset();
        mDevice.reset();
        mWindowSurface.reset();
        mInstance.reset();
        mWindow.reset();
    }


} // namespace golias
