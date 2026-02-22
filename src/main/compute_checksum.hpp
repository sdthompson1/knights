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
#include <string>

inline uint64_t ComputeLocalChecksum(const std::string &build_id)
{
    XXHash hasher(0);
    hasher.updateHashPartial(reinterpret_cast<const uint8_t*>(build_id.data()), build_id.length());
    //RStream::HashDirectory("server", hasher);  // TODO: solve checksumming problem for modules!
    return hasher.finalHash();
}

#endif
