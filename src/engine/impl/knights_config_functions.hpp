/*
 * knights_config_functions.hpp
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

#ifndef KNIGHTS_CONFIG_FUNCTIONS_HPP
#define KNIGHTS_CONFIG_FUNCTIONS_HPP

#include "kconfig_fwd.hpp"

class ItemType;
class KnightsConfigImpl;


// Pop an ItemType definition from the KFile and use it to initialize *it (calls it->construct)
void PopKFileItemType(KnightsConfigImpl *kcfg, KConfig::KFile *kf, ItemType *it);


#endif
