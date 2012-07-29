/*
 * misc.hpp
 * Header included by all .cpp files.
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
 *
 */

#ifndef MISC_HPP
#define MISC_HPP

#ifdef NDEBUG

// Disable assertions
#define ASSERT(x)

#elif defined(BOOST_ENABLE_ASSERT_HANDLER)

// Use boost::assert
#include "boost/assert.hpp"
#define ASSERT(x) BOOST_ASSERT(x)

#else

// Use C standard assert command
#include <assert.h>
#define ASSERT(x) assert(x)

#endif

#endif

