#include "graphics/render_resources.h"

namespace golias {

    namespace {
        bool CheckProperty(const Ref<Shader>& shader, ShaderPropertyId id, ShaderPropertyType type) {
            const ShaderProperty* property = shader ? shader->FindProperty(id) : nullptr;
            if (!property || property->Type != type) {
                LOG_ERROR("Invalid material property id {}", id);
                return false;
            }

            return true;
        }

        bool CheckColorProperty(const Ref<Shader>& shader, ShaderPropertyId id) {
            const ShaderProperty* property = shader ? shader->FindProperty(id) : nullptr;
            if (!property || (property->Type != ShaderPropertyType::Color && property->Type != ShaderPropertyType::Float4)) {
                LOG_ERROR("Invalid color material property id {}", id);
                return false;
            }

            return true;
        }

    } // namespace

#pragma region MaterialPropertyBlock

    void MaterialPropertyBlock::Set(ShaderPropertyId id, const MaterialPropertyValue& value) {
        if (id != kInvalidPropertyId) {
            mValues[id] = value;
        }
    }

    const MaterialPropertyValue* MaterialPropertyBlock::Get(ShaderPropertyId id) const {
        auto it = mValues.find(id);
        return it == mValues.end() ? nullptr : &it->second;
    }

    bool MaterialPropertyBlock::Contains(ShaderPropertyId id) const {
        return mValues.find(id) != mValues.end();
    }

#pragma endregion


#pragma region Material

    void Material::SetShader(Ref<Shader> shader) {
        mShader = std::move(shader);
    }

    Ref<Shader> Material::GetShader() const {
        return mShader;
    }


    void Material::SetFloat(const String& name, float value) {
        SetFloat(MakePropertyId(name), value);
    }

    void Material::SetVec2(const String& name, const glm::vec2& value) {
        SetVec2(MakePropertyId(name), value);
    }


    void Material::SetVec3(const String& name, const glm::vec3& value) {
        SetVec3(MakePropertyId(name), value);
    }


    void Material::SetColor(const String& name, const glm::vec4& value) {
        SetColor(MakePropertyId(name), value);
    }


    void Material::SetTexture(const String& name, Ref<Texture2D> texture) {
        SetTexture(MakePropertyId(name), std::move(texture));
    }

    const MaterialPropertyBlock& Material::GetProperties() const {
        return mProperties;
    }

    void Material::SetFloat(ShaderPropertyId id, float value) {
        if (CheckProperty(mShader, id, ShaderPropertyType::Float)) {
            mProperties.Set(id, value);
        }
    }

    void Material::SetVec2(ShaderPropertyId id, const glm::vec2& value) {
        if (CheckProperty(mShader, id, ShaderPropertyType::Float2)) {
            mProperties.Set(id, value);
        }
    }

    void Material::SetVec3(ShaderPropertyId id, const glm::vec3& value) {
        if (CheckProperty(mShader, id, ShaderPropertyType::Float3)) {
            mProperties.Set(id, value);
        }
    }

    void Material::SetColor(ShaderPropertyId id, const glm::vec4& value) {
        if (CheckColorProperty(mShader, id)) {
            mProperties.Set(id, value);
        }
    }

    void Material::SetTexture(ShaderPropertyId id, Ref<Texture2D> texture) {
        if (CheckProperty(mShader, id, ShaderPropertyType::Texture2D)) {
            mProperties.Set(id, std::move(texture));
        }
    }

#pragma endregion

#pragma region Material Instance

    void MaterialInstance::SetFloat(const String& name, float value) {
        SetFloat(MakePropertyId(name), value);
    }

    void MaterialInstance::SetVec2(const String& name, const glm::vec2& value) {
        SetVec2(MakePropertyId(name), value);
    }


    void MaterialInstance::SetVec3(const String& name, const glm::vec3& value) {
        SetVec3(MakePropertyId(name), value);
    }


    void MaterialInstance::SetColor(const String& name, const glm::vec4& value) {
        SetColor(MakePropertyId(name), value);
    }


    void MaterialInstance::SetTexture2D(const String& name, Ref<Texture2D> texture) {
        SetTexture2D(MakePropertyId(name), std::move(texture));
    }

    void MaterialInstance::SetBlendMode(BlendMode mode) {
        mBlendMode = mode;
    }

    BlendMode MaterialInstance::GetBlendMode() const {
        return mBlendMode;
    }

    void MaterialInstance::SetDepthTest(bool enabled) {
        mDepthTest = enabled;
    }

    bool MaterialInstance::GetDepthTest() const {
        return mDepthTest;
    }

    void MaterialInstance::SetDepthWrite(bool enabled) {
        mDepthWriteOverride = enabled;
    }

    bool MaterialInstance::GetDepthWrite() const {
        return mDepthWriteOverride.value_or(mBlendMode == BlendMode::Opaque);
    }

    void MaterialInstance::SetCullMode(CullMode mode) {
        mCullMode = mode;
    }

    CullMode MaterialInstance::GetCullMode() const {
        return mCullMode;
    }

    const MaterialPropertyBlock& MaterialInstance::GetOverrides() const {
        return mOverrides;
    }

    void MaterialInstance::SetFloat(ShaderPropertyId id, float value) {
        if (Parent && CheckProperty(Parent->GetShader(), id, ShaderPropertyType::Float)) {
            mOverrides.Set(id, value);
        }
    }

    void MaterialInstance::SetVec2(ShaderPropertyId id, const glm::vec2& value) {
        if (Parent && CheckProperty(Parent->GetShader(), id, ShaderPropertyType::Float2)) {
            mOverrides.Set(id, value);
        }
    }

    void MaterialInstance::SetVec3(ShaderPropertyId id, const glm::vec3& value) {
        if (Parent && CheckProperty(Parent->GetShader(), id, ShaderPropertyType::Float3)) {
            mOverrides.Set(id, value);
        }
    }

    void MaterialInstance::SetColor(ShaderPropertyId id, const glm::vec4& value) {
        if (Parent && CheckColorProperty(Parent->GetShader(), id)) {
            mOverrides.Set(id, value);
        }
    }

    void MaterialInstance::SetTexture2D(ShaderPropertyId id, Ref<Texture2D> texture) {
        if (Parent && CheckProperty(Parent->GetShader(), id, ShaderPropertyType::Texture2D)) {
            mOverrides.Set(id, std::move(texture));
        }
    }

    const MaterialPropertyValue* MaterialInstance::Resolve(ShaderPropertyId id) const {
        if (const auto* value = mOverrides.Get(id)) {
            return value;
        }
        return Parent ? Parent->GetProperties().Get(id) : nullptr;
    }


    const MaterialPropertyValue* MaterialInstance::Resolve(const String& name) const {
        return Resolve(MakePropertyId(name));
    }

#pragma endregion


#pragma region Texture2D

    uint32_t Texture2D::GetWidth() const {
        return Width;
    }

    uint32_t Texture2D::GetHeight() const {
        return Height;
    }

    TextureHandle Texture2D::GetHandle() const {
        return Handle;
    }

#pragma endregion


#pragma region Shader


    ShaderHandle Shader::GetHandle(ShaderStage stage) const {
        return stage == ShaderStage::Fragment ? FragmentHandle : VertexHandle;
    }

    const std::vector<uint8_t>& Shader::GetCompiledBinary() const {
        return Binary;
    }

    void Shader::AddProperty(ShaderProperty property) {
        if (property.Id == kInvalidPropertyId) {
            return;
        }

        auto it = mProperties.find(property.Id);
        if (it != mProperties.end() && it->second.Name != property.Name) {
            LOG_ERROR("Shader property hash collision between '{}' and '{}'", it->second.Name, property.Name);
            return;
        }

        mProperties[property.Id] = std::move(property);
    }

    void Shader::AddProperty(const String& name, ShaderPropertyType type, uint32_t binding, ShaderStage stages) {
        ShaderProperty property;
        property.Id      = MakePropertyId(name);
        property.Name    = name;
        property.Type    = type;
        property.Binding = binding;
        property.Stage   = stages;

        AddProperty(std::move(property));
    }

    const ShaderProperty* Shader::FindProperty(ShaderPropertyId id) const {
        auto it = mProperties.find(id);
        return it == mProperties.end() ? nullptr : &it->second;
    }

    const ShaderProperty* Shader::FindProperty(const String& name) const {
        const ShaderProperty* property = FindProperty(MakePropertyId(name));
        return property && property->Name == name ? property : nullptr;
    }

    const std::unordered_map<ShaderPropertyId, ShaderProperty>& Shader::GetProperties() const {
        return mProperties;
    }


#pragma endregion

} // namespace golias
