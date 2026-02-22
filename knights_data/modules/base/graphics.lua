--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2026.
-- Copyright (C) Kalle Marjola, 1994.
--
-- Knights is free software: you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation, either version 2 of the License, or
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



-- GRAPHICS:


-- Blood/gore

g_blood_icon    = kts.Graphic("gfx/blood_icon.bmp", 0,0,0)
g_blood_1       = kts.Graphic("gfx/blood_1.bmp", 0,0,0)
g_blood_2       = kts.Graphic("gfx/blood_2.bmp", 0,0,0)
g_blood_3       = kts.Graphic("gfx/blood_3.bmp", 0,0,0)
g_dead_knight_1 = kts.Graphic("gfx/dead_knight_1.bmp")
g_dead_knight_2 = kts.Graphic("gfx/dead_knight_2.bmp")
g_dead_knight_3 = kts.Graphic("gfx/dead_knight_3.bmp")
g_dead_knight_4 = kts.Graphic("gfx/dead_knight_4.bmp")
g_dead_vbat_1   = kts.Graphic("gfx/dead_vbat_1.bmp", 0,0,0)
g_dead_vbat_2   = kts.Graphic("gfx/dead_vbat_2.bmp", 0,0,0)
g_dead_vbat_3   = kts.Graphic("gfx/dead_vbat_3.bmp", 0,0,0)


-- Items
g_blade_trap     = kts.Graphic("gfx/blade_trap.bmp",0,0,0,-4,-4)
g_bolts          = kts.Graphic("gfx/bolts.bmp",0,0,0,-3,-4)
g_book           = kts.Graphic("gfx/book.bmp",0,0,0,-3,-4)
g_closed_bear_trap = kts.Graphic("gfx/closed_bear_trap.bmp",0,0,0,-4,-4)
g_crossbow       = kts.Graphic("gfx/crossbow.bmp",0,0,0,-3,-2)
g_dagger         = kts.Graphic("gfx/dagger.bmp",0,0,0,-4,-4)
g_daggers        = kts.Graphic("gfx/daggers.bmp",0,0,0,-2,-4)
g_gem            = kts.Graphic("gfx/gem.bmp",0,0,0,-5,-4)
g_hammer         = kts.Graphic("gfx/hammer.bmp",0,0,0,-2,-2)
g_key            = kts.Graphic("gfx/key.bmp",0,0,0,-4,-5)
g_open_bear_trap = kts.Graphic("gfx/open_bear_trap.bmp",0,0,0,-3,-2)
g_poison_trap    = kts.Graphic("gfx/poison_trap.bmp",0,0,0,-6,-4)
g_potion         = kts.Graphic("gfx/potion.bmp",0,0,0,-5,-4)
g_scroll         = kts.Graphic("gfx/scroll.bmp",0,0,0,-4,-4)
g_staff          = kts.Graphic("gfx/staff.bmp",0,0,0,-1,-2)
g_stuff_bag      = kts.Graphic("gfx/stuff_bag.bmp",0,0,0,-4,-4)
g_wand           = kts.Graphic("gfx/wand.bmp",0,0,0,-3,-6)
g_axe            = kts.Graphic("gfx/axe.bmp", 0, 0, 0, -2, -2)



-- Item overlays

g_axe_north = kts.Graphic("gfx/axe_north.bmp",0,0,0, -6,-4)
g_axe_east  = kts.Graphic("gfx/axe_east.bmp",0,0,0,   0,-6)
g_axe_south = kts.Graphic("gfx/axe_south.bmp",0,0,0, -7, 0)
g_axe_west  = kts.Graphic("gfx/axe_west.bmp",0,0,0,  -4,-7)

g_beartrap_north = kts.Graphic("gfx/beartrap_north.bmp",0,0,0, -6,-7)
g_beartrap_east  = kts.Graphic("gfx/beartrap_east.bmp",0,0,0,   0,-6)
g_beartrap_south = kts.Graphic("gfx/beartrap_south.bmp",0,0,0, -5, 0)
g_beartrap_west  = kts.Graphic("gfx/beartrap_west.bmp",0,0,0,  -7,-5)

g_bolt_horiz = kts.Graphic("gfx/bolt_horiz.bmp",0,0,0, -3, -6)
g_bolt_vert  = kts.Graphic("gfx/bolt_vert.bmp",0,0,0, -7, -3)

g_book_north = kts.Graphic("gfx/book_north.bmp",0,0,0, -6,-8)
g_book_east  = kts.Graphic("gfx/book_east.bmp",0,0,0,   0,-6)
g_book_south = kts.Graphic("gfx/book_south.bmp",0,0,0, -5, 0)
g_book_west  = kts.Graphic("gfx/book_west.bmp",0,0,0,  -8,-5)

g_dagger_north = kts.Graphic("gfx/dagger_north.bmp",0,0,0, -6,-9)
g_dagger_east  = kts.Graphic("gfx/dagger_east.bmp",0,0,0,   0,-6)
g_dagger_south = kts.Graphic("gfx/dagger_south.bmp",0,0,0, -7, 0)
g_dagger_west  = kts.Graphic("gfx/dagger_west.bmp",0,0,0,  -9,-7)

g_hammer_north = kts.Graphic("gfx/hammer_north.bmp",0,0,0, -5,-4)
g_hammer_east  = kts.Graphic("gfx/hammer_east.bmp",0,0,0,   0,-5)
g_hammer_south = kts.Graphic("gfx/hammer_south.bmp",0,0,0, -6, 0)
g_hammer_west  = kts.Graphic("gfx/hammer_west.bmp",0,0,0,  -4,-6)

g_staff_north = kts.Graphic("gfx/staff_north.bmp",0,0,0, -6,-1)
g_staff_east  = kts.Graphic("gfx/staff_east.bmp",0,0,0,   0,-6)
g_staff_south = kts.Graphic("gfx/staff_south.bmp",0,0,0, -7, 0)
g_staff_west  = kts.Graphic("gfx/staff_west.bmp",0,0,0,  -1,-7)

g_sword_north = kts.Graphic("gfx/sword_north.bmp",0,0,0, -6,-3)
g_sword_east  = kts.Graphic("gfx/sword_east.bmp",0,0,0,   0,-6)
g_sword_south = kts.Graphic("gfx/sword_south.bmp",0,0,0, -7, 0)
g_sword_west  = kts.Graphic("gfx/sword_west.bmp",0,0,0,  -3,-7)

g_wand_north = kts.Graphic("gfx/wand_north.bmp",0,0,0, -6,-5)
g_wand_east  = kts.Graphic("gfx/wand_east.bmp",0,0,0,   0,-6)
g_wand_south = kts.Graphic("gfx/wand_south.bmp",0,0,0, -7, 0)
g_wand_west  = kts.Graphic("gfx/wand_west.bmp",0,0,0,  -5,-7)

-- Knights

g_ktp1n            = kts.Graphic("gfx/ktp1n.bmp",0,0,0)
g_ktp1e            = kts.Graphic("gfx/ktp1e.bmp",0,0,0)
g_ktp1s            = kts.Graphic("gfx/ktp1s.bmp",0,0,0)
g_ktp1w            = kts.Graphic("gfx/ktp1w.bmp",0,0,0)
g_ktp2n            = kts.Graphic("gfx/ktp2n.bmp",0,0,0)
g_ktp2e            = kts.Graphic("gfx/ktp2e.bmp",0,0,0)
g_ktp2s            = kts.Graphic("gfx/ktp2s.bmp",0,0,0)
g_ktp2w            = kts.Graphic("gfx/ktp2w.bmp",0,0,0)
g_ktp3n            = kts.Graphic("gfx/ktp3n.bmp",0,0,0)
g_ktp3e            = kts.Graphic("gfx/ktp3e.bmp",0,0,0)
g_ktp3s            = kts.Graphic("gfx/ktp3s.bmp",0,0,0)
g_ktp3w            = kts.Graphic("gfx/ktp3w.bmp",0,0,0)
g_ktp4n            = kts.Graphic("gfx/ktp4n.bmp",0,0,0)
g_ktp4e            = kts.Graphic("gfx/ktp4e.bmp",0,0,0)
g_ktp4s            = kts.Graphic("gfx/ktp4s.bmp",0,0,0)
g_ktp4w            = kts.Graphic("gfx/ktp4w.bmp",0,0,0)
g_ktp5n            = kts.Graphic("gfx/ktp5n.bmp",0,0,0)
g_ktp5e            = kts.Graphic("gfx/ktp5e.bmp",0,0,0)
g_ktp5s            = kts.Graphic("gfx/ktp5s.bmp",0,0,0)
g_ktp5w            = kts.Graphic("gfx/ktp5w.bmp",0,0,0)
g_ktp6n            = kts.Graphic("gfx/ktp6n.bmp",0,0,0)
g_ktp6e            = kts.Graphic("gfx/ktp6e.bmp",0,0,0)
g_ktp6s            = kts.Graphic("gfx/ktp6s.bmp",0,0,0)
g_ktp6w            = kts.Graphic("gfx/ktp6w.bmp",0,0,0)
g_ktp7n            = kts.Graphic("gfx/ktp7n.bmp",0,0,0)
g_ktp7e            = kts.Graphic("gfx/ktp7e.bmp",0,0,0)
g_ktp7s            = kts.Graphic("gfx/ktp7s.bmp",0,0,0)
g_ktp7w            = kts.Graphic("gfx/ktp7w.bmp",0,0,0)
g_ktp8n            = kts.Graphic("gfx/ktp8n.bmp",0,0,0)
g_ktp8e            = kts.Graphic("gfx/ktp8e.bmp",0,0,0)
g_ktp8s            = kts.Graphic("gfx/ktp8s.bmp",0,0,0)
g_ktp8w            = kts.Graphic("gfx/ktp8w.bmp",0,0,0)


-- Menus and Inventory Icons

g_inv_bolt         = kts.Graphic("gfx/inv_bolt.bmp")
g_inv_dagger       = kts.Graphic("gfx/inv_dagger.bmp")
g_inv_key1         = kts.Graphic("gfx/inv_key1.bmp")
g_inv_key2         = kts.Graphic("gfx/inv_key2.bmp")
g_inv_key3         = kts.Graphic("gfx/inv_key3.bmp")
g_inv_lockpicks    = kts.Graphic("gfx/inv_lockpicks.bmp")
g_inv_overdraw_big = kts.Graphic("gfx/inv_overdraw.bmp",0,0,0,0,-14)
g_inv_overdraw_small = kts.Graphic("gfx/inv_overdraw.bmp",0,0,0,0,-6)
g_menu_axe         = kts.Graphic("gfx/menu_axe.bmp")
g_menu_beartrap    = kts.Graphic("gfx/menu_beartrap.bmp")
g_menu_blade_trap  = kts.Graphic("gfx/menu_blade_trap.bmp")
g_menu_crossbow    = kts.Graphic("gfx/menu_crossbow.bmp")
g_menu_dagger      = kts.Graphic("gfx/menu_dagger.bmp")
g_menu_drop        = kts.Graphic("gfx/menu_drop.bmp")
g_menu_drop_gem    = kts.Graphic("gfx/menu_drop_gem.bmp")
g_menu_fist        = kts.Graphic("gfx/menu_fist.bmp")
g_menu_lockpicks   = kts.Graphic("gfx/menu_lockpicks.bmp")
g_menu_open_close  = kts.Graphic("gfx/menu_open_close.bmp")
g_menu_pickup      = kts.Graphic("gfx/menu_pickup.bmp")
g_menu_poison_trap = kts.Graphic("gfx/menu_poison_trap.bmp")
g_menu_suicide     = kts.Graphic("gfx/menu_suicide.bmp")


-- Tiles

g_barrel           = kts.Graphic("gfx/barrel.bmp")
g_broken_wood_1    = kts.Graphic("gfx/broken_wood_1.bmp")
g_broken_wood_2    = kts.Graphic("gfx/broken_wood_2.bmp")
g_broken_wood_3    = kts.Graphic("gfx/broken_wood_3.bmp")
g_broken_wood_4    = kts.Graphic("gfx/broken_wood_4.bmp")
g_broken_wood_5    = kts.Graphic("gfx/broken_wood_5.bmp")
g_cage             = kts.Graphic("gfx/cage.bmp")
g_chair_north      = kts.Graphic("gfx/chair_north.bmp")
g_chair_south      = kts.Graphic("gfx/chair_south.bmp")
g_chair_east       = kts.Graphic("gfx/chair_east.bmp")
g_chair_west       = kts.Graphic("gfx/chair_west.bmp")
g_chest_east       = kts.Graphic("gfx/chest_east.bmp")
g_chest_north      = kts.Graphic("gfx/chest_north.bmp")
g_chest_south      = kts.Graphic("gfx/chest_south.bmp")
g_chest_west       = kts.Graphic("gfx/chest_west.bmp")
g_crystal_ball     = kts.Graphic("gfx/crystal_ball.bmp")
g_dead_zombie      = kts.Graphic("gfx/dead_zombie.bmp")
g_door_hgc         = kts.Graphic("gfx/door_hgc.bmp",0,0,0)
g_door_hgo         = kts.Graphic("gfx/door_hgo.bmp",0,0,0)
g_door_vgc         = kts.Graphic("gfx/door_vgc.bmp",0,0,0)
g_door_vgo         = kts.Graphic("gfx/door_vgo.bmp",0,0,0)
g_door_hic         = kts.Graphic("gfx/door_hic.bmp",0,0,0)
g_door_hio         = kts.Graphic("gfx/door_hio.bmp",0,0,0)
g_door_vic         = kts.Graphic("gfx/door_vic.bmp",0,0,0)
g_door_vio         = kts.Graphic("gfx/door_vio.bmp",0,0,0)
g_door_hwc         = kts.Graphic("gfx/door_hwc.bmp",0,0,0)
g_door_hwo         = kts.Graphic("gfx/door_hwo.bmp",0,0,0)
g_door_vwc         = kts.Graphic("gfx/door_vwc.bmp",0,0,0)
g_door_vwo         = kts.Graphic("gfx/door_vwo.bmp",0,0,0)
g_floor1           = kts.Graphic("gfx/floor1.bmp")
g_floor2           = kts.Graphic("gfx/floor2.bmp")
g_floor3           = kts.Graphic("gfx/floor3.bmp")
g_floor4           = kts.Graphic("gfx/floor4.bmp")
g_floor5           = kts.Graphic("gfx/floor5.bmp")
g_floor6           = kts.Graphic("gfx/floor6.bmp")
g_floor7           = kts.Graphic("gfx/floor7.bmp")
g_floor8           = kts.Graphic("gfx/floor8.bmp")
g_floor9           = kts.Graphic("gfx/floor9.bmp")
g_floor10          = kts.Graphic("gfx/floor10.bmp")
g_haystack         = kts.Graphic("gfx/haystack.bmp")
g_hdoor_background = kts.Graphic("gfx/hdoor_background.bmp")
g_home_east        = kts.Graphic("gfx/home_east.bmp")
g_home_north       = kts.Graphic("gfx/home_north.bmp")
g_home_south       = kts.Graphic("gfx/home_south.bmp")
g_home_west        = kts.Graphic("gfx/home_west.bmp")
g_open_chest_east  = kts.Graphic("gfx/open_chest_east.bmp")
g_open_chest_north = kts.Graphic("gfx/open_chest_north.bmp")
g_open_chest_south = kts.Graphic("gfx/open_chest_south.bmp")
g_open_chest_west  = kts.Graphic("gfx/open_chest_west.bmp")
g_pentagram        = kts.Graphic("gfx/pentagram.bmp")
g_pillar           = kts.Graphic("gfx/pillar.bmp")
g_pit_c            = kts.Graphic("gfx/pit_c.bmp")
g_pit_o            = kts.Graphic("gfx/pit_o.bmp")
g_pitv_c           = kts.Graphic("gfx/pitv_c.bmp")
g_pitv_o           = kts.Graphic("gfx/pitv_o.bmp")
g_pith_c           = kts.Graphic("gfx/pith_c.bmp")
g_pith_o           = kts.Graphic("gfx/pith_o.bmp")
g_pressure_plate   = kts.Graphic("gfx/pressure_plate.bmp")
g_skull_left       = kts.Graphic("gfx/skull_left.bmp")
g_skull_right      = kts.Graphic("gfx/skull_right.bmp")
g_skull_up         = kts.Graphic("gfx/skull_up.bmp")
g_skull_down       = kts.Graphic("gfx/skull_down.bmp")
g_small_skull      = kts.Graphic("gfx/small_skull.bmp")
g_stairs_east      = kts.Graphic("gfx/stairs_east.bmp")
g_stairs_north     = kts.Graphic("gfx/stairs_north.bmp")
g_stairs_south     = kts.Graphic("gfx/stairs_south.bmp")
g_stairs_top       = kts.Graphic("gfx/stairs_top.bmp")
g_stairs_west      = kts.Graphic("gfx/stairs_west.bmp")
g_switch_down      = kts.Graphic("gfx/switch_down.bmp")
g_switch_up        = kts.Graphic("gfx/switch_up.bmp")
g_large_table_horiz = kts.Graphic("gfx/large_table_horiz.bmp")
g_large_table_vert  = kts.Graphic("gfx/large_table_vert.bmp")
g_table_north      = kts.Graphic("gfx/table_north.bmp")
g_table_small      = kts.Graphic("gfx/table_small.bmp")
g_table_south      = kts.Graphic("gfx/table_south.bmp")
g_table_vert       = kts.Graphic("gfx/table_vert.bmp")
g_table_west       = kts.Graphic("gfx/table_west.bmp")
g_table_horiz      = kts.Graphic("gfx/table_horiz.bmp")
g_table_east       = kts.Graphic("gfx/table_east.bmp")
g_vdoor_background = kts.Graphic("gfx/vdoor_background.bmp")
g_wall             = kts.Graphic("gfx/wall.bmp")
g_wooden_floor     = kts.Graphic("gfx/wooden_floor.bmp")
g_wooden_pit       = kts.Graphic("gfx/wooden_pit.bmp")


-- Vampire Bats

g_vbat_1    = kts.Graphic("gfx/vbat1.bmp", 0,0,0)
g_vbat_2    = kts.Graphic("gfx/vbat2.bmp", 0,0,0)
g_vbat_3    = kts.Graphic("gfx/vbat3.bmp", 0,0,0)
g_vbat_bite = kts.Graphic("gfx/vbatbite.bmp", 0,0,0)


-- Zombies

g_zom1n = kts.Graphic("gfx/zom1n.bmp", 0,0,0)
g_zom1e = kts.Graphic("gfx/zom1e.bmp", 0,0,0)
g_zom1s = kts.Graphic("gfx/zom1s.bmp", 0,0,0)
g_zom1w = kts.Graphic("gfx/zom1w.bmp", 0,0,0)
g_zom2n = kts.Graphic("gfx/zom2n.bmp", 0,0,0)
g_zom2e = kts.Graphic("gfx/zom2e.bmp", 0,0,0)
g_zom2s = kts.Graphic("gfx/zom2s.bmp", 0,0,0)
g_zom2w = kts.Graphic("gfx/zom2w.bmp", 0,0,0)
g_zom3n = kts.Graphic("gfx/zom3n.bmp", 0,0,0, 0,16)
g_zom3e = kts.Graphic("gfx/zom3e.bmp", 0,0,0)
g_zom3s = kts.Graphic("gfx/zom3s.bmp", 0,0,0)
g_zom3w = kts.Graphic("gfx/zom3w.bmp", 0,0,0, 16,0)
g_zom4n = kts.Graphic("gfx/zom4n.bmp", 0,0,0)
g_zom4e = kts.Graphic("gfx/zom4e.bmp", 0,0,0)
g_zom4s = kts.Graphic("gfx/zom4s.bmp", 0,0,0)
g_zom4w = kts.Graphic("gfx/zom4w.bmp", 0,0,0)




-- OVERLAY OFFSETS:

-- Note: these must be set BEFORE creating any Anims, otherwise the
-- overlays won't work.

-- TODO: Probably want a better way of handling offsets in general...

kts.SetOverlayOffsets(

-- Normal
    "west", -250, -188,
    "north", 188, -250,
    "east", 250, 188,
    "south", -188, 250,

-- Melee backswing
    "south", 188, 625,
    "west", -625, 188,
    "north", -188, -625,
    "east", 625, -188,

-- Melee downswing
    "north", 0, -1000,
    "east", 1000, 0,
    "south", 0, 1000,
    "west", -1000, 0,

-- Parry
    "west", -250, -125,
    "north", 125, -250,
    "east", 250, 125,
    "south", -125, 250,

-- Throwing backswing
    "south", 188, 625,
    "west", -625, 188,
    "north", -188, -625,
    "east", 625, -188
)
