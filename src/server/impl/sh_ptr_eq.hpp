/*
 * sh_ptr_eq.hpp
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

#ifndef SH_PTR_EQ_HPP
#define SH_PTR_EQ_HPP

#include "boost/shared_ptr.hpp"

template<class T>
struct ShPtrEq {
    explicit ShPtrEq(T*p_) : p(p_) { }
    bool operator()(boost::shared_ptr<T> q) const { return p == q.get(); }
    T* p;
};

#endif
