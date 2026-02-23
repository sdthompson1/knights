/*
 * compute_checksum.hpp
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

#ifndef COMPUTE_CHECKSUM_HPP
#define COMPUTE_CHECKSUM_HPP

#include "vfs.hpp"
#include "xxhash.hpp"
#include <filesystem>

inline uint64_t ComputeLocalChecksum(const std::filesystem::path &dir_path)
{
    VFS vfs;
    vfs.add(dir_path, "dir");
    XXHash hasher(0);
    vfs.hashDirectory("dir", hasher);
    return hasher.finalHash();
}

#endif
