#include "graphics/render_resources.h"

namespace golias {

    TextureHandle Texture2D::GetHandle() const {
        return Handle;
    }

    uint32_t Texture2D::GetWidth() const {
        return Width;
    }

    uint32_t Texture2D::GetHeight() const {
        return Height;
    }

    ShaderHandle Shader::GetHandle() const {
        return Handle;
    }

    const std::vector<uint8_t>& Shader::GetCompiledBinary() const {
        return Binary;
    }

    void Material::DefineTextureProperty(RHIDevice& device, const String& name, uint32_t slot, bool vertexStage) {
        if (!mDefaultSampler) {
            mDefaultSampler = device.CreateSampler({});
        }

        mPropertyBindings[MakePropertyId(name)] = {slot, vertexStage};
    }

    void Material::DefineTextureProperty(RHIDevice& device, const char* name, uint32_t slot, bool vertexStage) {
        DefineTextureProperty(device, String(name), slot, vertexStage);
    }

    const TextureProperty* Material::GetPropertyBinding(ShaderPropertyId id) const {
        auto it = mPropertyBindings.find(id);
        return it != mPropertyBindings.end() ? &it->second : nullptr;
    }

    const TextureProperty* Material::GetPropertyBinding(const String& name) const {
        return GetPropertyBinding(MakePropertyId(name));
    }

    SamplerHandle Material::GetDefaultSampler() const {
        return mDefaultSampler;
    }

    void MaterialInstance::SetTexture(const String& name, TextureHandle texture) {
        if (!Parent) {
            return;
        }

        const ShaderPropertyId id = MakePropertyId(name);
        const TextureProperty* property = Parent->GetPropertyBinding(id);
        if (!property) {
            LOG_ERROR("MaterialInstance: unknown texture property '{}'", name);
            return;
        }

        const SamplerHandle sampler = Parent->GetDefaultSampler();
        for (auto& entry : mTextures) {
            if (entry.PropertyId == id) {
                entry.Texture = texture;
                entry.Sampler = sampler;
                return;
            }
        }

        mTextures.push_back({id, property->slot, texture, sampler, property->vertexStage});
    }

    void MaterialInstance::SetTexture2D(const char* name, Ref<Texture2D> texture) {
        SetTexture(String(name), texture->GetHandle());
    }

    const std::vector<BoundTexture>& MaterialInstance::GetTextures() const {
        return mTextures;
    }

    bool MaterialInstance::HasTextures() const {
        return !mTextures.empty();
    }

} // namespace golias
