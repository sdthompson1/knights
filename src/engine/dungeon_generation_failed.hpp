/*
 * dungeon_generation_failed.hpp
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
 * Exception class that can be thrown during dungeon generation
 *
 */
  
#ifndef DUNGEON_GENERATION_FAILED_HPP
#define DUNGEON_GENERATION_FAILED_HPP

#include <exception>

class DungeonGenerationFailed : public std::exception {
public:
    // (TODO) probably should add more helpful error messages to this, since the dungeon
    // generator can fail (in several different ways!) if there is bad input. (e.g. if
    // you ask for required items but have not got any DungeonStuff directives with
    // weights given.)
    DungeonGenerationFailed() { } 
    virtual const char * what() const throw() { return "dungeon generator failure"; }
};

#endif
