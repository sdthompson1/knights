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
#include "online_platform.hpp"
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
#ifdef ONLINE_PLATFORM
    explicit ModuleManagerImpl(OnlinePlatform &op) : online_platform(op) { }
    OnlinePlatform &online_platform;
#endif
    std::optional<std::filesystem::path> pref_path;
    std::filesystem::path knights_data_modules_dir;
    std::vector<std::filesystem::path> extra_module_dirs;
    std::vector<std::string> cmd_line_load_order;
    std::string build_id;
    std::vector<ModuleInfo> modules;  // All available (installed) modules
    std::unordered_map<std::string, size_t> index;   // Index into `modules` vector
    std::vector<std::string> enabled_modules;   // Enabled modules (for new games)
};

namespace {
    // Add a filesystem directory to `infos` and `known`
    // (if it is a valid module directory)
    void AddModuleInfo(std::vector<ModuleInfo> &infos,
                       std::unordered_set<std::string> &known,
                       std::string name,  // "VFS mod name" e.g. "workshop_123" or dir name
                       const std::filesystem::path &path)
    {
        // Only directories are accepted
        if (!std::filesystem::is_directory(path)) return;

        // Only accept valid mod names
        if (!IsValidModuleName(name)) return;

        // Construct the ModuleInfo and add it
        ModuleInfo info;
        info.name = name;
        info.path = path;
        info.checksum = ComputeLocalChecksum(path);
        infos.push_back(info);
        known.insert(name);
    }
}

ModuleManager::ModuleManager(std::optional<std::filesystem::path> pref_path,
                             std::filesystem::path knights_data_modules_dir,
                             std::vector<std::filesystem::path> extra_module_dirs,
                             std::vector<std::string> cmd_line_load_order
#ifdef ONLINE_PLATFORM
                             , OnlinePlatform &online_platform
#endif
                             )
    : pimpl(std::make_unique<ModuleManagerImpl>(
#ifdef ONLINE_PLATFORM
          online_platform
#endif
      ))
{
    pimpl->pref_path = std::move(pref_path);
    pimpl->knights_data_modules_dir = std::move(knights_data_modules_dir);
    pimpl->extra_module_dirs = std::move(extra_module_dirs);
    pimpl->cmd_line_load_order = std::move(cmd_line_load_order);
#ifdef ONLINE_PLATFORM
    pimpl->build_id = online_platform.getBuildId();
#endif

    // Do an initial update so that we are ready to go from the start
    update();
}

ModuleManager::~ModuleManager() = default;

void ModuleManager::update()
{
    // Read the module load order from modules.txt (or cmd_line_load_order if set).
    std::vector<std::string> enabled_names;
    if (!pimpl->cmd_line_load_order.empty()) {
        // Use the cmd line load order, ignoring modules.txt.
        std::unordered_set<std::string> seen;
        for (const std::string &name : pimpl->cmd_line_load_order) {
            if (!IsValidModuleName(name)) {
                throw std::runtime_error("Invalid module name: '" + name + "'");
            }
            if (seen.insert(name).second) enabled_names.push_back(name);
        }
    } else if (pimpl->pref_path) {
        // Load modules.txt from the prefs directory.
        enabled_names = ReadModuleNames(pimpl->pref_path);

        // If loading fails (or no modules.txt exists), try re-using the previous value
        // of pimpl->enabled_names. On startup this will just be empty, but if the user
        // went to the Mods UI (which calls setAndSaveLoadOrder) there might be something
        // valid there. This at least means that mods set in the UI will work for this
        // session, even if saving for future sessions fails for some reason.
        if (enabled_names.empty()) {
            enabled_names = pimpl->enabled_modules;
        }
    }

    // Discover all installed modules by scanning:
    //  (1) All subdirectories of knights_data/modules
    //  (2) All dirs given in extra_module_dirs (if any)
    //  (3) All installed Steam Workshop (or other online platform) mods

    std::vector<ModuleInfo> new_modules;
    std::unordered_set<std::string> known;

    // (1) knights_data modules (sorted alphabetically)
    for (const auto &entry : std::filesystem::directory_iterator(pimpl->knights_data_modules_dir)) {
        AddModuleInfo(new_modules, known, entry.path().filename().string(), entry.path());
    }
    std::sort(new_modules.begin(), new_modules.end(),
              [](const ModuleInfo &lhs, const ModuleInfo &rhs) {
                  return lhs.name < rhs.name;
              });

    // (2) extra_module_dirs (in order given, and after the knights_data modules)
    for (const auto &path : pimpl->extra_module_dirs) {
        AddModuleInfo(new_modules, known, path.filename().string(), path);
    }

#ifdef ONLINE_PLATFORM
    // (3) Workshop modules, after the others, in the order returned by the online platform.
    for (const OnlinePlatform::InstalledMod &mod : pimpl->online_platform.getInstalledMods()) {
        AddModuleInfo(new_modules, known, mod.vfs_name, mod.path);
    }
#endif

    // Filter down the load order (enabled_names) to only include actually installed names.
    enabled_names.erase(
        std::remove_if(enabled_names.begin(), enabled_names.end(),
            [&known](const std::string& s) { return !known.count(s); }),
        enabled_names.end());

    // If the load order is empty, use "base" as a default.
    if (enabled_names.empty()) {
        if (!known.count("base")) {
            // "base" should always exist
            throw std::runtime_error("'base' module not found!");
        }
        enabled_names.push_back("base");
    }

    // Success: copy results back, and recompute pimpl->index.
    pimpl->modules = std::move(new_modules);
    pimpl->index.clear();
    for (size_t i = 0; i < pimpl->modules.size(); ++i) {
        pimpl->index[pimpl->modules[i].name] = i;
    }
    pimpl->enabled_modules = std::move(enabled_names);
}

void ModuleManager::setAndSaveLoadOrder(std::vector<std::string> load_order)
{
    pimpl->enabled_modules = load_order;
    WriteModuleNames(pimpl->pref_path, load_order);  // save to modules.txt if possible
    pimpl->cmd_line_load_order.clear();
    update();
}

bool ModuleManager::isModuleInstalled(const std::string &module_name) const
{
    return pimpl->index.count(module_name) != 0;
}

std::vector<std::string> ModuleManager::getInstalledModules() const
{
    std::vector<std::string> result;
    result.reserve(pimpl->modules.size());
    for (const ModuleInfo &info : pimpl->modules) {
        result.push_back(info.name);
    }
    return result;
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
