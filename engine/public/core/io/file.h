#pragma once
#include "stdafx.h"

namespace golias {

    namespace file {

        // Returns true if `path` exists, false if it does not exist.
        bool Exists(const String& path);

        // Returns true if `path` exists and is a directory, false if it does not exist or is a file.
        bool IsDirectory(const String& path);

        // Reads a whole file as raw bytes. Returns nullopt when the file cannot be opened or read.
        std::optional<std::vector<uint8_t>> ReadAllBytes(const String& path);

        // Reads a whole text file. Returns nullopt when the file cannot be opened or read.
        std::optional<String> ReadAllText(const String& path);

        // Writes a whole file as raw bytes. Returns false when the file cannot be opened or written.
        bool WriteAllBytes(const String& path, const void* data, size_t size);

        // Writes a whole text file. Returns false when the file cannot be opened or written.
        bool WriteAllText(const String& path, const String& content);

        // Creates a single directory.
        bool CreateDirectory(const String& path);

        // Creates every missing directory along `path` (like `mkdir -p`).
        bool CreateDirectories(const String& path);

        // Removes a file or an empty directory.
        bool Remove(const String& path);

        // Returns the filename (last path segment) of `path`.
        String GetFileName(const String& path);

        // Returns the file extension (last path segment) of `path`.
        String GetExtension(const String& path);

        // Returns the parent directory of `path`.
        String GetParentDirectory(const String& path);

        // Joins two path segments with the platform separator.
        String Combine(const String& lhs, const String& rhs);

        // Returns the file extension (last path segment) of `path`.
        String Extension(const String& path);

    }; // namespace file

} // namespace golias
