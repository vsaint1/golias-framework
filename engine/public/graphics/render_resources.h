#pragma once

#include "core/asset/asset.h"
#include "graphics/rhi/rhi_device.h"
#include "stdafx.h"


namespace golias {

    using ShaderPropertyId                        = uint32_t;
    constexpr ShaderPropertyId kInvalidPropertyId = 0;

    inline ShaderPropertyId MakePropertyId(const String& name) {
        const ShaderPropertyId id = static_cast<ShaderPropertyId>(std::hash<String>{}(name));
        return id == kInvalidPropertyId ? 1u : id;
    }

    inline ShaderPropertyId MakePropertyId(const char* name) {
        return MakePropertyId(String(name ? name : ""));
    }


    // TODO: add Texture3D, TextureCube, Matrices, etc.
    enum class ShaderPropertyType : uint8_t {
        Float, // Single float value
        Float2, // 2D vector of floats
        Float3, // 3D vector of floats
        Float4, // 4D vector of floats
        Color, // Color value (can be represented as a 4D vector of floats)
        Texture2D // 2D texture resource
    };

    enum class BlendMode : uint8_t {
        Opaque,
        Alpha,
        Additive,
        Premultiplied,
    };

    struct ShaderProperty {
        ShaderPropertyId Id = kInvalidPropertyId;
        String Name;
        ShaderPropertyType Type = ShaderPropertyType::Float;
        uint32_t Binding        = 0;
        ShaderStage Stage      = ShaderStage::Vertex;
    };

    using MaterialPropertyValue = std::variant<float, glm::vec2, glm::vec3, glm::vec4, Ref<class Texture2D>>;

    class MaterialPropertyBlock {
    public:
        void Set(ShaderPropertyId id, const MaterialPropertyValue& value);

        const MaterialPropertyValue* Get(ShaderPropertyId id) const;

        bool Contains(ShaderPropertyId id) const;

    private:
        std::unordered_map<ShaderPropertyId, MaterialPropertyValue> mValues = {};
    };

    struct MeshSubmesh {
        uint32_t FirstIndex = 0;
        uint32_t IndexCount = 0;
    };

    class Mesh : public Asset {
    public:
        std::vector<float> Vertices;
        std::vector<uint32_t> IndicesData;


        // Runtime-only resources generated from the asset data.
        BufferHandle VertexBuffer;
        BufferHandle IndexBuffer;
        IndexFormat Indices = IndexFormat::UInt32;
        std::vector<MeshSubmesh> Submeshes;
        std::vector<VertexBufferDesc> VertexBuffers;
        std::vector<VertexAttribute> VertexAttributes;
    };

    // TODO: Make this read only
    class Texture2D : public Asset {
    public:
        TextureHandle GetHandle() const;

        uint32_t GetWidth() const;

        uint32_t GetHeight() const;

        // Runtime-only GPU resource. Import data remains in the asset.
        TextureHandle Handle;
        uint32_t Width  = 0;
        uint32_t Height = 0;
    };

    class Shader : public Asset {
    public:
        ShaderHandle GetHandle(ShaderStage stage = ShaderStage::Vertex) const;

        const std::vector<uint8_t>& GetCompiledBinary() const;

        void AddProperty(ShaderProperty property);
        
        void AddProperty(const String& name, ShaderPropertyType type, uint32_t binding, ShaderStage stages = ShaderStage::Fragment);

        const ShaderProperty* FindProperty(ShaderPropertyId id) const;

        const ShaderProperty* FindProperty(const String& name) const;

        const std::unordered_map<ShaderPropertyId, ShaderProperty>& GetProperties() const;

        std::vector<uint8_t> Binary;
        String EntryPoint = "main";

        // Runtime-only handles. A shader asset may be compiled for each backend.
        ShaderHandle VertexHandle;
        ShaderHandle FragmentHandle;

    private:
        std::unordered_map<ShaderPropertyId, ShaderProperty> mProperties = {};
    };

    class Material : public Asset {
    public:
        void SetShader(Ref<Shader> shader);

        Ref<Shader> GetShader() const;

        void SetFloat(ShaderPropertyId id, float value);
        void SetFloat(const String& name, float value);

        void SetVec2(ShaderPropertyId id, const glm::vec2& value);
        void SetVec2(const String& name, const glm::vec2& value);

        void SetVec3(ShaderPropertyId id, const glm::vec3& value);
        void SetVec3(const String& name, const glm::vec3& value);

        void SetColor(ShaderPropertyId id, const glm::vec4& value);
        void SetColor(const String& name, const glm::vec4& value);

        void SetTexture(ShaderPropertyId id, Ref<Texture2D> texture);
        void SetTexture(const String& name, Ref<Texture2D> texture);

        const MaterialPropertyBlock& GetProperties() const;

    private:
        Ref<Shader> mShader               = nullptr;
        MaterialPropertyBlock mProperties = {};
    };

    class MaterialInstance {
    public:
        Ref<Material> Parent = nullptr;

        void SetFloat(ShaderPropertyId id, float value);
        void SetFloat(const String& name, float value);

        void SetVec2(ShaderPropertyId id, const glm::vec2& value);
        void SetVec2(const String& name, const glm::vec2& value);

        void SetVec3(ShaderPropertyId id, const glm::vec3& value);
        void SetVec3(const String& name, const glm::vec3& value);

        void SetColor(ShaderPropertyId id, const glm::vec4& value);
        void SetColor(const String& name, const glm::vec4& value);

        void SetTexture2D(ShaderPropertyId id, Ref<Texture2D> texture);
        void SetTexture2D(const String& name, Ref<Texture2D> texture);

        void SetBlendMode(BlendMode mode);
        BlendMode GetBlendMode() const;

        void SetDepthTest(bool enabled);
        bool GetDepthTest() const;

        void SetDepthWrite(bool enabled);
        bool GetDepthWrite() const;

        void SetCullMode(CullMode mode);
        CullMode GetCullMode() const;

        const MaterialPropertyValue* Resolve(ShaderPropertyId id) const;

        const MaterialPropertyValue* Resolve(const String& name) const;

        const MaterialPropertyBlock& GetOverrides() const;

    private:
        MaterialPropertyBlock mOverrides;
        BlendMode mBlendMode = BlendMode::Opaque;
        bool mDepthTest = true;
        std::optional<bool> mDepthWriteOverride;
        CullMode mCullMode = CullMode::None;
    };

} // namespace golias
