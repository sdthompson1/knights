/*
 * rstream_rwops.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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
 * Create an SDL_RWops structure from an rstream.
 * Note that it will be read-only; the write functions will fail (returning -1).
 *
 * NB: Could easily convert this to a general "iostream_rwops" class.
 *
 * NB: This is not actually used currently (as at 10-May-2024).
 *
 */

#ifndef RSTREAM_RWOPS_HPP
#define RSTREAM_RWOPS_HPP

#include "rstream.hpp"

#include <SDL_rwops.h>


// At the moment can load only from a resource name,
// but extension to loading from a given rstream is easy.
// The returned ptr must be freed in usual way (using SDL_RWclose)
SDL_RWops* RWFromRStream(const std::string &resource_name);

#endif  // RSTREAM_RWOPS_HPP
