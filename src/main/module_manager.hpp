/*
 * module_manager.hpp
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

#ifndef MODULE_MANAGER_HPP
#define MODULE_MANAGER_HPP

#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class VFS;
struct GameModuleSpec;
struct ModuleManagerImpl;

class ModuleManager {
public:
    explicit ModuleManager(std::filesystem::path modules_path, std::string build_id = "");
    ~ModuleManager();

    // Re-read available modules and their checksums from disk
    void update();

    // Determine if a module is installed/available locally
    bool isModuleInstalled(const std::string &module_name) const;

    // Get the user's currently enabled module list (for creating new games)
    std::vector<std::string> getEnabledModules() const;

    // "Resolve" a module list, meaning: add any required dependencies, and sort into
    // dependency order
    std::vector<std::string> resolveModuleList(const std::vector<std::string> &modules) const;

    // Get a VFS corresponding to a list of modules
    VFS getVFS(const std::vector<std::string> &modules) const;

    // Get the checksum for a game that uses a particular (resolved) list of modules
    uint64_t computeCombinedChecksum(const std::vector<std::string> &modules) const;

    // Check if a game is compatible with ours.
    //  - If returns false and missing_modules_out is empty, that means a checksum mismatch.
    //  - If returns false and missing_modules_out is non-empty, those modules are loaded on the
    //    remote server but not installed locally.
    //  - If returns true, the game is compatible.
    bool isCompatible(const GameModuleSpec &other_spec,
                      std::vector<std::string> &missing_modules_out) const;

private:
    std::unique_ptr<ModuleManagerImpl> pimpl;
};

#endif
