/*
 * rng.cpp
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

#include "misc.hpp"

#include "rng.hpp"

#ifndef VIRTUAL_SERVER
#include "boost/thread/mutex.hpp"
#include "boost/thread/locks.hpp"
#endif

#include <cstring>
#include <random>
#include <stdexcept>

RNG g_rng;

class RNGImpl {
public:
    RNGImpl(std::seed_seq &seed);
    float getFloat(float a, float b);
    int getInt(int a, int b);

private:
#ifndef VIRTUAL_SERVER
    boost::mutex mutex;
#endif
    std::mt19937 rng;
};

RNGImpl::RNGImpl(std::seed_seq &seed)
    : rng(seed)
{}

void RNG::initialize()
{
    // Generate 32 random seed bytes
    std::random_device rd;
    unsigned char bytes[32];
    for (int i = 0; i < sizeof(bytes); ++i) {
        bytes[i] = rd();
    }

    // Initialize using that seed
    initialize(bytes, sizeof(bytes));
}

void RNG::initialize(const unsigned char *bytes, int num_bytes)
{
    // Initialize using the provided bytes
    std::seed_seq seq(bytes, bytes + num_bytes);
    pimpl = std::make_unique<RNGImpl>(seq);
}

float RNGImpl::getFloat(float a, float b)
{
    if (b <= a) {
        throw std::invalid_argument("RNG::getFloat: empty range");
    }

    std::uniform_real_distribution<float> dist(a, b);
    float x;

    {
#ifndef VIRTUAL_SERVER
        boost::unique_lock lock(mutex);
#endif
        x = dist(rng);
    }

    // Some implementations of uniform_real_distribution might return values
    // outside the expected range (e.g. they might return exactly 'b'). We don't
    // want to return a value outside the [a,b) range, so we do an explicit
    // range check:
    if (a <= x && x < b) {
        return x;
    } else {
        // This should happen only extremely rarely (if at all) so just returning
        // 'a' in these (rare) cases should be fine
        return a;
    }
}

int RNGImpl::getInt(int a, int b)
{
    if (b <= a) {
        throw std::invalid_argument("RNG::getInt: empty range");
    }
    std::uniform_int_distribution<int> dist(a, b-1);
#ifndef VIRTUAL_SERVER
    boost::unique_lock lock(mutex);
#endif
    return dist(rng);
}

float RNG::getFloat(float a, float b)
{
    return pimpl->getFloat(a, b);
}

int RNG::getInt(int a, int b)
{
    return pimpl->getInt(a, b);
}
