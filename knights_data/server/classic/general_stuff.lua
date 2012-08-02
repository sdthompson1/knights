--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2012.
-- Copyright (C) Kalle Marjola, 1994.
--
-- Knights is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 3 of the License, or
-- (at your option) any later version.
--
-- Knights is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with Knights.  If not, see <http://www.gnu.org/licenses/>.
--


-- Some things that didn't fit anywhere else

-- Default item carried by a knight
kts.DEFAULT_ITEM = i_sword

-- Anim used by knights (must use the first house colours in the list)
kts.KNIGHT_ANIM = a_purple_knight

-- Knight house colours table
kts.KNIGHT_HOUSE_COLOURS = {
    {0x664477, 0x553366, 0x442255},   -- purple
    {0x116622, 0x115522, 0x114422},   -- green
    {0xaa3300, 0x882200, 0x661100},   -- red
    {0x0000aa, 0x000088, 0x000066},   -- blue
    {0xaa7700, 0x996600, 0x885500},   -- gold
    {0x333333, 0x222222, 0x111111},   -- black
    {0x007755, 0x005544, 0x003333},   -- turquoise

    {0xcc8844, 0xbb7744, 0xaa6644},   -- peach
    {0x993377, 0x882266, 0x771155},   -- pink
    {0x662200, 0x552200, 0x442200},   -- brown
    {0x0077aa, 0x006688, 0x005566},   -- sky blue
    {0xddbb00, 0xbb9900, 0x997700},   -- yellow
}

-- Colours used in the house colours dropdown
kts.KNIGHT_HOUSE_COLOURS_MENU = { 
   0xaa55bb,    -- purple
   0x189933,    -- green
   0xff2200,    -- red
   0x0000ff,    -- blue
   0xffaa00,    -- gold
   0x444444,    -- black
   0x00bb88,    -- turquoise

   0xffc699,    -- peach
   0xff44bb,    -- pink
   0x772800,    -- brown
   0x00b3ff,    -- sky blue
   0xffee00,    -- yellow
}

-- Stuff bag graphic
kts.STUFF_BAG_GRAPHIC = g_stuff_bag

-- Blood and gore
-- (Also note monster corpses and zombie activity sequence are defined in monsters.lua.)
kts.BLOOD_ICON = g_blood_icon
kts.BLOOD_TILES = {t_blood_1, t_blood_2, t_blood_3}
kts.DEAD_KNIGHT_TILES = {t_dead_knight_1, t_dead_knight_2, t_dead_knight_3, t_dead_knight_4}
