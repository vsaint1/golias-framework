#include "graphics/rhi/vulkan/vk_common.h"

#include "graphics/rhi/rhi_types.h"

namespace golias {

    VkShaderModule CreateShaderModule(VkDevice device, const void* code, size_t size) {
        if (!code || size == 0 || (size % 4) != 0) {
            LOG_ERROR("Cannot create Vulkan shader module: invalid SPIR-V data.");
            return VK_NULL_HANDLE;
        }

        VkShaderModuleCreateInfo info = {
            .sType    = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
            .codeSize = size,
            .pCode    = reinterpret_cast<const uint32_t*>(code),
        };

        VkShaderModule module = VK_NULL_HANDLE;
        if (!VK_CHECK_RESULT(vkCreateShaderModule(device, &info, nullptr, &module))) {
            return VK_NULL_HANDLE;
        }
        return module;
    }

    VkResult
        CreateDebugMessenger(VkInstance instance, const VkDebugUtilsMessengerCreateInfoEXT* info, VkDebugUtilsMessengerEXT* messenger) {
        auto function =
            reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT"));
        return function ? function(instance, info, nullptr, messenger) : VK_ERROR_EXTENSION_NOT_PRESENT;
    }

    void DestroyDebugMessenger(VkInstance instance, VkDebugUtilsMessengerEXT messenger) {
        auto function =
            reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT"));

        if (function) {
            function(instance, messenger, nullptr);
        }
    }

    VkPipelineStageFlags StageForLayout(VkImageLayout layout) {
        switch (layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_PIPELINE_STAGE_TRANSFER_BIT;
        case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            return VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        default:
            return VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        }
    }

    VkAccessFlags AccessForLayout(VkImageLayout layout) {
        switch (layout) {
        case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
            return VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            return VK_ACCESS_SHADER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
            return VK_ACCESS_TRANSFER_READ_BIT;
        case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
            return VK_ACCESS_TRANSFER_WRITE_BIT;
        default:
            return 0;
        }
    }

    VkImageMemoryBarrier MakeImageMemoryBarrier(VkImage image,
                                                VkImageLayout oldLayout,
                                                VkImageLayout newLayout,
                                                VkAccessFlags srcAccess,
                                                VkAccessFlags dstAccess,
                                                VkImageAspectFlags aspect,
                                                uint32_t baseMipLevel,
                                                uint32_t levelCount,
                                                uint32_t baseArrayLayer,
                                                uint32_t layerCount) {
        return {
            .sType         = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = srcAccess,
            .dstAccessMask = dstAccess,
            .oldLayout     = oldLayout,
            .newLayout     = newLayout,
            .image         = image,
            .subresourceRange =
                {
                                   .aspectMask     = aspect,
                                   .baseMipLevel   = baseMipLevel,
                                   .levelCount     = levelCount,
                                   .baseArrayLayer = baseArrayLayer,
                                   .layerCount     = layerCount,
                                   },
        };
    }

    VkSampleCountFlagBits ToVkSampleCount(SampleCount count) {
        switch (count) {
        case SampleCount::Count2:
            return VK_SAMPLE_COUNT_2_BIT;
        case SampleCount::Count4:
            return VK_SAMPLE_COUNT_4_BIT;
        case SampleCount::Count8:
            return VK_SAMPLE_COUNT_8_BIT;
        default:
            return VK_SAMPLE_COUNT_1_BIT;
        }
    }

    VkFormat ToVkFormat(TextureFormat format, VkFormat swapchainFormat) {
        switch (format) {
        case TextureFormat::R8G8B8A8_UNORM:
            return VK_FORMAT_R8G8B8A8_UNORM;
        case TextureFormat::R8G8B8A8_SRGB:
            return VK_FORMAT_R8G8B8A8_SRGB;
        case TextureFormat::B8G8R8A8_UNORM:
            return VK_FORMAT_B8G8R8A8_UNORM;
        case TextureFormat::B8G8R8A8_SRGB:
            return VK_FORMAT_B8G8R8A8_SRGB;
        case TextureFormat::R8_UNORM:
            return VK_FORMAT_R8_UNORM;
        case TextureFormat::R16G16B16A16_FLOAT:
            return VK_FORMAT_R16G16B16A16_SFLOAT;
        case TextureFormat::R32G32B32A32_FLOAT:
            return VK_FORMAT_R32G32B32A32_SFLOAT;
        case TextureFormat::D24_UNORM_S8_UINT:
            return VK_FORMAT_D24_UNORM_S8_UINT;
        case TextureFormat::D32_FLOAT:
            return VK_FORMAT_D32_SFLOAT;
        case TextureFormat::D32_FLOAT_S8_UINT:
            return VK_FORMAT_D32_SFLOAT_S8_UINT;
        case TextureFormat::Swapchain:
            return swapchainFormat;
        default:
            return VK_FORMAT_UNDEFINED;
        }
    }

    VkPrimitiveTopology ToVkTopology(PrimitiveType type) {
        switch (type) {
        case PrimitiveType::TriangleStrip:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
        case PrimitiveType::LineList:
            return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;
        case PrimitiveType::LineStrip:
            return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
        case PrimitiveType::PointList:
            return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        default:
            return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        }
    }

    VkCullModeFlags ToVkCullMode(CullMode mode) {
        switch (mode) {
        case CullMode::Front:
            return VK_CULL_MODE_FRONT_BIT;
        case CullMode::Back:
            return VK_CULL_MODE_BACK_BIT;
        default:
            return VK_CULL_MODE_NONE;
        }
    }

    VkFrontFace ToVkFrontFace(FrontFace face) {
        return face == FrontFace::Clockwise ? VK_FRONT_FACE_CLOCKWISE : VK_FRONT_FACE_COUNTER_CLOCKWISE;
    }

    VkCompareOp ToVkCompareOp(CompareOp op) {
        switch (op) {
        case CompareOp::Never:
            return VK_COMPARE_OP_NEVER;
        case CompareOp::Less:
            return VK_COMPARE_OP_LESS;
        case CompareOp::Equal:
            return VK_COMPARE_OP_EQUAL;
        case CompareOp::LessOrEqual:
            return VK_COMPARE_OP_LESS_OR_EQUAL;
        case CompareOp::Greater:
            return VK_COMPARE_OP_GREATER;
        case CompareOp::NotEqual:
            return VK_COMPARE_OP_NOT_EQUAL;
        case CompareOp::GreaterOrEqual:
            return VK_COMPARE_OP_GREATER_OR_EQUAL;
        default:
            return VK_COMPARE_OP_ALWAYS;
        }
    }

    VkBlendFactor ToVkBlendFactor(BlendFactor factor) {
        switch (factor) {
        case BlendFactor::Zero:
            return VK_BLEND_FACTOR_ZERO;
        case BlendFactor::One:
            return VK_BLEND_FACTOR_ONE;
        case BlendFactor::SrcColor:
            return VK_BLEND_FACTOR_SRC_COLOR;
        case BlendFactor::OneMinusSrcColor:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_COLOR;
        case BlendFactor::SrcAlpha:
            return VK_BLEND_FACTOR_SRC_ALPHA;
        case BlendFactor::OneMinusSrcAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::DstColor:
            return VK_BLEND_FACTOR_DST_COLOR;
        case BlendFactor::OneMinusDstColor:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_COLOR;
        case BlendFactor::DstAlpha:
            return VK_BLEND_FACTOR_DST_ALPHA;
        case BlendFactor::OneMinusDstAlpha:
            return VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
        default:
            return VK_BLEND_FACTOR_ONE;
        }
    }

    VkBlendOp ToVkBlendOp(BlendOp op) {
        switch (op) {
        case BlendOp::Subtract:
            return VK_BLEND_OP_SUBTRACT;
        case BlendOp::ReverseSubtract:
            return VK_BLEND_OP_REVERSE_SUBTRACT;
        case BlendOp::Min:
            return VK_BLEND_OP_MIN;
        case BlendOp::Max:
            return VK_BLEND_OP_MAX;
        default:
            return VK_BLEND_OP_ADD;
        }
    }

    VkColorComponentFlags ToVkColorMask(ColorComponent mask) {
        VkColorComponentFlags result = 0;
        const auto value             = static_cast<uint8_t>(mask);
        if (value & static_cast<uint8_t>(ColorComponent::R)) {
            result |= VK_COLOR_COMPONENT_R_BIT;
        }

        if (value & static_cast<uint8_t>(ColorComponent::G)) {
            result |= VK_COLOR_COMPONENT_G_BIT;
        }

        if (value & static_cast<uint8_t>(ColorComponent::B)) {
            result |= VK_COLOR_COMPONENT_B_BIT;
        }

        if (value & static_cast<uint8_t>(ColorComponent::A)) {
            result |= VK_COLOR_COMPONENT_A_BIT;
        }

        return result;
    }

    VkFilter ToVkFilter(Filter filter) {
        return filter == Filter::Nearest ? VK_FILTER_NEAREST : VK_FILTER_LINEAR;
    }

    VkSamplerMipmapMode ToVkMipmapMode(SamplerMipmapMode mode) {
        return mode == SamplerMipmapMode::Nearest ? VK_SAMPLER_MIPMAP_MODE_NEAREST : VK_SAMPLER_MIPMAP_MODE_LINEAR;
    }

    VkSamplerAddressMode ToVkAddressMode(SamplerAddressMode mode) {
        switch (mode) {
        case SamplerAddressMode::Repeat:
            return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        case SamplerAddressMode::MirroredRepeat:
            return VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT;
        default:
            return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
        }
    }

    bool IsDepthFormat(TextureFormat format) {
        return format == TextureFormat::D24_UNORM_S8_UINT || format == TextureFormat::D32_FLOAT
            || format == TextureFormat::D32_FLOAT_S8_UINT;
    }

    bool HasStencil(TextureFormat format) {
        return format == TextureFormat::D24_UNORM_S8_UINT || format == TextureFormat::D32_FLOAT_S8_UINT;
    }

    VkImageAspectFlags AspectForFormat(TextureFormat format) {
        if (HasStencil(format)) {
            return VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
        }

        return IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : VK_IMAGE_ASPECT_COLOR_BIT;
    }

    uint32_t BytesPerPixel(TextureFormat format) {
        switch (format) {
        case TextureFormat::R8_UNORM:
            return 1;
        case TextureFormat::R8G8B8A8_UNORM:
        case TextureFormat::R8G8B8A8_SRGB:
        case TextureFormat::B8G8R8A8_UNORM:
        case TextureFormat::B8G8R8A8_SRGB:
        case TextureFormat::D24_UNORM_S8_UINT:
        case TextureFormat::D32_FLOAT:
            return 4;
        case TextureFormat::D32_FLOAT_S8_UINT:
        case TextureFormat::R16G16B16A16_FLOAT:
            return 8;
        case TextureFormat::R32G32B32A32_FLOAT:
            return 16;
        default:
            return 0;
        }
    }

} // namespace golias
