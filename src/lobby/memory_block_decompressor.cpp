/*
 * memory_block_decompressor.cpp
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

#include "misc.hpp"

#ifdef USE_VM_LOBBY

#include "memory_block_decompressor.hpp"
#include "protocol.hpp"
#include <stdexcept>
#include <cstring>

MemoryBlockDecompressor::MemoryBlockDecompressor()
{
    decompression_stream.zalloc = Z_NULL;
    decompression_stream.zfree = Z_NULL;
    decompression_stream.opaque = Z_NULL;
    decompression_stream.avail_in = 0;
    decompression_stream.next_in = Z_NULL;

    int ret = inflateInit(&decompression_stream);
    if (ret != Z_OK) {
        throw std::runtime_error("Failed to initialize decompression stream");
    }
}

MemoryBlockDecompressor::~MemoryBlockDecompressor()
{
    inflateEnd(&decompression_stream);
}

size_t MemoryBlockDecompressor::readCompressedBlockGroup(const std::vector<unsigned char> &input,
                                                        size_t start_pos, KnightsVM &vm)
{
    const int MAX_BLOCKS = 8;
    size_t pos = start_pos;

    if (pos + (MAX_BLOCKS * 4) > input.size()) {
        throw std::runtime_error("Input buffer too small for base addresses");
    }

    // Read 8 base addresses (32-bit little-endian each)
    uint32_t base_addresses[MAX_BLOCKS];
    for (int i = 0; i < MAX_BLOCKS; ++i) {
        base_addresses[i] = static_cast<uint32_t>(input[pos]) |
                           (static_cast<uint32_t>(input[pos + 1]) << 8) |
                           (static_cast<uint32_t>(input[pos + 2]) << 16) |
                           (static_cast<uint32_t>(input[pos + 3]) << 24);
        pos += 4;
    }

    // Read compressed data size (32-bit little-endian)
    if (pos + 4 > input.size()) {
        throw std::runtime_error("Input buffer too small for compressed size");
    }

    uint32_t compressed_size = static_cast<uint32_t>(input[pos]) |
                              (static_cast<uint32_t>(input[pos + 1]) << 8) |
                              (static_cast<uint32_t>(input[pos + 2]) << 16) |
                              (static_cast<uint32_t>(input[pos + 3]) << 24);
    pos += 4;

    // Check that we have enough data for the compressed payload
    if (pos + compressed_size > input.size()) {
        throw std::runtime_error("Input buffer too small for compressed data");
    }

    if (compressed_size == 0) {
        throw std::runtime_error("No compressed data to decompress");
    }

    // Count how many non-zero base addresses we have
    size_t expected_blocks = 0;
    for (int i = 0; i < MAX_BLOCKS; ++i) {
        if (base_addresses[i] != 0) {
            expected_blocks++;
        } else {
            break; // Assume zero addresses come at the end
        }
    }

    if (expected_blocks == 0) {
        throw std::runtime_error("No valid base addresses found");
    }

    // Calculate expected decompressed size based on block count
    size_t expected_decompressed_size = expected_blocks * HOST_MIGRATION_BLOCK_SIZE_BYTES;
    std::vector<unsigned char> decompressed_buffer(expected_decompressed_size);

    // Set up decompression stream
    decompression_stream.next_in = const_cast<unsigned char*>(&input[pos]);
    decompression_stream.avail_in = compressed_size;
    decompression_stream.next_out = decompressed_buffer.data();
    decompression_stream.avail_out = expected_decompressed_size;

    // Decompress using Z_SYNC_FLUSH to maintain stream state
    int result = inflate(&decompression_stream, Z_SYNC_FLUSH);
    if (result != Z_OK) {
        throw std::runtime_error("Decompression failed");
    }

    size_t actual_decompressed_size = expected_decompressed_size - decompression_stream.avail_out;
    if (actual_decompressed_size != expected_decompressed_size) {
        throw std::runtime_error("Decompressed size mismatch");
    }

    // Write each block to VM memory
    const uint32_t words_per_block = HOST_MIGRATION_BLOCK_SIZE_BYTES / 4;
    size_t byte_offset = 0;

    for (size_t block_idx = 0; block_idx < expected_blocks; ++block_idx) {
        uint32_t base_addr = base_addresses[block_idx];

        // Write all words for this block
        for (uint32_t word_idx = 0; word_idx < words_per_block; ++word_idx) {
            // Convert from little-endian bytes to uint32_t
            uint32_t word = static_cast<uint32_t>(decompressed_buffer[byte_offset]) |
                           (static_cast<uint32_t>(decompressed_buffer[byte_offset + 1]) << 8) |
                           (static_cast<uint32_t>(decompressed_buffer[byte_offset + 2]) << 16) |
                           (static_cast<uint32_t>(decompressed_buffer[byte_offset + 3]) << 24);
            byte_offset += 4;

            vm.writeWord(base_addr + (word_idx * 4), word);
        }
    }

    pos += compressed_size;
    return pos - start_pos;
}

#endif  // USE_VM_LOBBY
