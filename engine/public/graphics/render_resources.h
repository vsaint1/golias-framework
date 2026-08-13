#pragma once

#include "core/asset/asset.h"
#include "graphics/rhi/rhi_device.h"
#include "stdafx.h"

#include <glm/glm.hpp>

namespace golias {

    using ShaderPropertyId = uint32_t;

    constexpr ShaderPropertyId kInvalidPropertyId = 0;

    inline ShaderPropertyId MakePropertyId(const String& name) {
        return static_cast<ShaderPropertyId>(std::hash<String>{}(name));
    }

    inline ShaderPropertyId MakePropertyId(const char* name) {
        return MakePropertyId(String(name));
    }

    struct MeshSubmesh {
        uint32_t FirstIndex = 0;
        uint32_t IndexCount = 0;
    };

    class Mesh : public Asset {
    public:
        std::vector<float> Vertices;
        std::vector<uint32_t> IndicesData;
        BufferHandle VertexBuffer;
        BufferHandle IndexBuffer;
        IndexFormat Indices = IndexFormat::UInt32;
        std::vector<MeshSubmesh> Submeshes;
    };

    class Texture2D : public Asset {
    public:
        TextureHandle GetHandle() const;

        uint32_t GetWidth() const;

        uint32_t GetHeight() const;

        TextureHandle Handle;
        uint32_t Width  = 0;
        uint32_t Height = 0;
    };

    class Shader : public Asset {
    public:
        ShaderHandle GetHandle() const;
        const std::vector<uint8_t>& GetCompiledBinary() const;

        ShaderHandle Handle;
        std::vector<uint8_t> Binary;
    };

    struct TextureProperty {
        uint32_t slot    = 0;
        bool vertexStage = false;
    };

    class Material : public Asset {
    public:
        GraphicsPipelineHandle Pipeline;

        void DefineTextureProperty(RHIDevice& device, const String& name, uint32_t slot, bool vertexStage = false);

        void DefineTextureProperty(RHIDevice& device, const char* name, uint32_t slot, bool vertexStage = false);

        const TextureProperty* GetPropertyBinding(ShaderPropertyId id) const;

        const TextureProperty* GetPropertyBinding(const String& name) const;

        SamplerHandle GetDefaultSampler() const;

    private:
        std::unordered_map<ShaderPropertyId, TextureProperty> mPropertyBindings;
        SamplerHandle mDefaultSampler;
    };

    struct BoundTexture {
        ShaderPropertyId PropertyId = kInvalidPropertyId;
        uint32_t Slot               = 0;
        TextureHandle Texture;
        SamplerHandle Sampler;
        bool VertexStage = false;
    };

    class MaterialInstance {
    public:
        Ref<Material> Parent;
        glm::vec4 Color = {1, 1, 1, 1};

        void SetTexture(const String& name, TextureHandle texture);

        void SetTexture2D(const char* name, Ref<Texture2D> texture);

        const std::vector<BoundTexture>& GetTextures() const;

        bool HasTextures() const;

    private:
        std::vector<BoundTexture> mTextures;
    };

} // namespace golias
