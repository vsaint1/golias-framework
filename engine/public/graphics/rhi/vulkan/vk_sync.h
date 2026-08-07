#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanDevice;

    class VulkanFence {
    public:
        VulkanFence(Ref<VulkanDevice> device, bool signaled = false);
        ~VulkanFence();

        VkFence GetHandle() const;

        void Wait() const;
        
        void Reset() const;

    private:
        Ref<VulkanDevice> mDevice = nullptr;
        VkFence mFence            = VK_NULL_HANDLE;
    };


    class VulkanSemaphore {
    public:
        VulkanSemaphore(Ref<VulkanDevice> device);
        ~VulkanSemaphore();

        VkSemaphore GetHandle() const;

    private:
        Ref<VulkanDevice> mDevice = nullptr;
        VkSemaphore mSemaphore    = VK_NULL_HANDLE;
    };
} // namespace golias
