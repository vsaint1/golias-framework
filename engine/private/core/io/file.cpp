#include "core/io/file.h"

#include "core/io/file_system.h"


namespace golias {


    namespace {

        std::filesystem::path ToPath(const String& path) {
            return std::filesystem::path(path);
        }

    } // namespace

    namespace file {

        bool Exists(const String& path) {
            return std::filesystem::exists(ToPath(path));
        }

        bool IsDirectory(const String& path) {
            return std::filesystem::is_directory(ToPath(path));
        }

        std::optional<std::vector<uint8_t>> ReadAllBytes(const String& path) {
            std::ifstream file(ToPath(path), std::ios::binary | std::ios::ate);
            if (!file.is_open()) {
                return std::nullopt;
            }

            const std::streampos end = file.tellg();
            if (end < 0) {
                return std::nullopt;
            }

            std::vector<uint8_t> buffer(static_cast<size_t>(end));
            file.seekg(0, std::ios::beg);
            if (!buffer.empty() && !file.read(reinterpret_cast<char*>(buffer.data()), static_cast<std::streamsize>(buffer.size()))) {
                return std::nullopt;
            }

            return buffer;
        }

        std::optional<String> ReadAllText(const String& path) {
            std::ifstream file(ToPath(path), std::ios::binary);
            if (!file.is_open()) {
                return std::nullopt;
            }

            std::stringstream buffer;
            buffer << file.rdbuf();
            return buffer.str();
        }

        bool WriteAllBytes(const String& path, const void* data, size_t size) {
            std::ofstream file(ToPath(path), std::ios::binary | std::ios::trunc);
            if (!file.is_open()) {
                return false;
            }

            if (size > 0) {
                file.write(static_cast<const char*>(data), static_cast<std::streamsize>(size));
            }

            return static_cast<bool>(file);
        }

        bool WriteAllText(const String& path, const String& content) {
            return WriteAllBytes(path, content.data(), content.size());
        }

        bool CreateDirectory(const String& path) {
            std::error_code error;
            return std::filesystem::create_directory(ToPath(path), error);
        }

        bool CreateDirectories(const String& path) {
            std::error_code error;
            return std::filesystem::create_directories(ToPath(path), error);
        }

        bool Remove(const String& path) {
            std::error_code error;
            return std::filesystem::remove(ToPath(path), error);
        }

        String GetFileName(const String& path) {
            return ToPath(path).filename().string();
        }

        String GetExtension(const String& path) {
            return ToPath(path).extension().string();
        }

        String GetParentDirectory(const String& path) {
            return ToPath(path).parent_path().string();
        }

        String Combine(const String& lhs, const String& rhs) {
            return (ToPath(lhs) / ToPath(rhs)).string();
        }

        String Extension(const String& path) {
            String extension = std::filesystem::path(path).extension().string();
            std::transform(
                extension.begin(), extension.end(), extension.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
            return extension;
        }


    } // namespace file


} // namespace golias
