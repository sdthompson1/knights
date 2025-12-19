/*
 * exception_base.hpp
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

/*
 * ExceptionBase: subclass of std::exception that contains a LocalKey
 * and params for the error message.
 *
 */

#ifndef MISC_ERROR_HPP
#define MISC_ERROR_HPP

#include "localization.hpp"

#include <exception>
#include <string>

class ExceptionBase : public std::exception {
public:
    explicit ExceptionBase(LocalKey key) : key(key) { }
    ExceptionBase(LocalKey key, std::vector<LocalParam> params) : key(key), params(params) { }
    virtual ~ExceptionBase() throw() { }
    virtual const char *what() const throw() { return key.getKey().c_str(); }
    const LocalKey &getKey() const { return key; }
    const std::vector<LocalParam> &getParams() const { return params; }
private:
    LocalKey key;
    std::vector<LocalParam> params;
};

#endif
