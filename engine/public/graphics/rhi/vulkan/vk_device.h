#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanInstance;
    class VulkanWindowSurface;

    class VulkanDevice {
    public:
        VulkanDevice(Ref<VulkanInstance> instance, Ref<VulkanWindowSurface> surface);
        ~VulkanDevice();


        VkPhysicalDevice GetPhysicalDeviceHandle() const {
            return mPhysicalDevice;
        }

        VkDevice GetHandle() const {
            return mDevice;
        }

        VkQueue GetGraphicsQueue() const {
            return mGraphicsQueue;
        }

        VkQueue GetPresentQueue() const {
            return mPresentQueue;
        }

        uint32_t GetGraphicsQueueFamilyIndex() const {
            return mGraphicsQueueFamilyIndex;
        }

        uint32_t GetPresentQueueFamilyIndex() const {
            return mPresentQueueFamilyIndex;
        }

        uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

    private:
        void PickPhysicalDevice();
        void CreateLogicalDevice();
        void SetupDeviceQueues();

        const char* GetVendorName(uint32_t vendorID) const;

    private:
        Ref<VulkanInstance> mInstance     = nullptr;
        Ref<VulkanWindowSurface> mSurface = nullptr;

        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice mDevice                 = VK_NULL_HANDLE;

        VkQueue mGraphicsQueue = VK_NULL_HANDLE;
        VkQueue mPresentQueue  = VK_NULL_HANDLE;

        uint32_t mGraphicsQueueFamilyIndex = UINT32_MAX;
        uint32_t mPresentQueueFamilyIndex  = UINT32_MAX;

        const std::vector<const char*> mRequiredDeviceExtensions = {"VK_KHR_swapchain"};

    };
} // namespace golias
