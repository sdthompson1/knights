/*
 * memory_block_decompressor.hpp
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

#ifndef MEMORY_BLOCK_DECOMPRESSOR_HPP
#define MEMORY_BLOCK_DECOMPRESSOR_HPP

#ifdef USE_VM_LOBBY

#include "knights_vm.hpp"

#include <zlib.h>

class MemoryBlockDecompressor {
public:
    MemoryBlockDecompressor();
    ~MemoryBlockDecompressor();

    // Read a group of compressed memory blocks that was created by a MemoryBlockCompressor.
    // Decompress the contents, and install them into the given KnightsVM.
    // (The data is read starting at input[start_pos]. Total number of bytes read from the
    // vector is returned.)
    size_t readCompressedBlockGroup(const std::vector<unsigned char> &input,
                                    size_t start_pos, KnightsVM &vm);

private:
    MemoryBlockDecompressor(const MemoryBlockDecompressor &) = delete;
    void operator=(const MemoryBlockDecompressor &) = delete;

private:
    z_stream decompression_stream;
};

#endif  // USE_VM_LOBBY

#endif  // MEMORY_BLOCK_DECOMPRESSOR_HPP
