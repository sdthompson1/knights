/*
 * FILE:
 *   utf8string.cpp
 *
 * PURPOSE:
 *   Class representing a string that is definitely in UTF-8 format.
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2014.
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

#include "coercri_error.hpp"
#include "utf8string.hpp"

#include "../external/utf8.h"

namespace Coercri {

    UTF8String UTF8String::fromUTF8(const std::string &input)
    {
        if (!utf8::is_valid(input.begin(), input.end())) {
            throw CoercriError("UTF8String::fromUTF8: Input string is not valid");
        }

        UTF8String result;
        result.repr = input;
        return result;
    }

    UTF8String UTF8String::fromUTF8Safe(const std::string &input)
    {
        UTF8String result;
        utf8::replace_invalid(input.begin(), input.end(), std::back_inserter(result.repr));
        return result;
    }
    
    UTF8String UTF8String::fromLatin1(const std::string &input)
    {
        UTF8String result;

        for (std::string::const_iterator it = input.begin(); it != input.end(); ++it) {
            unsigned int cp = static_cast<unsigned char>(*it);
            utf8::append(cp, std::back_inserter(result.repr));
        }

        return result;
    }

    UTF8String UTF8String::fromCodePoint(int cp)
    {
        UTF8String result;
        utf8::append(cp, std::back_inserter(result.repr));
        return result;
    }

    std::string UTF8String::asLatin1() const
    {
        std::string result;
        std::string::const_iterator it = repr.begin();
        while (it != repr.end()) {
            unsigned int cp = utf8::next(it, repr.end());
            if (cp < 256) {
                result += (unsigned char)(cp);
            } else {
                result += '?';
            }
        }
        return result;
    }

    UTF8String& UTF8String::operator+=(const UTF8String &other)
    {
        repr += other.repr;
        return *this;
    }
    
    UTF8String UTF8String::operator+(const UTF8String &other) const
    {
        UTF8String result(*this);
        result += other;
        return result;
    }

    UTF8String UTF8String::toLower() const
    {
        UTF8String result = *this;
        for (std::string::iterator it = result.repr.begin(); it != result.repr.end(); ++it) {
            if (*it >= 'A' && *it <= 'Z') {
                *it += 'a' - 'A';
            }
        }
        return result;
    }

    UTF8String UTF8String::toUpper() const
    {
        UTF8String result = *this;
        for (std::string::iterator it = result.repr.begin(); it != result.repr.end(); ++it) {
            if (*it >= 'a' && *it <= 'z') {
                *it += 'A' - 'a';
            }
        }
        return result;
    }
}
