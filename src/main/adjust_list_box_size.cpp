/*
 * adjust_list_box_size.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
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

#include "adjust_list_box_size.hpp"

void AdjustListBoxSize(gcn::ListBox &listbox, gcn::ScrollArea &scrollarea)
{
    gcn::ListModel *model = listbox.getListModel();
    gcn::Font *font = listbox.getFont();
    if (!model || !font) return;
    
    listbox.adjustSize();  // adjusts the vertical size only.

    // work out min width for the listbox. this depends on whether the
    // vertical scrollbar is being shown or not.
    const bool scrollbar_shown = listbox.getHeight() > scrollarea.getHeight();
    int width = scrollarea.getWidth();
    if (scrollbar_shown) width -= scrollarea.getScrollbarWidth();
    
    // compute the horizontal size as the max of horiz sizes of each row.
    const int nels = model->getNumberOfElements();
    for (int i = 0; i < nels; ++i) {
        width = std::max(width, font->getWidth(model->getElementAt(i)));
    }

    listbox.setWidth(width);
}
