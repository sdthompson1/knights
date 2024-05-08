/*
 * FILE:
 *   istream_rwops.cpp
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
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

#include "istream_rwops.hpp"

#include <istream>

namespace {

    typedef boost::shared_ptr<std::istream> *stream_ptr;
    
    Sint64 Rseek(SDL_RWops *context, Sint64 offset, int whence)
    {
        stream_ptr str = static_cast<stream_ptr>(context->hidden.unknown.data1);
        switch (whence) {
        case SEEK_SET:
            (*str)->seekg(offset, std::ios_base::beg);
            break;
        case SEEK_CUR:
            (*str)->seekg(offset, std::ios_base::cur);
            break;
        case SEEK_END:
            (*str)->seekg(offset, std::ios_base::end);
            break;
        default:
            return -1; // error
        }
        if (!(*str)) return -1;
        else return (*str)->tellg();
    }
    
    size_t Rread(SDL_RWops *context, void *ptr, size_t size, size_t maxnum)
    {
        stream_ptr str = static_cast<stream_ptr>(context->hidden.unknown.data1);
        (*str)->read(static_cast<char*>(ptr), size*maxnum);
        if (!(*str)) return 0;
        else return (*str)->gcount() / size;
    }
    
    size_t Rwrite(SDL_RWops *context, const void *ptr, size_t size, size_t maxnum)
    {
        return 0; // read only!
    }
    
    int Rclose(SDL_RWops *context)
    {
        stream_ptr str_ptr = static_cast<stream_ptr>(context->hidden.unknown.data1);
        delete str_ptr;
        delete context;
        return 0;
    }
    
}  // namespace

SDL_RWops * Coercri::CreateRWOpsForIstream(boost::shared_ptr<std::istream> str)
{
    SDL_RWops * rwops = new SDL_RWops;
    try {
        rwops->seek = &Rseek;
        rwops->read = &Rread;
        rwops->write = &Rwrite;
        rwops->close = &Rclose;
        
        boost::shared_ptr<std::istream> * str_ptr = new boost::shared_ptr<std::istream>(str);
        rwops->hidden.unknown.data1 = static_cast<void*>(str_ptr); // never throws

        return rwops;    // never throws

    } catch (...) {
        delete rwops;
        throw;
    }
}
