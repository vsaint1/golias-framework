#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanInstance;
    class VulkanWindowSurface;

    class VulkanDevice {
    public:
        VulkanDevice(Ref<VulkanInstance> instance, Ref<VulkanWindowSurface> surface);
        ~VulkanDevice();


        VkPhysicalDevice GetPhysicalDevice() const {
            return mPhysicalDevice;
        }

        VkDevice GetDevice() const {
            return mDevice;
        }

    private:
        void PickPhysicalDevice();
        void CreateLogicalDevice();

        const char* GetDeviceTypeCString(VkPhysicalDeviceType type) const;
        const char* GetVendorName(uint32_t vendorID) const;

    private:
        Ref<VulkanInstance> mInstance     = nullptr;
        Ref<VulkanWindowSurface> mSurface = nullptr;

        VkPhysicalDevice mPhysicalDevice = VK_NULL_HANDLE;
        VkDevice mDevice                 = VK_NULL_HANDLE;

        uint32_t mGraphicsQueueFamilyIndex = 0;
        uint32_t mPresentQueueFamilyIndex  = 0;
    };
} // namespace golias
