/*
 * xxhash.cpp
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

#include "xxhash.hpp"

#include "network/byte_buf.hpp"

#include <algorithm>
#include <cstring>

namespace {
    // XXHash64 prime constants
    constexpr uint64_t PRIME64_1 = UINT64_C(0x9E3779B185EBCA87);
    constexpr uint64_t PRIME64_2 = UINT64_C(0xC2B2AE3D27D4EB4F);
    constexpr uint64_t PRIME64_3 = UINT64_C(0x165667B19E3779F9);
    constexpr uint64_t PRIME64_4 = UINT64_C(0x85EBCA77C2B2AE63);
    constexpr uint64_t PRIME64_5 = UINT64_C(0x27D4EB2F165667C5);

    uint64_t RotateLeft(uint64_t x, unsigned int bits)
    {
        return (x << bits) | (x >> (64 - bits));
    }

    uint64_t HashRound(uint64_t accN, uint64_t laneN)
    {
        accN += (laneN * PRIME64_2);
        accN = RotateLeft(accN, 31);
        return accN * PRIME64_1;
    }

    uint64_t MergeAccumulator(uint64_t acc, uint64_t accN)
    {
        acc ^= HashRound(0, accN);
        acc *= PRIME64_1;
        return acc + PRIME64_4;
    }
}

XXHash::XXHash(uint64_t seed)
{
    // Step 1. Initialize internal accumulators
    acc[0] = seed + PRIME64_1 + PRIME64_2;
    acc[1] = seed + PRIME64_2;
    acc[2] = seed;
    acc[3] = seed - PRIME64_1;
    total_length = 0;
}

void XXHash::updateHash(const uint64_t lane[4])
{
    // Step 2. Process stripes
    for (int i = 0; i < 4; ++i) {
        acc[i] = HashRound(acc[i], lane[i]);
    }
    // Track the input length (32 bytes per stripe)
    total_length += 32;
}

void XXHash::updateHashPartial(const uint8_t* data, size_t length)
{
    total_length += length;

    // Process data in 32-byte chunks, padding the last chunk with zeros
    while (length > 0) {
        uint8_t buffer[32] = {0};  // Zero-initialized padding
        size_t chunk_size = std::min(length, size_t(32));
        memcpy(buffer, data, chunk_size);

        // Convert buffer to little-endian 64-bit lanes
        uint64_t lanes[4];
        for (int i = 0; i < 4; i++) {
            lanes[i] = 0;
            for (int j = 0; j < 8; j++) {
                lanes[i] |= uint64_t(buffer[i*8 + j]) << (j*8);
            }
        }

        // Process using existing updateHash
        updateHash(lanes);

        data += chunk_size;
        length -= chunk_size;
    }
}

uint64_t XXHash::finalHash() const
{
    // Step 3. Accumulator convergence
    uint64_t hash = RotateLeft(acc[0], 1)
        + RotateLeft(acc[1], 7)
        + RotateLeft(acc[2], 12)
        + RotateLeft(acc[3], 18);
    for (int i = 0; i < 4; ++i) {
        hash = MergeAccumulator(hash, acc[i]);
    }

    // Step 4. Add input length
    hash += total_length;

    // Step 6. Final mix (avalanche)
    hash ^= (hash >> 33);
    hash *= PRIME64_2;
    hash ^= (hash >> 29);
    hash *= PRIME64_3;
    hash ^= (hash >> 32);

    return hash;
}

XXHash::XXHash(Coercri::InputByteBuf &buf)
{
    for (int i = 0; i < 4; ++i) {
        uint32_t low = buf.readUlong();
        uint32_t high = buf.readUlong();
        acc[i] = uint64_t(low) | (uint64_t(high) << 32);
    }
    uint32_t low = buf.readUlong();
    uint32_t high = buf.readUlong();
    total_length = uint64_t(low) | (uint64_t(high) << 32);
}

void XXHash::writeInternalState(Coercri::OutputByteBuf &buf) const
{
    for (int i = 0; i < 4; ++i) {
        buf.writeUlong(acc[i] & 0xffffffffu);
        buf.writeUlong(acc[i] >> 32);
    }
    buf.writeUlong(total_length & 0xffffffffu);
    buf.writeUlong(total_length >> 32);
}
