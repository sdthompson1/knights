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


-- This is the tile table for the standard knights room files

tile_table = {
   -- These tile numbers are mostly the same as in the original Amiga
   -- Knights (there are one or two changes). 
    
   t_live_pentagram,    -- 1
   t_wall_normal,       -- 2
   t_wall_pillar,       -- 3
   t_wall_skull_east,   -- 4
   t_wall_skull_west,   -- 5
   t_wall_cage,         -- 6
   {t_door_horiz, t_hdoor_background},        -- 7
   {t_door_vert, t_vdoor_background},         -- 8     
   {t_iron_door_horiz, t_hdoor_background},   -- 9
   {t_iron_door_vert, t_vdoor_background},    -- 10
   t_home_south,        -- 11
   t_home_west,         -- 12
   t_home_north,        -- 13
   t_home_east,         -- 14
   t_crystal_ball,      -- 15
   t_gate_horiz,        -- 16
   t_gate_vert,         -- 17
   t_switch_up,         -- 18
   t_switch_down,       -- 19
   nil,              -- 20
   nil,              -- 21
   nil,              -- 22
   t_haystack,          -- 23
   t_barrel,            -- 24
   t_chest_north,       -- 25
   t_chest_east,        -- 26
   t_chest_south,       -- 27
   t_chest_west,        -- 28
   nil,              -- 29
   nil,              -- 30
   nil,              -- 31
   t_small_skull,       -- 32
   nil,              -- 33
   nil,              -- 34
   nil,              -- 35 
   nil,              -- 36 
   nil,              -- 37 
   nil,              -- 38 
   t_table_small,       -- 39
   t_table_north,       -- 40
   t_table_vert,        -- 41
   t_table_south,       -- 42
   t_large_table_horiz, -- 43
   t_chair_south,       -- 44
   t_chair_north,       -- 45
   nil,              -- 46
   t_open_pit_vert,     -- 47
   t_open_pit_wooden,   -- 48
   t_open_pit_normal,   -- 49
   {t_large_table_horiz, i_basic_book},        -- 50 
   {t_large_table_horiz, i_necronomicon},      -- 51 
   nil,              -- 52 
   nil,              -- 53 
   t_broken_wood_1,     -- 54
   t_broken_wood_2,     -- 55
   t_broken_wood_3,     -- 56
   t_broken_wood_4,     -- 57
   t_broken_wood_5,     -- 58
   {t_dead_zombie, t_floor1},       -- 59
   nil,              -- 60
   nil,              -- 61
   nil,              -- 62
   nil,              -- 63
   t_open_gate_horiz,   -- 64
   t_open_gate_vert,    -- 65
   t_floor1,            -- 66
   t_floorpp,           -- 67
   t_floor2,            -- 68
   t_floor3,            -- 69
   t_floor4,            -- 70
   t_floor5,            -- 71
   {t_floor6, t_floor1},            -- 72
   t_floor7,            -- 73
   t_floor8,            -- 74
   t_floor9,            -- 75
   t_dead_pentagram,    -- 76
   t_floor10,           -- 77
   t_closed_pit_vert,   -- 78
   t_closed_pit_wooden, -- 79
   t_closed_pit_normal, -- 80
   t_stairs_top,        -- 81
   t_stairs_south,      -- 82
   t_stairs_west,       -- 83
   t_stairs_north,      -- 84
   t_stairs_east,       -- 85
   t_special_pentagram,     -- 86
   {t_door_horiz_locked, t_hdoor_background}, -- 87
   {t_door_vert_locked, t_vdoor_background}, -- 88
   {t_iron_door_horiz_locked, t_hdoor_background}, -- 89
   {t_iron_door_vert_locked, t_vdoor_background}, -- 90
   {t_floor7, m_vampire_bat},   -- 91

   t_home_south_special,  -- 92
   t_home_west_special,   -- 93
   t_home_north_special,  -- 94
   t_home_east_special    -- 95
}


-- Load the room files

standard_rooms     = kts.LoadSegments(tile_table, "standard_rooms.txt")
gnome_rooms        = kts.LoadSegments(tile_table, "gnome_rooms.txt")
liche_tombs        = kts.LoadSegments(tile_table, "liche_tombs.txt")
guarded_exits      = kts.LoadSegments(tile_table, "guarded_exits.txt")
special_pentagrams = kts.LoadSegments(tile_table, "special_pentagrams.txt")
