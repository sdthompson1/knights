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
    bool is_local;  // true for local mods, false for Workshop (online platform) mods
    bool enabled;
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
};

namespace {
    // Add a filesystem directory to `infos`
    // (if it is a valid module directory)
    void AddModuleInfo(std::vector<ModuleInfo> &infos,
                       std::string name,  // "VFS mod name" e.g. "workshop_123" or dir name
                       const std::filesystem::path &path,
                       bool is_local)
    {
        // Only directories are accepted
        if (!std::filesystem::is_directory(path)) return;

        // Only accept valid mod names
        if (!IsValidModuleName(name)) return;

        // Construct the ModuleInfo and add it (disabled initially)
        ModuleInfo info;
        info.name = name;
        info.path = path;
        info.checksum = ComputeLocalChecksum(path);
        info.is_local = is_local;
        info.enabled = false;
        infos.push_back(info);
    }

    // Discover all installed modules by scanning:
    //  (1) All subdirectories of knights_data/modules
    //  (2) All dirs given in extra_module_dirs (if any)
    //  (3) All installed Steam Workshop (or other online platform) mods
    // Results are appended to "infos", with a correctly computed checksum, but with
    // enabled=false initially.
    void DiscoverModules(const std::filesystem::path &knights_data_modules_dir,
                         const std::vector<std::filesystem::path> &extra_module_dirs,
#ifdef ONLINE_PLATFORM
                         OnlinePlatform &online_platform,
#endif
                         std::vector<ModuleInfo> &infos)
    {
        // (1) knights_data modules (sorted alphabetically)
        for (const auto &entry : std::filesystem::directory_iterator(knights_data_modules_dir)) {
            AddModuleInfo(infos, entry.path().filename().string(), entry.path(), true);
        }
        std::sort(infos.begin(), infos.end(),
                  [](const ModuleInfo &lhs, const ModuleInfo &rhs) {
                      return lhs.name < rhs.name;
                  });

        // (2) extra_module_dirs (in order given, and after the knights_data modules)
        for (const auto &path : extra_module_dirs) {
            AddModuleInfo(infos, path.filename().string(), path, true);
        }

#ifdef ONLINE_PLATFORM
        // (3) Workshop modules, after the others, in the order returned by the online platform.
        for (const OnlinePlatform::InstalledMod &mod : online_platform.getInstalledMods()) {
            AddModuleInfo(infos, mod.vfs_name, mod.path, false);
        }
#endif
    }

    // Adjust a load order (vector of ModulesTxtLine) to match a given list of
    // installed modules.
    // Load order items corresponding to a non-installed module are removed.
    // Then, any installed modules not in the load order are added to the end,
    // with the enabled flag taken from the installed-list.
    void ReconcileLoadOrder(std::vector<ModulesTxtLine> &load_order,
                            const std::vector<ModuleInfo> &installed)
    {
        // Make a hash set of the installed names, for speedy lookup
        std::unordered_set<std::string> known;
        for (const ModuleInfo &info : installed) {
            known.insert(info.name);
        }

        // Remove load order entries that correspond to unknown (uninstalled) modules
        std::erase_if(load_order, [&known](const ModulesTxtLine &line) {
            return known.count(line.module_name) == 0; });

        // Now we need the hash set to contain everything that exists in the load order
        known.clear();
        for (const ModulesTxtLine &line : load_order) {
            known.insert(line.module_name);
        }

        // Now add any installed-modules that are NOT in the load order
        for (const ModuleInfo &info : installed) {
            if (known.count(info.name) == 0) {
                load_order.push_back(ModulesTxtLine{ info.name, info.enabled });
            }
        }
    }

    // Reorder the "infos" according to the given load order, and apply the
    // enabled-flags from the load order as well.
    // ReconcileLoadOrder should be called first (so that the load_order includes
    // exactly the set of module names present in "infos").
    // Throws exception if no module is enabled.
    void ApplyLoadOrder(std::vector<ModuleInfo> &infos,
                        const std::vector<ModulesTxtLine> &load_order)
    {
        // Build an index
        std::unordered_map<std::string, const ModuleInfo *> index;
        for (const ModuleInfo &info : infos) {
            index[info.name] = &info;
        }

        // Copy the ModuleInfos to the new vector in the correct order, and set
        // the enabled flags
        bool any_enabled = false;
        std::vector<ModuleInfo> new_infos;
        new_infos.reserve(load_order.size());
        for (const ModulesTxtLine &line : load_order) {
            auto find = index.find(line.module_name);
            if (find == index.end()) {
                throw std::runtime_error("ApplyLoadOrder: load_order must be reconciled first");
            }
            new_infos.push_back(*find->second);
            new_infos.back().enabled = line.enabled;
            any_enabled |= line.enabled;
        }

        if (!any_enabled) {
            // This is always a user error: the mods UI always adds "base" as an
            // enabled module, and "base" should always be installed, so if we end
            // up without any enabled modules, the user must have either deleted
            // knights_data/modules/base, or edited modules.txt by hand, or set
            // enabled modules on the command line (setting no valid ones).
            throw std::runtime_error("No modules enabled! Check modules.txt, command line options, and whether knights_data/modules/base exists.");
        }

        // Copy results back
        infos = std::move(new_infos);
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
    bool using_modules_txt = false;
    std::vector<ModulesTxtLine> modules_txt_contents;

    // Read the module load order from modules.txt (or cmd_line_load_order if set).
    std::vector<ModulesTxtLine> load_order;
    if (!pimpl->cmd_line_load_order.empty()) {
        // Use the cmd line load order, ignoring modules.txt.
        std::unordered_set<std::string> seen;
        for (const std::string &name : pimpl->cmd_line_load_order) {
            if (!IsValidModuleName(name)) {
                throw std::runtime_error("Invalid module name: '" + name + "'");
            }
            if (seen.insert(name).second) {
                load_order.push_back(ModulesTxtLine{ name, true });
            }
        }
    } else if (pimpl->pref_path) {
        // Load modules.txt from the prefs directory.
        load_order = ReadModulesTxt(pimpl->pref_path);
        using_modules_txt = true;
        modules_txt_contents = load_order;

        // If loading fails (or no modules.txt exists), try re-using the previous value
        // of getInstalledModules as the load order. On startup this will just be empty,
        // but if the user went to the Mods UI (which calls setAndSaveLoadOrder) there
        // might be something valid there. This at least means that mods set in the UI
        // will work for this session, even if saving for future sessions fails for some
        // reason.
        if (load_order.empty()) {
            load_order = getInstalledModules();
            using_modules_txt = false;
        }
    }

    // If the load order is empty, use "base" as a default.
    if (load_order.empty()) {
        load_order.push_back(ModulesTxtLine{ "base", true });
    }

    // Discover all installed modules
    std::vector<ModuleInfo> new_modules;
    DiscoverModules(pimpl->knights_data_modules_dir,
                    pimpl->extra_module_dirs,
#ifdef ONLINE_PLATFORM
                    pimpl->online_platform,
#endif
                    new_modules);

    // Reconcile the load order with the installed list
    ReconcileLoadOrder(load_order, new_modules);

    // If we're using the contents of modules.txt verbatim, but then the load_order
    // changed after reconciliation, then this means that modules.txt is out of date
    // (e.g. because new modules were installed, or previously enabled mods were uninstalled).
    // To avoid future confusion, we'll try to update modules.txt right now.
    if (using_modules_txt && load_order != modules_txt_contents) {
        WriteModulesTxt(pimpl->pref_path, load_order);
    }

    // Apply the load order to the installed list
    ApplyLoadOrder(new_modules, load_order);

    // Copy the result to pimpl->modules and recompute the index
    pimpl->modules = std::move(new_modules);
    pimpl->index.clear();
    for (size_t i = 0; i < pimpl->modules.size(); ++i) {
        pimpl->index[pimpl->modules[i].name] = i;
    }
}

void ModuleManager::setAndSaveLoadOrder(std::vector<ModulesTxtLine> load_order)
{
    // Cancel any previous command line load order
    pimpl->cmd_line_load_order.clear();

    // Reconcile the load order and apply it to the current pimpl->modules list.
    ReconcileLoadOrder(load_order, pimpl->modules);
    ApplyLoadOrder(pimpl->modules, load_order);

    // Save to modules.txt if possible.
    WriteModulesTxt(pimpl->pref_path, load_order);

    // Now do a full update.
    // Note: If saving failed, this will revert back to what's on disk currently.
    // There are two cases:
    //  1) No modules.txt exists at all. In this case, the fallback in update() will
    //     engage, and the current getInstalledModules() list will be used. That was
    //     already set to load_order above (in the call to ApplyLoadOrder) so that's
    //     what we want. The user's load_order will be used for this session, but it
    //     can't be saved for future sessions.
    //  2) A conflicting modules.txt exists on disk, and we can't write to it. In
    //     this case, the existing modules.txt takes priority, and the user's attempt
    //     to set a new load order is just ignored.
    update();
}

bool ModuleManager::isModuleInstalled(const std::string &module_name) const
{
    return pimpl->index.count(module_name) != 0;
}

bool ModuleManager::isLocalModule(const std::string &module_name) const
{
    auto it = pimpl->index.find(module_name);
    return it != pimpl->index.end() && pimpl->modules[it->second].is_local;
}

std::filesystem::path ModuleManager::getModulePath(const std::string &module_name) const
{
    auto it = pimpl->index.find(module_name);
    if (it == pimpl->index.end()) {
        throw std::runtime_error("Unknown module: '" + module_name + "'");
    }
    return pimpl->modules[it->second].path;
}

std::vector<ModulesTxtLine> ModuleManager::getInstalledModules() const
{
    std::vector<ModulesTxtLine> result;
    result.reserve(pimpl->modules.size());
    for (const ModuleInfo &info : pimpl->modules) {
        result.push_back(ModulesTxtLine{ info.name, info.enabled });
    }
    return result;
}

std::vector<std::string> ModuleManager::getEnabledModules() const
{
    std::vector<std::string> result;
    for (const ModuleInfo &info : pimpl->modules) {
        if (info.enabled) {
            result.push_back(info.name);
        }
    }
    return result;
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
    bool all_workshop = true;

    for (const auto &other_module : other_spec.module_vfs_names) {
        if (pimpl->index.count(other_module) == 0) {
            missing_modules_out.push_back(other_module);
#ifdef ONLINE_PLATFORM
            if (!pimpl->online_platform.isWorkshopModName(other_module))
#endif
                all_workshop = false;
        }
    }

    if (!missing_modules_out.empty()) {
        // Missing Workshop mods can be installed by the user, so the game still counts
        // as compatible. (The checksum will be checked once the mods are installed.)
        return all_workshop;
    }

    return computeCombinedChecksum(other_spec.module_vfs_names) == other_spec.checksum;
}
