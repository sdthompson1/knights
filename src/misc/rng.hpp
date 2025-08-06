/*
 * rng.hpp
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

#ifndef RNG_HPP
#define RNG_HPP

#include <memory>

// RNG implementation has been simplified as of July 2025. There is
// now one "global" (and thread-safe) RNG implementation, instead of
// one per thread.

class RNGImpl;

class RNG {
public:
    // Initialize using std::random_device
    void initialize();

    // Initialize using a provided sequence of bytes
    // num_bytes must match the size of std::mt19937's state, else this will throw
    void initialize(const char *bytes, int num_bytes);

    // Generate random numbers
    float getU01() { return getFloat(0, 1); }  // return random float in range [0,1)
    bool getBool(float p = 0.5f) { return getU01() <= p; }  // return true with given probability
    int getInt(int a, int b);           // return random int in range [a,b)
    float getFloat(float a, float b);   // return random float in range [a,b)

private:
    std::unique_ptr<RNGImpl> pimpl;
};

// for std::shuffle compatibility
class RNG_Wrapper {
public:
    RNG_Wrapper(RNG &r_) : r(r_) { }
    typedef unsigned int result_type;
    static constexpr unsigned int min() { return 0; }
    static constexpr unsigned int max() { return 0x0fffffff; }
    unsigned int operator()() { return r.getInt(0, max() + 1); }
private:
    RNG &r;
};

// global access point
extern RNG g_rng;

#endif      // RNG_HPP
