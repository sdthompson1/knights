/*
 * random_int.hpp
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

#ifndef KCFG_RANDOM_INT_HPP
#define KCFG_RANDOM_INT_HPP

#include <utility>
#include <vector>

namespace KConfig {

//
// GetRandom function: this is used by the library as a way of
// generating random numbers. It must be defined externally by the
// client. It should return a uniformly distributed integer between 0
// and N-1 (inclusive).
//
int GetRandom(int N);


//
// RandomInt interface
//

class RandomInt {
public:
	virtual ~RandomInt() { }
	virtual int get() const = 0;
};


//
// Subclasses
//

class RIConstant : public RandomInt {
	// Wrapper for a constant (non-random) integer.
public:
	explicit RIConstant(int x_) : x(x_) { }
	void reset(int x_) { x = x_; }
	virtual int get() const { return x; }
private:
	int x;
};

class RIDice : public RandomInt {
	// Dice expression, eg 2d6.
public:
	RIDice(int n_, int d_) : n(n_), d(d_) { }
	virtual int get() const;
private:
	int n, d;
};

class RIList : public RandomInt {
	// A random selection from a list of RandomInts.
public:
	explicit RIList(int nrsrv)
		: total_weight(0) { contents.reserve(nrsrv); }
	void add(const RandomInt *r, int w)
		{ if (r) { contents.push_back(std::make_pair(r,w)); total_weight += w; } }
	virtual int get() const;
	
private:
	std::vector<std::pair<const RandomInt*, int> > contents;
	int total_weight;
};

class RIBinExpr : public RandomInt {
	// Base class for binary operators (+,-,*,/).
	// (Unary minus can be implemented via RISub(0,x).)
public:
	RIBinExpr(const RandomInt *l, const RandomInt *r) : lhs(l), rhs(r) { }
	int getLhs() const { return lhs? lhs->get() : 0; }
	int getRhs() const { return rhs? rhs->get() : 0; }
private:
	const RandomInt *lhs;
	const RandomInt *rhs;
};

class RIAdd : public RIBinExpr {
public:
	RIAdd(const RandomInt *lhs, const RandomInt *rhs) : RIBinExpr(lhs, rhs) { }
	virtual int get() const { return getLhs() + getRhs(); }
};

class RISub : public RIBinExpr {
public:
	RISub(const RandomInt *lhs, const RandomInt *rhs) : RIBinExpr(lhs, rhs) { }
	virtual int get() const { return getLhs() - getRhs(); }
};

class RIMul : public RIBinExpr {
public:
	RIMul(const RandomInt *lhs, const RandomInt *rhs) : RIBinExpr(lhs, rhs) { }
	virtual int get() const { return getLhs() * getRhs(); }
};

class RIDiv : public RIBinExpr {
public:
	RIDiv(const RandomInt *lhs, const RandomInt *rhs) : RIBinExpr(lhs, rhs) { }
	virtual int get() const { return getLhs() / getRhs(); }
};

}

#endif
