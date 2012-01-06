/*
 * server_status_display.hpp
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

#ifndef SERVER_STATUS_DISPLAY_HPP
#define SERVER_STATUS_DISPLAY_HPP

#include "status_display.hpp"

#include <vector>

class ServerStatusDisplay : public StatusDisplay {
public:
    typedef unsigned char ubyte;
    explicit ServerStatusDisplay(std::vector<ubyte> &out_) : out(out_) { }

    virtual void setBackpack(int slot, const Graphic *gfx, const Graphic *overdraw, int no_carried, int no_max);
    virtual void addSkull();
    virtual void setHealth(int h);
    virtual void setPotionMagic(PotionMagic pm, bool poison_immunity);
    virtual void setQuestMessage(const std::string &msg);
    virtual void setQuestIcons(const std::vector<StatusDisplay::QuestIconInfo> &icons);
    
private:
    std::vector<ubyte> &out;
};

#endif
