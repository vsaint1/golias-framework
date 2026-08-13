#pragma once

#include "graphics/rhi/rhi_types.h"
#include <cstddef>
#include <vulkan/vulkan.h>

namespace golias {

    VkShaderModule CreateShaderModule(VkDevice device, const void* code, size_t size);

    VkResult CreateDebugMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* info, VkDebugUtilsMessengerEXT* messenger);

    void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger);

    VkPipelineStageFlags StageForLayout(VkImageLayout layout);

    VkAccessFlags AccessForLayout(VkImageLayout layout);

    VkImageMemoryBarrier MakeImageMemoryBarrier(VkImage image,
                                                VkImageLayout oldLayout,
                                                VkImageLayout newLayout,
                                                VkAccessFlags srcAccess,
                                                VkAccessFlags dstAccess,
                                                VkImageAspectFlags aspect,
                                                uint32_t baseMipLevel,
                                                uint32_t levelCount,
                                                uint32_t baseArrayLayer,
                                                uint32_t layerCount);

    VkSampleCountFlagBits ToVkSampleCount(SampleCount count);

    VkFormat ToVkFormat(TextureFormat format, VkFormat swapchainFormat);

    VkPrimitiveTopology ToVkTopology(PrimitiveType type);

    VkCullModeFlags ToVkCullMode(CullMode mode);

    VkFrontFace ToVkFrontFace(FrontFace face);

    VkCompareOp ToVkCompareOp(CompareOp op);

    VkBlendFactor ToVkBlendFactor(BlendFactor factor);

    VkBlendOp ToVkBlendOp(BlendOp op);

    VkColorComponentFlags ToVkColorMask(ColorComponent mask);

    VkFilter ToVkFilter(Filter filter);

    VkSamplerMipmapMode ToVkMipmapMode(SamplerMipmapMode mode);

    VkSamplerAddressMode ToVkAddressMode(SamplerAddressMode mode);

    VkImageAspectFlags AspectForFormat(TextureFormat format);

    uint32_t BytesPerPixel(TextureFormat format);

    bool IsDepthFormat(TextureFormat format);

    bool HasStencil(TextureFormat format);

} // namespace golias

#define VK_CHECK_RESULT(expression)                                                             \
    ([&]() {                                                                                    \
        const VkResult result = (expression);                                                   \
        if (result != VK_SUCCESS) {                                                             \
            LOG_ERROR("{} failed with Vulkan error {}", #expression, static_cast<int>(result)); \
            return false;                                                                       \
        }                                                                                       \
        return true;                                                                            \
    }())
