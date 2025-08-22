/*
 * memory_block_compressor.hpp
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

#ifndef MEMORY_BLOCK_COMPRESSOR_HPP
#define MEMORY_BLOCK_COMPRESSOR_HPP

#ifdef USE_VM_LOBBY

#include "knights_vm.hpp"  // for MemoryBlock

#include <zlib.h>

class MemoryBlockCompressor {
public:
    MemoryBlockCompressor();
    ~MemoryBlockCompressor();

    // Pop some number of blocks (at least one) from the queue, and convert them to a
    // compressed byte vector that can be sent over the network.
    // The compressed data is appended to the given output vector.
    void appendCompressedBlockGroup(std::deque<MemoryBlock> &blocks,
                                    std::vector<unsigned char> &output);

private:
    MemoryBlockCompressor(const MemoryBlockCompressor &) = delete;
    void operator=(const MemoryBlockCompressor &) = delete;

private:
    z_stream compression_stream;
};

#endif  // USE_VM_LOBBY

#endif  // MEMORY_BLOCK_COMPRESSOR_HPP
