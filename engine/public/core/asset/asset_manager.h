#pragma once

#include "core/asset/asset.h"
#include "stdafx.h"

namespace golias {

    class RHIDevice;

    class AssetManager {
    public:
        static void Initialize(RHIDevice& device);
        static void Shutdown();

        template <typename T>
        static Ref<T> Load(const String& virtualPath);

        template <typename T>
        static Ref<T> LoadByUUID(const UUID& uuid) {
            auto it = sCacheByUUID.find(uuid);
            if (it != sCacheByUUID.end()) {
                return static_pointer_cast<T>(it->second);
            }

            auto pathIt = sUUIDToPath.find(uuid);
            return pathIt == sUUIDToPath.end() ? nullptr : Load<T>(pathIt->second);
        }

        static bool Exists(const String& virtualPath);

        static RHIDevice* GetDevice() {
            return sDevice;
        }

    private:
        static RHIDevice* sDevice;
        static std::unordered_map<String, Ref<Asset>> sCache;
        static std::unordered_map<UUID, Ref<Asset>> sCacheByUUID;
        static std::unordered_map<UUID, String> sUUIDToPath;

        static UUID LoadOrCreateUUID(const String& virtualPath);

        static void IndexMetadata();
        
        static void RegisterAsset(const String& virtualPath, const UUID& uuid, const Ref<Asset>& asset);
    };

} // namespace golias
