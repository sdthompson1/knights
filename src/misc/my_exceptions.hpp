/*
 * my_exceptions.hpp
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

#ifndef MY_EXCEPTIONS_HPP
#define MY_EXCEPTIONS_HPP

#include "exception_base.hpp"

#include <string>

// error during game initialization (eg: sdl font could not be loaded)
class InitError : public ExceptionBase {
public:
    explicit InitError(LocalKey k) : ExceptionBase(k) { }
    explicit InitError(const LocalMsg &m) : ExceptionBase(m) { }
};

// failed to load graphic (or other resource!)
class GraphicLoadFailed : public ExceptionBase {
public:
    explicit GraphicLoadFailed(LocalKey k) : ExceptionBase(k) { }
};

// error when loading or running a Lua script file
class LuaError : public ExceptionBase {
public:
    explicit LuaError(const std::string &s)
        : ExceptionBase(LocalMsg{LocalKey("lua_error_is"), {LocalParam(Coercri::UTF8String::fromUTF8Safe(s))}}),
          orig_error_string(s)
    { }

    virtual const char* what() const throw() override {
        return orig_error_string.c_str();
    }

private:
    std::string orig_error_string;
};

// lua panic (lua error in unprotected context).
// (this should be treated as a fatal error and lua_State should be closed as soon as possible)
class LuaPanic : public ExceptionBase {
public:
    explicit LuaPanic(const std::string &s)
        : ExceptionBase(LocalMsg{LocalKey("lua_error_is"), {LocalParam(Coercri::UTF8String::fromUTF8Safe(s))}})
    { }

    virtual const char* what() const throw() override { return "Lua panic!"; }
};

// Something unexpected happened (throwing this indicates a bug or not-yet-implemented
// feature).
// This doesn't use a LocalKey because we don't expect the messages to be shown in
// normal operation, so localizing them has little value.
class UnexpectedError : public ExceptionBase {
public:
    explicit UnexpectedError(const std::string &s)
        : ExceptionBase(LocalMsg{LocalKey("error_is"), {LocalParam(Coercri::UTF8String::fromUTF8Safe(s))}}),
          orig_error_string(s)
    { }

    virtual const char* what() const throw() override {
        return orig_error_string.c_str();
    }

private:
    std::string orig_error_string;
};


#endif
