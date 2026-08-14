#include "graphics/rhi/vulkan/vk_surface.h"

#include "core/window.h"

namespace golias {

    std::vector<const char*> GetVulkanInstanceExtensions() {
        uint32_t count = 0;
        const char** extensions = glfwGetRequiredInstanceExtensions(&count);
        if (!extensions || count == 0) {
            LOG_ERROR("GLFW did not provide Vulkan instance extensions.");
            return {};
        }

        return {extensions, extensions + count};
    }

    VkResult CreateVulkanSurface(const Window& window, VkInstance instance, VkSurfaceKHR* surface) {
        auto* glfwWindow = static_cast<GLFWwindow*>(window.GetGLFWHandle());
        if (!glfwWindow) {
            return VK_ERROR_INITIALIZATION_FAILED;
        }

        return glfwCreateWindowSurface(instance, glfwWindow, nullptr, surface);
    }

} // namespace golias
