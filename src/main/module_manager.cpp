/*
 * module_manager.cpp
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

#include "misc.hpp"

#include "compute_checksum.hpp"
#include "game_module_spec.hpp"
#include "module_manager.hpp"
#include "read_module_names.hpp"
#include "version.hpp"
#include "vfs.hpp"
#include "xxhash.hpp"

#include <algorithm>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

struct ModuleInfo {
    std::string name;  // VFS name (mount point)
    std::filesystem::path path;  // Path on local filesystem
    uint64_t checksum;
};

struct ModuleManagerImpl {
    std::filesystem::path modules_path;
    std::string build_id;
    std::vector<ModuleInfo> modules;
    std::unordered_map<std::string, size_t> index;
    std::vector<std::string> enabled_modules;
};

ModuleManager::ModuleManager(std::filesystem::path modules_path, std::string build_id)
    : pimpl(std::make_unique<ModuleManagerImpl>())
{
    pimpl->modules_path = std::move(modules_path);
    pimpl->build_id = std::move(build_id);

    // Do an initial update so that we are ready to go from the start
    update();
}

ModuleManager::~ModuleManager() = default;

void ModuleManager::update()
{
    VFS root_vfs;
    root_vfs.add(pimpl->modules_path, "");

    // Load modules.txt: this is the user's enabled-module list, not the full install list.
    std::vector<std::string> enabled_names = ReadModuleNames(root_vfs, "modules.txt");

    // Discover all installed modules by scanning subdirectories of modules_path.
    std::vector<std::string> names;
    for (const auto &entry : std::filesystem::directory_iterator(pimpl->modules_path)) {
        if (!entry.is_directory()) continue;
        std::string name = entry.path().filename().string();
        if (IsValidModuleName(name)) names.push_back(name);
    }
    std::sort(names.begin(), names.end());

    std::unordered_set<std::string> known(names.begin(), names.end());

    // Validate that every name in modules.txt corresponds to a discovered directory.
    for (const std::string &name : enabled_names) {
        if (!known.count(name)) {
            throw std::runtime_error(
                "modules.txt lists '" + name + "', but no such module directory was found");
        }
    }

    std::vector<ModuleInfo> new_modules;

    for (const std::string &name : names) {
        ModuleInfo info;
        info.name = name;
        info.path = pimpl->modules_path / name;
        info.checksum = ComputeLocalChecksum(info.path);
        new_modules.push_back(std::move(info));
    }

    pimpl->modules = std::move(new_modules);
    pimpl->index.clear();
    for (size_t i = 0; i < pimpl->modules.size(); ++i) {
        pimpl->index[pimpl->modules[i].name] = i;
    }
    pimpl->enabled_modules = std::move(enabled_names);
}

bool ModuleManager::isModuleInstalled(const std::string &module_name) const
{
    return pimpl->index.count(module_name) != 0;
}

std::vector<std::string> ModuleManager::getEnabledModules() const
{
    return pimpl->enabled_modules;
}

std::vector<std::string> ModuleManager::resolveModuleList(
    const std::vector<std::string> &modules) const
{
    std::vector<std::string> result;
    std::unordered_set<std::string> seen;
    for (const auto &name : modules) {
        if (!pimpl->index.count(name))
            throw std::runtime_error("Unknown module: '" + name + "'");
        if (seen.insert(name).second) result.push_back(name);
    }
    return result;
}

VFS ModuleManager::getVFS(const std::vector<std::string> &modules) const
{
    VFS result;
    for (const std::string &name : modules) {
        auto it = pimpl->index.find(name);
        if (it != pimpl->index.end()) {
            result.add(pimpl->modules[it->second].path, name);
        }
    }
    return result;
}

uint64_t ModuleManager::computeCombinedChecksum(const std::vector<std::string> &modules) const
{
    XXHash hasher(0);
    uint64_t lane[4] = {};
    size_t lane_pos = 0;

    for (const std::string &name : modules) {
        auto it = pimpl->index.find(name);
        if (it == pimpl->index.end()) {
            throw std::runtime_error(
                "Unknown module in computeCombinedChecksum: '" + name + "'");
        }
        lane[lane_pos++] = pimpl->modules[it->second].checksum;
        if (lane_pos == 4) {
            hasher.updateHash(lane);
            lane[0] = lane[1] = lane[2] = lane[3] = 0;
            lane_pos = 0;
        }
    }

    if (lane_pos > 0) {
        // lane is already zero-padded in the unused slots
        hasher.updateHash(lane);
    }

    // Hash the build_id string into a uint64_t, then mix it in together with
    // KNIGHTS_VERSION_NUM so that different game versions are always incompatible.
    uint64_t build_id_hash = 0;
    if (!pimpl->build_id.empty()) {
        XXHash build_id_hasher(0);
        build_id_hasher.updateHashPartial(
            reinterpret_cast<const uint8_t*>(pimpl->build_id.data()),
            pimpl->build_id.size());
        build_id_hash = build_id_hasher.finalHash();
    }
    uint64_t version_lane[4] = {static_cast<uint64_t>(KNIGHTS_VERSION_NUM), build_id_hash, 0, 0};
    hasher.updateHash(version_lane);

    return hasher.finalHash();
}

bool ModuleManager::isCompatible(const GameModuleSpec &other_spec,
                                 std::vector<std::string> &missing_modules_out) const
{
    missing_modules_out.clear();

    for (const auto &other_module : other_spec.module_vfs_names) {
        if (pimpl->index.count(other_module) == 0) {
            missing_modules_out.push_back(other_module);
        }
    }

    if (!missing_modules_out.empty()) {
        return false;
    }

    return computeCombinedChecksum(other_spec.module_vfs_names) == other_spec.checksum;
}
