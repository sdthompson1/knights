/*
 * announcement_loc.hpp
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

#ifndef ANNOUNCEMENT_LOC_HPP
#define ANNOUNCEMENT_LOC_HPP

namespace Coercri {
    class InputByteBuf;
    class OutputByteBuf;
}

class LocalKey;
class LocalParam;

#include <vector>

void WriteAnnouncementLoc(Coercri::OutputByteBuf &buf,
                          const LocalKey &key,
                          const std::vector<LocalParam> &params);

void ReadAnnouncementLoc(Coercri::InputByteBuf &buf,
                         LocalKey &key,
                         std::vector<LocalParam> &params,
                         bool allow_untrusted_strings);

#endif
