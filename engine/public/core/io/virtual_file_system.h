#pragma once
#include "core/io/file_system.h"

namespace golias {

    // Virtual file system with scheme mounts. Paths use a `scheme://` prefix:
    //
    //     res://internal/shaders/vulkan/test.spv -> <res>/internal/shaders/vulkan/test.spv
    //     user://preferences/settings.cfg        -> <user data dir>/preferences/settings.cfg
    //
    // The engine registers two mounts by default:
    //   - `res`  -> the `res/` content folder (relative to the working directory)
    //   - `user` -> the platform user data folder (preferences, saves, ...)
    //
    // Scheme-less paths default to `res://`.
    class VirtualFileSystem : public FileSystem {
    public:
        VirtualFileSystem();

        bool Mount(const String& scheme, const String& realPath) override;

        bool Unmount(const String& scheme) override;

        bool Exists(const String& virtualPath) const override;

        std::vector<String> ListFiles(const String& virtualDirectory, bool recursive = true) const override;

        bool IsDirectory(const String& virtualPath) const override;

        std::optional<std::vector<uint8_t>> ReadAllBytes(const String& virtualPath) const override;

        std::optional<String> ReadAllText(const String& virtualPath) const override;

        bool WriteAllBytes(const String& virtualPath, const void* data, size_t size) const override;

        bool WriteAllText(const String& virtualPath, const String& content) const override;

        String Resolve(const String& virtualPath) const override;

    private:
        // Splits `scheme://rest`. Scheme-less paths get the default `res`
        // scheme; leading separators are stripped from `rest`.
        static void SplitScheme(const String& normalizedPath, String& scheme, String& rest);

        // Trims trailing `:`/`/` from a scheme written by the caller.
        static String NormalizeScheme(const String& scheme);

        std::unordered_map<String, String> mMounts;
    };

} // namespace golias
