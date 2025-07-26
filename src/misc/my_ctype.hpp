/*
 * my_ctype.hpp
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

#ifndef MY_CTYPE_HPP
#define MY_CTYPE_HPP

// std::isspace has a bit of a gotcha because it takes an int rather
// than a char. Since char is signed, chars in the 128..255 range will
// convert to a NEGATIVE integer, but the behaviour of isspace is
// undefined for negative arguments.

// i.e.
// 
// char c;
// isspace(c);                 // WRONG -- undefined behaviour if c < 0.
// isspace((unsigned char)c);  // Correct.

// The same applies to other ctype functions - isdigit, toupper, etc.

// To avoid this whole issue I have created my own functions that take
// a char input and cast it to unsigned char. Problem solved!

#include <cctype>

inline bool IsDigit(char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; }
inline bool IsSpace(char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; }
inline char ToLower(char c) { return std::tolower(static_cast<unsigned char>(c)); }
inline char ToUpper(char c) { return std::toupper(static_cast<unsigned char>(c)); }

#endif
