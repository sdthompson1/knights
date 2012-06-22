/*
 * FILE:
 *   byte_buf.hpp
 *
 * PURPOSE:
 *   Classes for marshalling various types (ints, strings etc) to/from
 *   a vector<unsigned char>. This makes it easier to send/receive
 *   binary data over a NetworkConnection.
 *   
 * AUTHOR:
 *   Stephen Thompson
 *
 * CREATED:
 *   20-Mar-2009
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2009.
 *
 *   This file is part of the "Coercri" software library. Usage of "Coercri"
 *   is permitted under the terms of the Boost Software License, Version 1.0, 
 *   the text of which is displayed below.
 *
 *   Boost Software License - Version 1.0 - August 17th, 2003
 *
 *   Permission is hereby granted, free of charge, to any person or organization
 *   obtaining a copy of the software and accompanying documentation covered by
 *   this license (the "Software") to use, reproduce, display, distribute,
 *   execute, and transmit the Software, and to prepare derivative works of the
 *   Software, and to permit third-parties to whom the Software is furnished to
 *   do so, all subject to the following:
 *
 *   The copyright notices in the Software and this entire statement, including
 *   the above license grant, this restriction and the following disclaimer,
 *   must be included in all copies of the Software, in whole or in part, and
 *   all derivative works of the Software, unless such copies or derivative
 *   works are solely in the form of machine-executable object code generated by
 *   a source language processor.
 *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
 *   SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
 *   FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *   DEALINGS IN THE SOFTWARE.
 *
 */

#ifndef COERCRI_BYTE_BUF_HPP
#define COERCRI_BYTE_BUF_HPP

#include <string>
#include <vector>

namespace Coercri {

    // Thread Safety: Multiple threads can access DIFFERENT ByteBufs
    // concurrently, but only one thread should access a given ByteBuf
    // at any one time.
    
    class InputByteBuf {
    public:
        typedef unsigned char ubyte;
    
        // NOTE: vector must NOT be reallocated while this class is used,
        // since we keep an iterator into it.
        InputByteBuf(const std::vector<ubyte> &buf_) : buf(buf_), iter(buf_.begin()) { }

        bool eof() const { return iter == buf.end(); }
        size_t getPos() const { return iter - buf.begin(); }
        
        int readUbyte();
        int readByte();
        int readUshort();
        int readShort();
        int readVarInt();  // variable length int (1,2,3 or 4 bytes)
        void readNibbles(int &x, int &y);   // two numbers in range [0,15], encoded in one byte
        std::string readString();  // encoded as length (as VarInt) + data.
        
    private:
        const std::vector<ubyte> &buf;
        std::vector<ubyte>::const_iterator iter;
    };

    class OutputByteBuf {
    public:
        typedef unsigned char ubyte;

        // This appends to the given vector.
        OutputByteBuf(std::vector<ubyte> &buf_) : buf(buf_) { }

        void writeUbyte(int x);
        void writeUshort(int x);
        void writeShort(int x);
        void writeVarInt(int x);
        void writeNibbles(int x, int y);
        void writeString(const std::string &x);

        void writePayloadSize(size_t &pos);  // writes "0" as a ushort; memorizes buffer position
        void backpatchPayloadSize(size_t pos);  // backpatches the "0" with the no of bytes written since then
                                                // (excluding the two bytes for the payload-size)        

    private:
        std::vector<ubyte> &buf;
    };

}

#endif
