/*
 * version.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#ifndef VERSION_HPP
#define VERSION_HPP

#define KNIGHTS_VERSION "018"
#define KNIGHTS_VERSION_NUM 18
#define COMPATIBLE_VERSION_NUM 16   // Lowest client version that can connect to this server

#ifdef WIN32
#define KNIGHTS_PLATFORM "Windows"
#else
#define KNIGHTS_PLATFORM "Unix"
#endif

#define KNIGHTS_WEBSITE "http://www.knightsgame.org.uk/"


#endif
