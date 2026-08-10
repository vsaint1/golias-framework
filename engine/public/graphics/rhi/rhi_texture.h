#pragma once
#include "stdafx.h"

namespace golias {

    // Unified format enumeration covering both color render targets and
    // depth(-stencil) resources.
    enum class TextureFormat : uint8_t {
        R8,
        RG8,
        RGBA8,
        RGBA8SRGB,

        R16F,
        RG16F,
        RGBA16F,
        RGBA32F,

        Depth16,
        Depth24Stencil8,
        Depth32Float,
    };

    // Bitmask describing every bind point a texture may be used at.
    enum class TextureUsage : uint8_t {
        None            = 0,
        RenderTarget    = 1 << 0, // Color (or resolve) attachment in a render pass
        DepthStencil    = 1 << 1, // Depth/stencil attachment in a render pass
        ShaderResource  = 1 << 2, // Sampleable in shaders (SSAO, shadows, DOF, ...)
        UnorderedAccess = 1 << 3, // Read/write access from compute shaders
    };

    constexpr TextureUsage operator|(TextureUsage lhs, TextureUsage rhs) {
        return static_cast<TextureUsage>(static_cast<uint8_t>(lhs) | static_cast<uint8_t>(rhs));
    }

    constexpr TextureUsage operator&(TextureUsage lhs, TextureUsage rhs) {
        return static_cast<TextureUsage>(static_cast<uint8_t>(lhs) & static_cast<uint8_t>(rhs));
    }

    constexpr bool HasFlag(TextureUsage value, TextureUsage flag) {
        return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0;
    }

    // Describes a texture to be created.
    struct TextureDesc {
        uint32_t Width = 1;

        uint32_t Height = 1;

        TextureFormat Format = TextureFormat::RGBA8;

        TextureUsage Usage = TextureUsage::RenderTarget;

        // Array layer count (1 for a plain 2D texture).
        uint32_t ArraySize = 1;

    // MSAA sample count (1 for a non-multisampled texture).
    uint32_t SampleCount = 1;

    // Full mip chain depth (1 for a single-level texture). When loading from
    // a file with MipLevels > 1 the backend generates the lower levels.
    uint32_t MipLevels = 1;

    const void* Data = nullptr;

    size_t DataSize = 0;
};


    class Texture {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const = 0;

        virtual uint32_t GetHeight() const = 0;

        virtual TextureFormat GetFormat() const = 0;

        // Array layer count (1 for a plain 2D texture, >1 for texture
        // arrays, e.g. cascaded shadow maps).
        virtual uint32_t GetArraySize() const = 0;

        // MSAA sample count (1 for a non-multisampled texture).
        virtual uint32_t GetSampleCount() const = 0;

        // Mip chain depth (1 for a single-level texture).
        virtual uint32_t GetMipLevels() const = 0;

        // Bind points this texture was created for.
        virtual TextureUsage GetUsage() const = 0;

        bool HasUsage(TextureUsage usage) const {
            return HasFlag(GetUsage(), usage);
        }
    };

} // namespace golias
