/*
 * rng.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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

#include "misc.hpp"

#include "rng.hpp"

#include "boost/random/mersenne_twister.hpp"

#include <cstdlib>
#include <ctime>
using namespace std;

namespace {
    typedef boost::mt19937 gen_type;
}

class RNGImpl {
public:
    gen_type gen;
};


// define g_rng
RNG g_rng;

void RNG::initialize()
{
    pimpl.reset(new RNGImpl);
}

bool RNG::isInitialized() const
{
    return pimpl.get() != 0;
}

void RNG::setSeed(unsigned int seed)
{
    pimpl->gen.seed(seed);
}

float RNG::getU01()
{
    const gen_type::result_type val = pimpl->gen();
    const gen_type::result_type min = pimpl->gen.min();
    const gen_type::result_type max = pimpl->gen.max();
    return float(val - min) / (float(max - min) + 1.0f);
}
