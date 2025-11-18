/*
 * xxhash.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
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

#ifndef XXHASH_HPP
#define XXHASH_HPP

#include <cstddef>
#include <cstdint>

// XXHash64 implementation
// Based on xxHash, see https://github.com/Cyan4973/xxHash/blob/dev/doc/xxhash_spec.md#xxh64-algorithm-description
class XXHash {
public:
    // Constructor takes the seed value
    explicit XXHash(uint64_t seed);

    // Update the hash with 4 lanes (32 bytes total)
    void updateHash(const uint64_t lane[4]);

    // Update hash with arbitrary-length data (pads to 32-byte boundary with zeros)
    void updateHashPartial(const uint8_t* data, size_t length);

    // Finalize and return the hash value
    uint64_t finalHash() const;

private:
    uint64_t acc[4];
    uint64_t total_length;
};

#endif // XXHASH_HPP
