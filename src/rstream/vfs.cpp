/*
 * vfs.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "vfs.hpp"
#include "rstream_error.hpp"

#ifdef VIRTUAL_SERVER
#include "syscalls.hpp"
#else
#include "xxhash.hpp"
#include <algorithm>
#endif

#include <vector>

std::string VFS::normalizePath(const char *vfs_path)
{
    std::vector<std::string> components;  // List of path components found
    std::string comp;  // Current path component being built
    const char *p = vfs_path;
    while (true) {
        if (*p == ':') {
            // Colons are not allowed
            throw RStreamError(vfs_path, "invalid path");
        } else if (*p == '/' || *p == '\\' || *p == 0) {
            // Path component separator character, or end of string
            if (comp == "" || comp == ".") {
                // Empty or "." path components are just dropped
            } else if (comp == "..") {
                // ".." means move back up a directory
                if (components.empty()) {
                    // Attempt to break out above the root directory!
                    throw RStreamError(vfs_path, "invalid path");
                }
                components.pop_back();
            } else {
                // This is a normal path component, add it to the list.
                components.push_back(comp);
            }
            if (*p == 0) break;   // Done!
            comp.clear();  // Not done, clear 'comp' for next component
        } else {
            // Normal character - part of the current path component
            comp += *p;
        }
        ++p;  // Move on to next character
    }

    // Path is valid; construct the normalized version
    std::string output;
    for (std::vector<std::string>::size_type i = 0; i < components.size(); ++i) {
        if (i != 0) {
            output += '/';
        }
        output += components[i];
    }
    return output;
}

std::string VFS::normalizePath(const std::string &vfs_path)
{
    return normalizePath(vfs_path.c_str());
}

std::ifstream VFS::open(const char *vfs_path) const
{
    std::string normalized = normalizePath(vfs_path);
    if (normalized.empty()) throw RStreamError(vfs_path, "invalid path");

#ifdef VIRTUAL_SERVER
    std::ifstream str(("VFS:" + normalized).c_str(), std::ios::binary);
#else
    std::ifstream str(map(normalized), std::ios::binary);
#endif

    if (!str) throw RStreamError(vfs_path, "could not open file");
    return str;
}

std::ifstream VFS::open(const std::string &vfs_path) const
{
    return open(vfs_path.c_str());
}

bool VFS::exists(const char *vfs_path) const
{
    std::string normalized = normalizePath(vfs_path);
    if (normalized.empty()) throw RStreamError(vfs_path, "invalid path");

#ifdef VIRTUAL_SERVER
    std::ifstream str(("VFS:" + normalized).c_str(), std::ios::binary);
    return bool(str);
#else
    return std::filesystem::exists(map(normalized));
#endif
}

bool VFS::exists(const std::string &vfs_path) const
{
    return exists(vfs_path.c_str());
}

#ifndef VIRTUAL_SERVER
void VFS::add(std::filesystem::path local_path, std::string vfs_path)
{
    if (vfs_path == "." || vfs_path == ".."
            || vfs_path.find_first_of("/\\:") != std::string::npos) {
        throw RStreamError(vfs_path, "invalid mount point name");
    }
    mappings[vfs_path] = std::move(local_path);
    enabled.insert(std::move(vfs_path));
}

void VFS::remove(const std::string &vfs_path)
{
    mappings.erase(vfs_path);
    enabled.erase(vfs_path);
}
#endif

void VFS::disableAll()
{
#ifdef VIRTUAL_SERVER
    vs_vfs_disable_all();
#else
    enabled.clear();
#endif
}

void VFS::enable(const std::string &vfs_path)
{
#ifdef VIRTUAL_SERVER
    vs_vfs_enable(vfs_path.c_str());
#else
    enabled.insert(vfs_path);
#endif
}

#ifndef VIRTUAL_SERVER
void VFS::enableAll()
{
    for (const auto &kv : mappings) {
        enabled.insert(kv.first);
    }
}

void VFS::disable(const std::string &vfs_path)
{
    enabled.erase(vfs_path);
}

std::filesystem::path VFS::map(const std::string &normalized) const
{
    // Split off the first path component
    auto slash_pos = normalized.find('/');
    std::string first_component = (slash_pos == std::string::npos) ? normalized : normalized.substr(0, slash_pos);
    std::string tail = (slash_pos == std::string::npos) ? "" : normalized.substr(slash_pos + 1);

    // Check if the first component matches an enabled mount point
    if (!first_component.empty()) {
        auto it = mappings.find(first_component);
        if (it != mappings.end() && enabled.count(first_component)) {
            return tail.empty() ? it->second : it->second / tail;
        }
    }

    // Fall back to the root mount point ("")
    auto it = mappings.find("");
    if (it != mappings.end() && enabled.count("")) {
        return normalized.empty() ? it->second : it->second / normalized;
    }

    throw RStreamError(normalized, "file not found");
}

void VFS::hashDirectory(const std::string &vfs_path, XXHash &hasher) const
{
    // Normalize and validate path
    std::string normalized = normalizePath(vfs_path);
    if (normalized.empty()) {
        throw RStreamError(vfs_path, "invalid path");
    }

    // Resolve to filesystem path
    std::filesystem::path dir_path = map(normalized);

    // Verify directory exists
    if (!std::filesystem::exists(dir_path)) {
        throw RStreamError(vfs_path, "directory not found");
    }
    if (!std::filesystem::is_directory(dir_path)) {
        throw RStreamError(vfs_path, "not a directory");
    }

    // Collect all regular files with normalized paths (following symlinks)
    std::vector<std::pair<std::filesystem::path, std::string>> file_list;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(
            dir_path, std::filesystem::directory_options::follow_directory_symlink)) {
        if (entry.is_regular_file()) {
            // Get relative path with forward slashes for cross-platform determinism
            std::string rel_path_str = entry.path().lexically_relative(dir_path).generic_string();
            file_list.push_back({entry.path(), rel_path_str});
        }
    }

    // Sort by normalized path string for deterministic ordering
    std::sort(file_list.begin(), file_list.end(),
        [](const auto& a, const auto& b) { return a.second < b.second; });

    // Process each file
    for (const auto& file_entry : file_list) {
        const std::filesystem::path& filepath = file_entry.first;
        const std::string& rel_path_str = file_entry.second;

        // Hash filename
        hasher.updateHashPartial(
            reinterpret_cast<const uint8_t*>(rel_path_str.data()),
            rel_path_str.size()
        );

        // Hash file size as little-endian bytes for portability
        uint64_t file_size = std::filesystem::file_size(filepath);
        uint8_t size_bytes[8];
        for (int i = 0; i < 8; i++) {
            size_bytes[i] = static_cast<uint8_t>((file_size >> (i * 8)) & 0xFF);
        }
        hasher.updateHashPartial(size_bytes, 8);

        // Hash file contents
        std::ifstream file(filepath, std::ios::binary);
        if (!file) {
            throw RStreamError(vfs_path, "failed to read file");
        }

        // Read in 32-byte chunks (use fast path for aligned data)
        uint64_t buffer[4];  // 32 bytes
        while (file.read(reinterpret_cast<char*>(buffer), 32)) {
            hasher.updateHash(buffer);
        }

        // Check for read errors (not just EOF)
        if (file.bad()) {
            throw RStreamError(vfs_path, "I/O error reading file");
        }

        // Handle remaining bytes (0-31) with padding
        std::streamsize remaining = file.gcount();
        if (remaining > 0) {
            hasher.updateHashPartial(
                reinterpret_cast<const uint8_t*>(buffer),
                static_cast<size_t>(remaining)
            );
        }
    }
}

#endif
