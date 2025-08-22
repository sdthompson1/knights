/*
 * memory_block_compressor.cpp
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

#include "memory_block_compressor.hpp"
#include <stdexcept>
#include <cstring>

MemoryBlockCompressor::MemoryBlockCompressor()
{
    compression_stream.zalloc = Z_NULL;
    compression_stream.zfree = Z_NULL;
    compression_stream.opaque = Z_NULL;

    int ret = deflateInit(&compression_stream, Z_DEFAULT_COMPRESSION);
    if (ret != Z_OK) {
        throw std::runtime_error("Failed to initialize compression stream");
    }
}

MemoryBlockCompressor::~MemoryBlockCompressor()
{
    deflateEnd(&compression_stream);
}

void MemoryBlockCompressor::appendCompressedBlockGroup(std::deque<MemoryBlock> &blocks,
                                                       std::vector<unsigned char> &output)
{
    const int MAX_BLOCKS = 8;
    std::vector<MemoryBlock> selected_blocks;
    selected_blocks.reserve(MAX_BLOCKS);

    // Pop blocks from the front until we have up to 8 non-empty blocks
    while (selected_blocks.size() < MAX_BLOCKS && !blocks.empty()) {
        MemoryBlock block = std::move(blocks.front());
        blocks.pop_front();

        if (!block.contents.empty()) {
            selected_blocks.push_back(std::move(block));
        }
    }

    // Write base addresses (8 addresses total, zero for missing blocks)
    for (int i = 0; i < MAX_BLOCKS; ++i) {
        uint32_t base_address = 0;
        if (i < selected_blocks.size()) {
            base_address = selected_blocks[i].base_address;
        }

        // Write as 32-bit little-endian
        output.push_back(base_address & 0xFF);
        output.push_back((base_address >> 8) & 0xFF);
        output.push_back((base_address >> 16) & 0xFF);
        output.push_back((base_address >> 24) & 0xFF);
    }

    // Prepare input data for compression
    std::vector<unsigned char> input_data;
    for (const auto& block : selected_blocks) {
        // Convert each 32-bit word to bytes in little-endian order
        for (uint32_t word : block.contents) {
            input_data.push_back(word & 0xFF);
            input_data.push_back((word >> 8) & 0xFF);
            input_data.push_back((word >> 16) & 0xFF);
            input_data.push_back((word >> 24) & 0xFF);
        }
    }

    // Callers should ensure that there is at least one non-empty block to compress
    if (input_data.empty()) {
        throw std::runtime_error("No data to compress");
    }

    // Estimate maximum compressed size
    uLong max_compressed_size = deflateBound(&compression_stream, input_data.size());
    std::vector<unsigned char> compressed_buffer(max_compressed_size);

    // Set up compression stream
    compression_stream.next_in = input_data.data();
    compression_stream.avail_in = input_data.size();
    compression_stream.next_out = compressed_buffer.data();
    compression_stream.avail_out = max_compressed_size;

    // Compress using Z_SYNC_FLUSH to maintain stream state for next call
    int result = deflate(&compression_stream, Z_SYNC_FLUSH);
    if (result != Z_OK) {
        throw std::runtime_error("Compression failed");
    }

    uLong compressed_size = max_compressed_size - compression_stream.avail_out;

    // Write compressed size as 32-bit little-endian
    output.push_back(compressed_size & 0xFF);
    output.push_back((compressed_size >> 8) & 0xFF);
    output.push_back((compressed_size >> 16) & 0xFF);
    output.push_back((compressed_size >> 24) & 0xFF);

    // Write compressed data
    output.insert(output.end(), compressed_buffer.begin(), compressed_buffer.begin() + compressed_size);
}

#endif  // USE_VM_LOBBY
