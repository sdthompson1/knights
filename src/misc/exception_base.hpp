/*
 * exception_base.hpp
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
 * ExceptionBase: subclass of std::exception that defines what() for
 * you.
 *
 */

#ifndef MISC_ERROR_HPP
#define MISC_ERROR_HPP

#include <exception>
#include <string>

class ExceptionBase : public std::exception {
public:
    explicit ExceptionBase(const std::string &s_) : s(s_) { }
    virtual ~ExceptionBase() throw() { }
    virtual const char *what() const throw() { return s.c_str(); }
private:
    std::string s;
};

#endif
