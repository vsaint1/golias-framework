#pragma once
#include "stdafx.h"

namespace golias {


    // Abstract virtual file system.
    //     res://internal/shaders/vulkan/test.spv   -> <res>/internal/shaders/vulkan/test.spv
    //     user://preferences/settings.cfg          -> <user data dir>/preferences/settings.cfg
    class FileSystem {
    public:
        virtual ~FileSystem() = default;

        // Maps the `scheme` (e.g. "res" or "user") onto `realPath`.
        virtual bool Mount(const String& scheme, const String& realPath) = 0;

        // Removes a previously mounted scheme.
        virtual bool Unmount(const String& scheme) = 0;

        virtual bool Exists(const String& virtualPath) const = 0;

        virtual std::vector<String> ListFiles(const String& virtualDirectory, bool recursive = true) const = 0;

        virtual bool IsDirectory(const String& virtualPath) const = 0;

        // Reads a whole file as raw bytes. Returns nullopt when the file cannot be opened or read.
        virtual std::optional<std::vector<uint8_t>> ReadAllBytes(const String& virtualPath) const = 0;

        // Reads a whole text file. Returns nullopt when the file cannot be opened or read.
        virtual std::optional<String> ReadAllText(const String& virtualPath) const = 0;

        virtual bool WriteAllBytes(const String& virtualPath, const void* data, size_t size) const = 0;

        virtual bool WriteAllText(const String& virtualPath, const String& content) const = 0;

        // Maps a virtual path onto its real path on disk.
        virtual String Resolve(const String& virtualPath) const = 0;

        // Engine-wide file system. A VirtualFileSystem with `res://` and
        // `user://` mounted by default.
        static FileSystem& GetInstance();
    };

    namespace VFS {
        inline bool Exists(const String& virtualPath) {
            return FileSystem::GetInstance().Exists(virtualPath);
        }

        inline std::vector<String> ListFiles(const String& virtualDirectory, bool recursive = true) {
            return FileSystem::GetInstance().ListFiles(virtualDirectory, recursive);
        }

        inline std::optional<std::vector<uint8_t>> ReadBinary(const String& virtualPath) {
            return FileSystem::GetInstance().ReadAllBytes(virtualPath);
        }

        inline std::optional<String> ReadText(const String& virtualPath) {
            return FileSystem::GetInstance().ReadAllText(virtualPath);
        }

        inline bool WriteText(const String& virtualPath, const String& content) {
            return FileSystem::GetInstance().WriteAllText(virtualPath, content);
        }

        inline String PhysicalDirectory(const String& virtualPath) {
            return std::filesystem::path(FileSystem::GetInstance().Resolve(virtualPath)).parent_path().string();
        }

    } // namespace VFS

} // namespace golias
