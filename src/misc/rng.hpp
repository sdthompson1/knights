/*
 * rng.hpp
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
 * Random number generator.
 *
 * getInt, getFloat return number in the range [a,b).
 *
 * Currently we use the Mersenne Twister RNG from Boost.
 *
 * The RNG is now setup to be thread specific. This is useful from a
 * debugging perspective because it means we can replay the exact
 * sequence of random numbers generated in a particular game, given
 * only the starting seed.
 *
 * There is still only one global access point to the RNG (g_rng); you
 * have to call initialize() once in each thread that wants random
 * numbers.
 * 
 */

#ifndef RNG_HPP
#define RNG_HPP

#include "boost/thread/tss.hpp"

class RNGImpl;

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

class RNG {
public:
    void initialize();
    void setSeed(unsigned int seed);
    float getU01();
    bool getBool(float p = 0.5f) { return getU01() <= p; }
    int getInt(int a, int b) { return a + int(getU01() * (b-a)); }
    float getFloat(float a, float b) { return a + getU01() * (b-a); }

private:
    boost::thread_specific_ptr<RNGImpl> pimpl;
};

// for std::random_shuffle compatibility
class RNG_Wrapper {
public:
    RNG_Wrapper(RNG &r_) : r(r_) { }
    int operator()(int n) { return r.getInt(0, n); }
private:
    RNG &r;
};

// global access point
extern RNG g_rng;

#endif      // RNG_HPP
