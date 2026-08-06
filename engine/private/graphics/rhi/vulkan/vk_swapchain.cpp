#include "graphics/rhi/vulkan/vk_swapchain.h"

#include "core/window.h"
#include "graphics/rhi/vulkan/vk_device.h"
#include "graphics/rhi/vulkan/vk_window_surface.h"

namespace golias {

    VulkanSwapchain::VulkanSwapchain(Ref<VulkanDevice> device, Ref<VulkanWindowSurface> windowSurface, Ref<Window> window)
        : mDevice(device), mWindowSurface(windowSurface), mWindow(window) {

        VulkanSwapchainSupportDetails swapchainSupport = QuerySwapchainSupport();

        mExtent        = ChooseSwapExtent(swapchainSupport.Capabilities);
        mSurfaceFormat = ChooseSwapSurfaceFormat(swapchainSupport.Formats);
        mPresentMode   = ChooseSwapPresentMode(swapchainSupport.PresentModes);


        mImageCount = swapchainSupport.Capabilities.minImageCount + 1;
        if (swapchainSupport.Capabilities.maxImageCount > 0 && mImageCount > swapchainSupport.Capabilities.maxImageCount) {
            mImageCount = swapchainSupport.Capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo = {
            .sType            = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
            .pNext            = nullptr,
            .surface          = mWindowSurface->GetHandle(),
            .minImageCount    = mImageCount,
            .imageFormat      = mSurfaceFormat.format,
            .imageColorSpace  = mSurfaceFormat.colorSpace,
            .imageExtent      = mExtent,
            .imageArrayLayers = 1,
            .imageUsage       = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            .clipped          = VK_TRUE,
        };

        if (mDevice->GetGraphicsQueueFamilyIndex() == mDevice->GetPresentQueueFamilyIndex()) {
            createInfo.imageSharingMode      = VK_SHARING_MODE_EXCLUSIVE;
            createInfo.queueFamilyIndexCount = 0;
            createInfo.pQueueFamilyIndices   = nullptr;
        } else {
            uint32_t queueFamilyIndices[]    = {mDevice->GetGraphicsQueueFamilyIndex(), mDevice->GetPresentQueueFamilyIndex()};
            createInfo.imageSharingMode      = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices   = queueFamilyIndices;
        }

        createInfo.preTransform   = swapchainSupport.Capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode    = mPresentMode;

        VkResult result = vkCreateSwapchainKHR(mDevice->GetDevice(), &createInfo, nullptr, &mSwapchain);
        VK_CHECK_RESULT(result);

        mImageCount = 0;
        result      = vkGetSwapchainImagesKHR(mDevice->GetDevice(), mSwapchain, &mImageCount, nullptr);
        VK_CHECK_RESULT(result);

        mSwapchainImages.resize(mImageCount);
        result = vkGetSwapchainImagesKHR(mDevice->GetDevice(), mSwapchain, &mImageCount, mSwapchainImages.data());
        VK_CHECK_RESULT(result);

        CreateImageViews();

        LOG_INFO("Swapchain Extent: {}x{} |  Present Mode: {} | Surface Format: {} | Color Space: {} | Image Count: {}",
                 mExtent.width,
                 mExtent.height,
                 string_VkPresentModeKHR(mPresentMode),
                 string_VkFormat(mSurfaceFormat.format),
                 string_VkColorSpaceKHR(mSurfaceFormat.colorSpace),
                 mImageCount);
    }

    void VulkanSwapchain::CreateImageViews() {
        for (size_t i = 0; i < mImageCount; i++) {
            VkImageViewCreateInfo createInfo = {

                .sType    = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                .image    = mSwapchainImages[i],
                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                .format   = mSurfaceFormat.format,
                .components =
                    {
                                 .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 .a = VK_COMPONENT_SWIZZLE_IDENTITY,
                                 },
                .subresourceRange =
                    {
                                 .aspectMask     = VK_IMAGE_ASPECT_COLOR_BIT,
                                 .baseMipLevel   = 0,
                                 .levelCount     = 1,
                                 .baseArrayLayer = 0,
                                 .layerCount     = 1,
                                 },
            };

            VkResult result = vkCreateImageView(mDevice->GetDevice(), &createInfo, nullptr, &mSwapchainImageViews.emplace_back());
            VK_CHECK_RESULT(result);
        };
    }

    VulkanSwapchain::~VulkanSwapchain() {
        for (const auto& framebuffer : mSwapchainFramebuffers) {
            vkDestroyFramebuffer(mDevice->GetDevice(), framebuffer, nullptr);
        }

        for (const auto& imageView : mSwapchainImageViews) {
            vkDestroyImageView(mDevice->GetDevice(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(mDevice->GetDevice(), mSwapchain, nullptr);
    }


    VulkanSwapchainSupportDetails VulkanSwapchain::QuerySwapchainSupport() {
        VulkanSwapchainSupportDetails details;
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(mDevice->GetPhysicalDevice(), mWindowSurface->GetHandle(), &details.Capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(mDevice->GetPhysicalDevice(), mWindowSurface->GetHandle(), &formatCount, nullptr);

        if (formatCount != 0) {
            details.Formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(
                mDevice->GetPhysicalDevice(), mWindowSurface->GetHandle(), &formatCount, details.Formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(mDevice->GetPhysicalDevice(), mWindowSurface->GetHandle(), &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.PresentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(
                mDevice->GetPhysicalDevice(), mWindowSurface->GetHandle(), &presentModeCount, details.PresentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR VulkanSwapchain::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& format : availableFormats) {
            if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return format;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR VulkanSwapchain::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& presentMode : availablePresentModes) {
            if (presentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return presentMode;
            }
        }

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D VulkanSwapchain::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != UINT32_MAX) {
            return capabilities.currentExtent;
        } else {
            VkExtent2D actualExtent = {static_cast<uint32_t>(mWindow->GetWidth()),
                                       static_cast<uint32_t>(mWindow->GetHeight())}; // TODO: query Framebuffer size?

            actualExtent.width =
                std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
            actualExtent.height =
                std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

            return actualExtent;
        }
    }

} // namespace golias
