/*
 * rstream_rwops.hpp
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

/*
 * Create an SDL_RWops structure from a VFS file.
 * Note that it will be read-only; the write functions will fail (returning -1).
 *
 * NB: This is not actually used currently (as at 10-May-2024).
 *
 */

#ifndef RSTREAM_RWOPS_HPP
#define RSTREAM_RWOPS_HPP

#include "vfs.hpp"

#include <SDL_rwops.h>

// Open vfs_path in the given VFS and wrap it in an SDL_RWops.
// The returned ptr must be freed in the usual way (using SDL_RWclose).
SDL_RWops* RWFromVFS(const VFS &vfs, const std::string &vfs_path);

#endif  // RSTREAM_RWOPS_HPP
