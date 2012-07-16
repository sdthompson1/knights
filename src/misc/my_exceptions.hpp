/*
 * my_exceptions.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
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

/*
 * An attempt to standardize some of the exceptions used in the game
 *
 */

#ifndef MY_EXCEPTIONS_HPP
#define MY_EXCEPTIONS_HPP

#include "exception_base.hpp"

#include <string>
using namespace std;

// error during game initialization (eg: sdl font could not be loaded)
class InitError : public ExceptionBase {
public:
    InitError(const string &s) : ExceptionBase(s) { }
};

// failed to load graphic (or other resource!)
class GraphicLoadFailed : public ExceptionBase {
public:
    GraphicLoadFailed(const string &s) : ExceptionBase(s) { }
};

// error when loading or running a Lua script file
class LuaError : public ExceptionBase {
public:
    LuaError(const string &s) : ExceptionBase(s) { }
};

// lua panic (lua error in unprotected context).
// (this should be treated as a fatal error and lua_State should be closed as soon as possible)
class LuaPanic : public ExceptionBase {
public:
    LuaPanic(const string &s) : ExceptionBase(s) { }
};

// something unexpected happened (throwing this indicates a bug or not-yet-implemented
// feature)

class UnexpectedError : public ExceptionBase {
public:
    UnexpectedError(const string &s) : ExceptionBase(s) { }
};


#endif
