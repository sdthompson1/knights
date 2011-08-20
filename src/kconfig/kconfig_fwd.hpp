/*
 * kconfig_fwd.hpp
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

/*
 * Forward declarations of various things
 *
 */

#ifndef KCFG_KCONFIG_FWD_HPP
#define KCFG_KCONFIG_FWD_HPP

namespace KConfig {

	class KConfigError;
	class KConfigLoader;
	class KConfigSource;
	class KFile;
	class RandomInt;
	class RandomIntContainer;
	class Value;
	
	int GetRandom(int N);  // this must be defined by the host program (generate random no. between 0 and N-1)
}

#endif
 
