/*
 * copy_if.hpp
 * The missing STL algorithm (Stroustrup, p 530)
 *
 */

#ifndef COPY_IF_HPP
#define COPY_IF_HPP

#include "misc.hpp"

template<class In, class Out, class Pred> Out copy_if(In first, In last, Out res, Pred p)
{
	while (first != last) {
		if (p(*first)) *res++ = *first;
		++first;
	}
	return res;
}

#endif
