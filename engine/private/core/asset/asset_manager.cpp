#include "core/asset/asset_manager.h"

#include "core/io/file_system.h"
#include "graphics/model_importer.h"
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

        constexpr uint32_t kModelVertexStride = 8;

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

        Ref<Texture2D> CreateImportedTexture(const ImportedTexture& source) {
            RHIDevice* device = AssetManager::GetDevice();
            if (!device || source.Data.empty()) {
                return nullptr;
            }

            int width = static_cast<int>(source.Width);
            int height = static_cast<int>(source.Height);
            unsigned char* pixels = nullptr;
            if (source.IsRawRGBA) {
                pixels = const_cast<unsigned char*>(source.Data.data());
            } else {
                int channels = 0;
                pixels = stbi_load_from_memory(source.Data.data(), static_cast<int>(source.Data.size()), &width, &height, &channels, 4);
            }

            if (!pixels || width <= 0 || height <= 0) {
                LOG_WARN("Failed to decode imported texture '{}'.", source.Name);
                return nullptr;
            }

            TextureDesc desc;
            desc.width = static_cast<uint32_t>(width);
            desc.height = static_cast<uint32_t>(height);
            desc.format = TextureFormat::R8G8B8A8_UNORM;
            desc.usage = TextureUsage::Sampler;

            auto texture = std::make_shared<Texture2D>();
            texture->Handle = device->CreateTexture(desc);
            texture->Width = desc.width;
            texture->Height = desc.height;
            device->UploadToTexture(texture->Handle, pixels, desc.width, desc.height, 0);
         
            if (!source.IsRawRGBA) {
                stbi_image_free(pixels);
            }

            return texture;
        }

        Ref<Mesh> CreateImportedMesh(const ImportedMesh& source) {
            RHIDevice* device = AssetManager::GetDevice();
            if (!device || source.Vertices.empty() || source.Indices.empty()) {
                return nullptr;
            }

            auto mesh = std::make_shared<Mesh>();
            mesh->Vertices = source.Vertices;
            mesh->IndicesData = source.Indices;
            mesh->VertexBuffers.push_back({0, kModelVertexStride * sizeof(float), 0, false});
            mesh->VertexAttributes = {
                {0, 0, 0, VertexElementFormat::Float3},
                {1, 0, 12, VertexElementFormat::Float3},
                {2, 0, 24, VertexElementFormat::Float2},
            };
            mesh->VertexBuffer = device->CreateBuffer({.usage = BufferUsage::Vertex, .size = static_cast<uint32_t>(mesh->Vertices.size() * sizeof(float))});
            mesh->IndexBuffer = device->CreateBuffer({.usage = BufferUsage::Index, .size = static_cast<uint32_t>(mesh->IndicesData.size() * sizeof(uint32_t))});
            device->UploadToBuffer(mesh->VertexBuffer, mesh->Vertices.data(), static_cast<uint32_t>(mesh->Vertices.size() * sizeof(float)), 0);
            device->UploadToBuffer(mesh->IndexBuffer, mesh->IndicesData.data(), static_cast<uint32_t>(mesh->IndicesData.size() * sizeof(uint32_t)), 0);
            mesh->Submeshes.push_back({0, static_cast<uint32_t>(mesh->IndicesData.size())});
            return mesh;
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
    Ref<Model> AssetManager::Load<Model>(const String& virtualPath) {
        auto it = sCache.find(virtualPath);
        if (it != sCache.end()) {
            return static_pointer_cast<Model>(it->second);
        }

        if (!VFS::Exists(virtualPath)) {
            LOG_ERROR("Asset does not exist '{}'.", virtualPath);
            return nullptr;
        }

        const UUID uuid = LoadOrCreateUUID(virtualPath);
        if (auto uuidIt = sCacheByUUID.find(uuid); uuidIt != sCacheByUUID.end()) {
            sCache[virtualPath] = uuidIt->second;
            return static_pointer_cast<Model>(uuidIt->second);
        }

        ImportedModel imported;
        String error;
        if (!ModelImporter::ImportFile(virtualPath, imported, error)) {
            LOG_ERROR("{}", error);
            return nullptr;
        }

        std::vector<Ref<Texture2D>> textures;
        textures.reserve(imported.Textures.size());
        for (const ImportedTexture& source : imported.Textures) {
            textures.push_back(CreateImportedTexture(source));
        }

        std::vector<Ref<Material>> materials;
        materials.reserve(imported.Materials.size());
        for (const ImportedMaterial& source : imported.Materials) {
            auto material = std::make_shared<Material>();
            material->SetColor("BaseColor", source.BaseColor);
            if (source.BaseColorTexture >= 0 && source.BaseColorTexture < static_cast<int32_t>(textures.size())) {
                material->SetTexture("BaseMap", textures[source.BaseColorTexture]);
            }

            materials.push_back(std::move(material));
        }

        auto model = std::make_shared<Model>();
        model->Textures = textures;
        model->Parts.reserve(imported.Meshes.size());
        std::vector<int32_t> meshToPart(imported.Meshes.size(), -1);
        for (size_t meshIndex = 0; meshIndex < imported.Meshes.size(); ++meshIndex) {
            const ImportedMesh& source = imported.Meshes[meshIndex];
            Ref<Mesh> mesh = CreateImportedMesh(source);
            if (!mesh) {
                continue;
            }

            Ref<Material> material = std::make_shared<Material>();
            if (source.MaterialIndex >= 0 && source.MaterialIndex < static_cast<int32_t>(materials.size())) {
                material = materials[source.MaterialIndex];
            } else {
                material->SetColor("BaseColor", glm::vec4(1.0f));
            }
            meshToPart[meshIndex] = static_cast<int32_t>(model->Parts.size());
            model->Parts.push_back({std::move(mesh), std::move(material)});
        }

        model->Nodes.reserve(imported.Nodes.size());
        for (const ImportedNode& source : imported.Nodes) {
            ModelNode node;
            node.Name = source.Name;
            node.ParentIndex = source.ParentIndex;
            node.Position = source.Position;
            node.Rotation = source.Rotation;
            node.Scale = source.Scale;
          
            for (uint32_t meshIndex : source.MeshIndices) {
                if (meshIndex < meshToPart.size() && meshToPart[meshIndex] >= 0) {
                    node.PartIndices.push_back(static_cast<uint32_t>(meshToPart[meshIndex]));
                }
            }

            model->Nodes.push_back(std::move(node));
        }

        if (model->Parts.empty()) {
            LOG_ERROR("Model '{}' contains no supported triangle meshes.", virtualPath);
            return nullptr;
        }

        RegisterAsset(virtualPath, uuid, model);
        return model;
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
