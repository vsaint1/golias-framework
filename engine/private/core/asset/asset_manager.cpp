#include "core/asset/asset_manager.h"

#include "core/io/file_system.h"
#include "graphics/render_resources.h"
#include "graphics/rhi/rhi_device.h"
#include "stb_image.h"
#include "stdafx.h"
#include "yaml-cpp/yaml.h"

namespace golias {

    RHIDevice* AssetManager::sDevice = nullptr;
    std::unordered_map<String, Ref<Asset>> AssetManager::sCache;
    std::unordered_map<UUID, Ref<Asset>> AssetManager::sCacheByUUID;
    std::unordered_map<UUID, String> AssetManager::sUUIDToPath;

    namespace {

        String MetadataPath(const String& virtualPath) {
            return virtualPath + ".meta";
        }

        bool ReadUUID(const String& metadataPath, UUID& uuid) {
            const auto metadata = VFS::ReadText(metadataPath);
            if (!metadata) {
                return false;
            }


            const YAML::Node document = YAML::Load(*metadata);
            if (!document["guid"] || !document["guid"].IsScalar()) {
                return false;
            }

            uuid = document["guid"].as<String>();
            if (!IsValid_UUID(uuid)) {
                LOG_WARN("Invalid UUID '{}' in metadata '{}'.", uuid, metadataPath);
                return false;
            }

            return true;
        }

        bool WriteUUID(const String& metadataPath, const UUID& uuid) {
            YAML::Emitter emitter;
            emitter << YAML::BeginMap << YAML::Key << "guid" << YAML::Value << uuid << YAML::EndMap;
            return VFS::WriteText(metadataPath, emitter.c_str());
        }

    } // namespace

    void AssetManager::Initialize(RHIDevice& device) {
        sDevice = &device;
        sCache.clear();
        sCacheByUUID.clear();
        sUUIDToPath.clear();
        
        IndexMetadata();
    }

    void AssetManager::Shutdown() {
        sCache.clear();
        sCacheByUUID.clear();
        sUUIDToPath.clear();
        sDevice = nullptr;
    }

    UUID AssetManager::LoadOrCreateUUID(const String& virtualPath) {
        const String metadataPath = MetadataPath(virtualPath);
        UUID uuid;
        if (ReadUUID(metadataPath, uuid)) {
            sUUIDToPath[uuid] = virtualPath;
            return uuid;
        }

        uuid = Generate_UUID();
        if (!WriteUUID(metadataPath, uuid)) {
            LOG_WARN("Failed to write metadata '{}'.", metadataPath);
        } else {
            LOG_DEBUG("Assigned UUID '{}' to '{}'.", uuid, virtualPath);
        }

        sUUIDToPath[uuid] = virtualPath;
        return uuid;
    }

    void AssetManager::IndexMetadata() {
        for (const String& metadataPath : VFS::ListFiles("res://", true)) {
            if (metadataPath.size() < 5 || metadataPath.substr(metadataPath.size() - 5) != ".meta") {
                continue;
            }

            UUID uuid;
            if (ReadUUID(metadataPath, uuid)) {
                sUUIDToPath[uuid] = metadataPath.substr(0, metadataPath.size() - 5);
            }
        }

        LOG_DEBUG("Indexed {} asset UUIDs.", sUUIDToPath.size());
    }

    void AssetManager::RegisterAsset(const String& virtualPath, const UUID& uuid, const Ref<Asset>& asset) {
        asset->SetPath(virtualPath);
        asset->SetUUID(uuid);
        sCache[virtualPath] = asset;
        sCacheByUUID[uuid]  = asset;
        sUUIDToPath[uuid]   = virtualPath;
    }

    bool AssetManager::Exists(const String& virtualPath) {
        return VFS::Exists(virtualPath);
    }

    template <>
    Ref<Texture2D> AssetManager::Load<Texture2D>(const String& virtualPath) {
        auto it = sCache.find(virtualPath);
        if (it != sCache.end()) {
            return static_pointer_cast<Texture2D>(it->second);
        }

        if (!VFS::Exists(virtualPath)) {
            LOG_ERROR("Asset does not exist '{}'.", virtualPath);
            return nullptr;
        }

        const UUID uuid = LoadOrCreateUUID(virtualPath);
        if (auto uuidIt = sCacheByUUID.find(uuid); uuidIt != sCacheByUUID.end()) {
            sCache[virtualPath] = uuidIt->second;
            return static_pointer_cast<Texture2D>(uuidIt->second);
        }

        auto data = VFS::ReadBinary(virtualPath);
        if (!data || data->empty()) {
            LOG_ERROR("Failed to read texture '{}'.", virtualPath);
            return nullptr;
        }

        int width             = 0;
        int height            = 0;
        int channels          = 0;
        unsigned char* pixels = stbi_load_from_memory(data->data(), static_cast<int>(data->size()), &width, &height, &channels, 4);
        if (!pixels) {
            LOG_ERROR("Failed to decode texture '{}': {}", virtualPath, stbi_failure_reason());
            return nullptr;
        }

        TextureDesc desc;
        desc.width  = static_cast<uint32_t>(width);
        desc.height = static_cast<uint32_t>(height);
        desc.format = TextureFormat::R8G8B8A8_UNORM;
        desc.usage  = TextureUsage::Sampler;

        auto texture    = std::make_shared<Texture2D>();
        texture->Handle = sDevice->CreateTexture(desc);
        texture->Width  = desc.width;
        texture->Height = desc.height;
        sDevice->UploadToTexture(texture->Handle, pixels, desc.width, desc.height, 0);
        stbi_image_free(pixels);

        RegisterAsset(virtualPath, uuid, texture);
        return texture;
    }

    template <>
    Ref<Shader> AssetManager::Load<Shader>(const String& virtualPath) {
        auto it = sCache.find(virtualPath);
        if (it != sCache.end()) {
            return static_pointer_cast<Shader>(it->second);
        }

        if (!VFS::Exists(virtualPath)) {
            LOG_ERROR("Asset does not exist '{}'.", virtualPath);
            return nullptr;
        }

        const UUID uuid = LoadOrCreateUUID(virtualPath);
        if (auto uuidIt = sCacheByUUID.find(uuid); uuidIt != sCacheByUUID.end()) {
            sCache[virtualPath] = uuidIt->second;
            return static_pointer_cast<Shader>(uuidIt->second);
        }

        auto data = VFS::ReadBinary(virtualPath);
        if (!data || data->empty()) {
            LOG_ERROR("Failed to read shader '{}'.", virtualPath);
            return nullptr;
        }

        auto shader    = std::make_shared<Shader>();
        shader->Binary = std::move(*data);
        RegisterAsset(virtualPath, uuid, shader);
        return shader;
    }

} // namespace golias
