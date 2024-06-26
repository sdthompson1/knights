/*
 * FILE:
 *   utf8string.hpp
 *
 * PURPOSE:
 *   Class representing a string that is definitely in UTF-8 format.
 *
 * AUTHOR:
 *   Stephen Thompson <stephen@solarflare.org.uk>
 *
 * COPYRIGHT:
 *   Copyright (C) Stephen Thompson, 2008 - 2024.
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

#ifndef COERCRI_UTF8STRING_HPP
#define COERCRI_UTF8STRING_HPP

#include <string>

namespace Coercri {

    class UTF8String {
    public:

        // CONSTRUCTORS
        
        // construct from a valid UTF-8 string
        //  (throws exception if input is invalid)
        static UTF8String fromUTF8(const std::string &input);

        // construct from a possibly-invalid UTF-8 string
        //  (inserts replacement characters if input is invalid)
        static UTF8String fromUTF8Safe(const std::string &input);
        
        // construct from a Latin-1 string
        static UTF8String fromLatin1(const std::string &input);

        // construct from a single integer code-point
        //  (throws exception if the code-point is not valid)
        static UTF8String fromCodePoint(int cp);

        

        // CONVERSIONS
        
        // convert a Coercri::UTF8String to a std::string in UTF-8 format
        const std::string & asUTF8() const { return repr; }

        // convert a Coercri::UTF8String to a std::string in Latin-1 format
        //  - un-representable characters are replaced with '?'
        std::string asLatin1() const;
        

        
        // GENERAL FUNCTIONS AND OPERATORS
        
        bool empty() const { return repr.empty(); }
        UTF8String& operator+=(const UTF8String &other);
        UTF8String operator+(const UTF8String &other) const;

        // case conversion - NOTE these only work on ASCII chars currently.
        UTF8String toLower() const;
        UTF8String toUpper() const;

        // comparison operators - note that these are not really correct
        // currently (they just compare the UTF-8 bytecodes - they should
        // compare code-points and probably should also take into account
        // normalization).
        bool operator==(const UTF8String &other) const { return repr == other.repr; }
        bool operator<(const UTF8String &other) const { return repr < other.repr; }
        bool operator!=(const UTF8String &other) const { return !(*this == other); }
        
    private:
        std::string repr;
    };
}

#endif
