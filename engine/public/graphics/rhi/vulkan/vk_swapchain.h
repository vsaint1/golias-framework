#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanDevice;
    class VulkanRenderPass;
    class VulkanWindowSurface;
    class Window;


    struct VulkanSwapchainSupportDetails {
        VkSurfaceCapabilitiesKHR Capabilities;
        std::vector<VkSurfaceFormatKHR> Formats;
        std::vector<VkPresentModeKHR> PresentModes;
    };

    class VulkanSwapchain {
    public:
        VulkanSwapchain(Ref<VulkanDevice> device, Ref<VulkanWindowSurface> windowSurface, Ref<Window> window);
        ~VulkanSwapchain();

        VkSwapchainKHR GetHandle() const {
            return mSwapchain;
        }

        VkFormat GetImageFormat() const {
            return mImageFormat;
        }

        VkExtent2D GetExtent() const {
            return mExtent;
        }

        uint32_t GetImageCount() const {
            return static_cast<uint32_t>(mSwapchainImages.size());
        }

        const std::vector<VkImage>& GetSwapchainImages() const {
            return mSwapchainImages;
        }


    private:
        VulkanSwapchainSupportDetails QuerySwapchainSupport();
        VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
        VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
        VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
        void CreateImageViews();

    private:
        VkFormat mImageFormat             = VK_FORMAT_UNDEFINED;
        VkExtent2D mExtent                = {0, 0};
        VkSurfaceFormatKHR mSurfaceFormat = {};
        VkPresentModeKHR mPresentMode     = VK_PRESENT_MODE_FIFO_KHR;

        VkSwapchainKHR mSwapchain = VK_NULL_HANDLE;

        uint32_t mImageCount = 0;
        std::vector<VkImage> mSwapchainImages;
        std::vector<VkImageView> mSwapchainImageViews;
        std::vector<VkFramebuffer> mSwapchainFramebuffers;

        Ref<VulkanDevice> mDevice               = nullptr;
        Ref<VulkanWindowSurface> mWindowSurface = nullptr;
        Ref<Window> mWindow                     = nullptr;
    };


} // namespace golias
