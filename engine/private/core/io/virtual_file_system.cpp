#include "core/io/virtual_file_system.h"

#include "core/io/file.h"
#include <cctype>
#include <cstdlib>
#include <filesystem>

namespace golias {

    namespace {

        constexpr const char* kDefaultScheme   = "res";
        constexpr const char* kSchemeSeparator = "://";

        /*!
            @brief Returns the platform user data folder for Golias, creating it if necessary.

            Windows:
            - %APPDATA%/Golias

            macOS:
            - ~/Library/Application Support/Golias

            Linux:
            - $XDG_DATA_HOME/Golias (if $XDG_DATA_HOME is set)
            - ~/.local/share/Golias (if $XDG_DATA_HOME is not set)

            Android:
            - @todo Define the platform-specific user data directory.

            Emscripten:
            - @todo Define the platform-specific user data directory.

            iOS:
            - @todo Define the platform-specific user data directory.

            @return Platform-specific user data directory..
        
        */
        String UserDataPath() {
            std::filesystem::path root;

#if defined(GOLIAS_PLATFORM_WINDOWS)
            if (const char* appData = std::getenv("APPDATA"); appData != nullptr && *appData != '\0') {
                root = std::filesystem::path(appData) / "Golias";
            }

#elif defined(GOLIAS_PLATFORM_OSX)
            if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
                root = std::filesystem::path(home) / "Library" / "Application Support" / "Golias";
            }

#elif defined(GOLIAS_PLATFORM_LINUX)
            if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg != nullptr && *xdg != '\0') {
                root = std::filesystem::path(xdg) / "Golias";
            } else if (const char* home = std::getenv("HOME"); home != nullptr && *home != '\0') {
                root = std::filesystem::path(home) / ".local" / "share" / "Golias";
            }
#endif

            if (root.empty()) {
                root = std::filesystem::temp_directory_path() / "Golias";
            }

            std::error_code error;
            std::filesystem::create_directories(root, error);

            if (error) {
                LOG_ERROR("Failed to create user data folder '{}': {}", root.string(), error.message());
            }

            return root.string();
        }

        // A normalized path is absolute when it starts with `/` (POSIX/UNC) or with a drive letter (`C:/...`).
        bool IsAbsolutePath(const String& path) {
            if (path.empty()) {
                return false;
            }

            if (path.front() == '/') {
                return true;
            }

            return path.size() >= 3 && std::isalpha(static_cast<unsigned char>(path[0])) != 0 && path[1] == ':' && path[2] == '/';
        }

    } // namespace

    VirtualFileSystem::VirtualFileSystem() {
        mMounts[kDefaultScheme] = "res";
        mMounts["user"]         = UserDataPath();
    }

    bool VirtualFileSystem::Mount(const String& scheme, const String& realPath) {
        mMounts[NormalizeScheme(scheme)] = realPath;
        return true;
    }

    bool VirtualFileSystem::Unmount(const String& scheme) {
        const String normalized = NormalizeScheme(scheme);

        // Keep the default `res` scheme around.
        if (normalized == kDefaultScheme) {
            return true;
        }

        return mMounts.erase(normalized) != 0;
    }

    String VirtualFileSystem::NormalizeScheme(const String& scheme) {
        String result = scheme;

        while (!result.empty() && (result.back() == ':' || result.back() == '/')) {
            result.pop_back();
        }

        return result;
    }

    void VirtualFileSystem::SplitScheme(const String& normalizedPath, String& scheme, String& rest) {
        const size_t separator = normalizedPath.find(kSchemeSeparator);

        if (separator == String::npos) {
            scheme = kDefaultScheme;
            rest   = normalizedPath;
        } else {
            scheme = normalizedPath.substr(0, separator);
            rest   = normalizedPath.substr(separator + 3);
        }

        const size_t start = rest.find_first_not_of('/');
        rest               = start == String::npos ? String() : rest.substr(start);
    }

    String VirtualFileSystem::Resolve(const String& virtualPath) const {
        String normalized = virtualPath;

        for (char& c : normalized) {
            if (c == '\\') {
                c = '/';
            }
        }

        if (IsAbsolutePath(normalized)) {
            return normalized;
        }

        String scheme;
        String rest;

        SplitScheme(normalized, scheme, rest);

        auto mount = mMounts.find(scheme);
        if (mount == mMounts.end()) {
            LOG_ERROR("VirtualFileSystem: unknown scheme '{}://' for '{}'; falling back to '{}'.", scheme, virtualPath, kDefaultScheme);
            mount = mMounts.find(kDefaultScheme);
        }

        if (mount == mMounts.end()) {
            return rest;
        }

        return rest.empty() ? mount->second : file::Combine(mount->second, rest);
    }

    bool VirtualFileSystem::Exists(const String& virtualPath) const {
        return file::Exists(Resolve(virtualPath));
    }

    bool VirtualFileSystem::IsDirectory(const String& virtualPath) const {
        return file::IsDirectory(Resolve(virtualPath));
    }

    std::optional<std::vector<uint8_t>> VirtualFileSystem::ReadAllBytes(const String& virtualPath) const {
        return file::ReadAllBytes(Resolve(virtualPath));
    }

    std::optional<String> VirtualFileSystem::ReadAllText(const String& virtualPath) const {
        return file::ReadAllText(Resolve(virtualPath));
    }

    bool VirtualFileSystem::WriteAllBytes(const String& virtualPath, const void* data, size_t size) const {
        return file::WriteAllBytes(Resolve(virtualPath), data, size);
    }

    bool VirtualFileSystem::WriteAllText(const String& virtualPath, const String& content) const {
        return file::WriteAllText(Resolve(virtualPath), content);
    }

    FileSystem& FileSystem::GetInstance() {
        static VirtualFileSystem instance;
        return instance;
    }

} // namespace golias
