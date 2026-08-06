#pragma once

#include "stdafx.h"
#include <vulkan/vulkan.h>
#include <vulkan/vk_enum_string_helper.h>

#define VK_CHECK_RESULT(f)                    \
    do {                                      \
        VkResult res = (f);                   \
        if (res != VK_SUCCESS) {              \
            LOG_ERROR("VkResult failed");     \
            GOLIAS_ASSERT(res == VK_SUCCESS); \
        }                                     \
    } while (0)
