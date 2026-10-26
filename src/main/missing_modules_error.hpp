/*
 * missing_modules_error.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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
 * MissingModulesError: thrown when trying to join a game that uses
 * modules we don't have installed locally. KnightsApp catches this
 * and goes to the MissingModsScreen.
 *
 */

#ifndef MISSING_MODULES_ERROR_HPP
#define MISSING_MODULES_ERROR_HPP

#include "exception_base.hpp"

#include <string>
#include <vector>

class MissingModulesError : public ExceptionBase {
public:
    explicit MissingModulesError(std::vector<std::string> names)
        : ExceptionBase(LocalMsg(LocalKey("missing_modules_game"))),
          mod_vfs_names(std::move(names))
    { }

    const std::vector<std::string> & getModVfsNames() const { return mod_vfs_names; }

private:
    std::vector<std::string> mod_vfs_names;
};

#endif
