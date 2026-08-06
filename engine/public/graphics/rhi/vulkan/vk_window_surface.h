#pragma once
#include "vk_common.h"

namespace golias {

    class VulkanInstance;
    class Window;

    class VulkanWindowSurface {
    public:
        VulkanWindowSurface(Ref<VulkanInstance> instance, Ref<Window> window);
        ~VulkanWindowSurface();

        VkSurfaceKHR GetHandle() const;
        VkInstance GetInstance() const;

    private:
        VkSurfaceKHR mSurface = VK_NULL_HANDLE;
        Ref<VulkanInstance> mInstance = nullptr;
    };
} // namespace golias
