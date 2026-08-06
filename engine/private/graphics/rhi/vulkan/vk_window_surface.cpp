#include "graphics/rhi/vulkan/vk_window_surface.h"
#include "graphics/rhi/vulkan/vk_instance.h"
#include "core/window.h"


namespace golias {

    VulkanWindowSurface::VulkanWindowSurface(Ref<VulkanInstance> instance, Ref<Window> window) : mInstance(instance) {
        VkResult result = glfwCreateWindowSurface(instance->GetHandle(), window->GetNativeWindow(), nullptr, &mSurface);
        VK_CHECK_RESULT(result);

        LOG_INFO("Created Vulkan window surface.");
    }

    VulkanWindowSurface::~VulkanWindowSurface() {
        if(mSurface != VK_NULL_HANDLE) {
            vkDestroySurfaceKHR(GetInstance(), mSurface, nullptr);
            mSurface = VK_NULL_HANDLE;
        }
        
    }

    VkSurfaceKHR VulkanWindowSurface::GetHandle() const {
        return mSurface;
    }

    VkInstance VulkanWindowSurface::GetInstance() const {
        return mInstance->GetHandle();
    }

} // namespace golias