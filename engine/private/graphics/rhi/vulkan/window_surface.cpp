#include "graphics/rhi/vulkan/window_surface.h"
#include "graphics/rhi/vulkan/instance.h"
#include "core/window.h"


namespace golias {

    VulkanWindowSurface::VulkanWindowSurface(Ref<VulkanInstance> instance, Ref<Window> window) : mInstance(instance) {
        VkResult result = glfwCreateWindowSurface(instance->GetInstance(), window->GetNativeWindow(), nullptr, &mSurface);
        if (result != VK_SUCCESS) {
            LOG_FATAL("Failed to create Vulkan window surface.");
        }
    }

    VulkanWindowSurface::~VulkanWindowSurface() {
        vkDestroySurfaceKHR(GetInstance(), mSurface, nullptr);
    }

    VkSurfaceKHR VulkanWindowSurface::GetSurface() const {
        return mSurface;
    }

    VkInstance VulkanWindowSurface::GetInstance() const {
        return mInstance->GetInstance();
    }

} // namespace golias