/*
 * make_scroll_area.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

#ifndef MAKE_SCROLL_AREA_HPP
#define MAKE_SCROLL_AREA_HPP

#include "guichan.hpp"
#include <memory>

const int DEFAULT_SCROLLBAR_WIDTH = 16;

std::unique_ptr<gcn::ScrollArea> MakeScrollArea(gcn::Widget &content, int width, int height,
                                              int scrollbar_width = DEFAULT_SCROLLBAR_WIDTH);

#endif
