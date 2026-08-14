#pragma once

#include "stdafx.h"
#include <vulkan/vulkan.h>

namespace golias {

    class Window;

    std::vector<const char*> GetVulkanInstanceExtensions();
    
    VkResult CreateVulkanSurface(const Window& window, VkInstance instance, VkSurfaceKHR* surface);

} // namespace golias
