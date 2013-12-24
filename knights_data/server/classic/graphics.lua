--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2013.
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



-- GRAPHICS:


-- Blood/gore

g_blood_icon    = kts.Graphic("+blood_icon.bmp", 0,0,0)
g_blood_1       = kts.Graphic("+blood_1.bmp", 0,0,0)
g_blood_2       = kts.Graphic("+blood_2.bmp", 0,0,0)
g_blood_3       = kts.Graphic("+blood_3.bmp", 0,0,0)
g_dead_knight_1 = kts.Graphic("+dead_knight_1.bmp")
g_dead_knight_2 = kts.Graphic("+dead_knight_2.bmp")
g_dead_knight_3 = kts.Graphic("+dead_knight_3.bmp")
g_dead_knight_4 = kts.Graphic("+dead_knight_4.bmp")
g_dead_vbat_1   = kts.Graphic("+dead_vbat_1.bmp", 0,0,0)
g_dead_vbat_2   = kts.Graphic("+dead_vbat_2.bmp", 0,0,0)
g_dead_vbat_3   = kts.Graphic("+dead_vbat_3.bmp", 0,0,0)


-- Items
g_blade_trap     = kts.Graphic("+blade_trap.bmp",0,0,0,-4,-4)
g_bolts          = kts.Graphic("+bolts.bmp",0,0,0,-3,-4)
g_book           = kts.Graphic("+book.bmp",0,0,0,-3,-4)
g_closed_bear_trap = kts.Graphic("+closed_bear_trap.bmp",0,0,0,-4,-4)
g_crossbow       = kts.Graphic("+crossbow.bmp",0,0,0,-3,-2)
g_dagger         = kts.Graphic("+dagger.bmp",0,0,0,-4,-4)
g_daggers        = kts.Graphic("+daggers.bmp",0,0,0,-2,-4)
g_gem            = kts.Graphic("+gem.bmp",0,0,0,-5,-4)
g_gem_main_menu  = kts.Graphic("+gem.bmp",0,0,0,0,0)
g_hammer         = kts.Graphic("+hammer.bmp",0,0,0,-2,-2)
g_key            = kts.Graphic("+key.bmp",0,0,0,-4,-5)
g_key_main_menu  = kts.Graphic("+key.bmp",0,0,0,0,0)
g_open_bear_trap = kts.Graphic("+open_bear_trap.bmp",0,0,0,-3,-2)
g_poison_trap    = kts.Graphic("+poison_trap.bmp",0,0,0,-6,-4)
g_potion         = kts.Graphic("+potion.bmp",0,0,0,-5,-4)
g_scroll         = kts.Graphic("+scroll.bmp",0,0,0,-4,-4)
g_staff          = kts.Graphic("+staff.bmp",0,0,0,-1,-2)
g_stuff_bag      = kts.Graphic("+stuff_bag.bmp",0,0,0,-4,-4)
g_wand           = kts.Graphic("+wand.bmp",0,0,0,-3,-6)
g_axe            = kts.Graphic("+axe.bmp", 0, 0, 0, -2, -2)



-- Item overlays

g_axe_north = kts.Graphic("+axe_north.bmp",0,0,0, -6,-4)
g_axe_east  = kts.Graphic("+axe_east.bmp",0,0,0,   0,-6)
g_axe_south = kts.Graphic("+axe_south.bmp",0,0,0, -7, 0)
g_axe_west  = kts.Graphic("+axe_west.bmp",0,0,0,  -4,-7)

g_beartrap_north = kts.Graphic("+beartrap_north.bmp",0,0,0, -6,-7)
g_beartrap_east  = kts.Graphic("+beartrap_east.bmp",0,0,0,   0,-6)
g_beartrap_south = kts.Graphic("+beartrap_south.bmp",0,0,0, -5, 0)
g_beartrap_west  = kts.Graphic("+beartrap_west.bmp",0,0,0,  -7,-5)

g_bolt_horiz = kts.Graphic("+bolt_horiz.bmp",0,0,0, -3, -6)
g_bolt_vert  = kts.Graphic("+bolt_vert.bmp",0,0,0, -7, -3)

g_book_north = kts.Graphic("+book_north.bmp",0,0,0, -6,-8)
g_book_east  = kts.Graphic("+book_east.bmp",0,0,0,   0,-6)
g_book_south = kts.Graphic("+book_south.bmp",0,0,0, -5, 0)
g_book_west  = kts.Graphic("+book_west.bmp",0,0,0,  -8,-5)

g_dagger_north = kts.Graphic("+dagger_north.bmp",0,0,0, -6,-9)
g_dagger_east  = kts.Graphic("+dagger_east.bmp",0,0,0,   0,-6)
g_dagger_south = kts.Graphic("+dagger_south.bmp",0,0,0, -7, 0)
g_dagger_west  = kts.Graphic("+dagger_west.bmp",0,0,0,  -9,-7)

g_hammer_north = kts.Graphic("+hammer_north.bmp",0,0,0, -5,-4)
g_hammer_east  = kts.Graphic("+hammer_east.bmp",0,0,0,   0,-5)
g_hammer_south = kts.Graphic("+hammer_south.bmp",0,0,0, -6, 0)
g_hammer_west  = kts.Graphic("+hammer_west.bmp",0,0,0,  -4,-6)

g_staff_north = kts.Graphic("+staff_north.bmp",0,0,0, -6,-1)
g_staff_east  = kts.Graphic("+staff_east.bmp",0,0,0,   0,-6)
g_staff_south = kts.Graphic("+staff_south.bmp",0,0,0, -7, 0)
g_staff_west  = kts.Graphic("+staff_west.bmp",0,0,0,  -1,-7)

g_sword_north = kts.Graphic("+sword_north.bmp",0,0,0, -6,-3)
g_sword_east  = kts.Graphic("+sword_east.bmp",0,0,0,   0,-6)
g_sword_south = kts.Graphic("+sword_south.bmp",0,0,0, -7, 0)
g_sword_west  = kts.Graphic("+sword_west.bmp",0,0,0,  -3,-7)

g_wand_north = kts.Graphic("+wand_north.bmp",0,0,0, -6,-5)
g_wand_east  = kts.Graphic("+wand_east.bmp",0,0,0,   0,-6)
g_wand_south = kts.Graphic("+wand_south.bmp",0,0,0, -7, 0)
g_wand_west  = kts.Graphic("+wand_west.bmp",0,0,0,  -5,-7)

-- Knights

g_ktp1n            = kts.Graphic("+ktp1n.bmp",0,0,0)
g_ktp1e            = kts.Graphic("+ktp1e.bmp",0,0,0)
g_ktp1s            = kts.Graphic("+ktp1s.bmp",0,0,0)
g_ktp1w            = kts.Graphic("+ktp1w.bmp",0,0,0)
g_ktp2n            = kts.Graphic("+ktp2n.bmp",0,0,0)
g_ktp2e            = kts.Graphic("+ktp2e.bmp",0,0,0)
g_ktp2s            = kts.Graphic("+ktp2s.bmp",0,0,0)
g_ktp2w            = kts.Graphic("+ktp2w.bmp",0,0,0)
g_ktp3n            = kts.Graphic("+ktp3n.bmp",0,0,0)
g_ktp3e            = kts.Graphic("+ktp3e.bmp",0,0,0)
g_ktp3s            = kts.Graphic("+ktp3s.bmp",0,0,0)
g_ktp3w            = kts.Graphic("+ktp3w.bmp",0,0,0)
g_ktp4n            = kts.Graphic("+ktp4n.bmp",0,0,0)
g_ktp4e            = kts.Graphic("+ktp4e.bmp",0,0,0)
g_ktp4s            = kts.Graphic("+ktp4s.bmp",0,0,0)
g_ktp4w            = kts.Graphic("+ktp4w.bmp",0,0,0)
g_ktp5n            = kts.Graphic("+ktp5n.bmp",0,0,0)
g_ktp5e            = kts.Graphic("+ktp5e.bmp",0,0,0)
g_ktp5s            = kts.Graphic("+ktp5s.bmp",0,0,0)
g_ktp5w            = kts.Graphic("+ktp5w.bmp",0,0,0)
g_ktp6n            = kts.Graphic("+ktp6n.bmp",0,0,0)
g_ktp6e            = kts.Graphic("+ktp6e.bmp",0,0,0)
g_ktp6s            = kts.Graphic("+ktp6s.bmp",0,0,0)
g_ktp6w            = kts.Graphic("+ktp6w.bmp",0,0,0)
g_ktp7n            = kts.Graphic("+ktp7n.bmp",0,0,0)
g_ktp7e            = kts.Graphic("+ktp7e.bmp",0,0,0)
g_ktp7s            = kts.Graphic("+ktp7s.bmp",0,0,0)
g_ktp7w            = kts.Graphic("+ktp7w.bmp",0,0,0)
g_ktp8n            = kts.Graphic("+ktp8n.bmp",0,0,0)
g_ktp8e            = kts.Graphic("+ktp8e.bmp",0,0,0)
g_ktp8s            = kts.Graphic("+ktp8s.bmp",0,0,0)
g_ktp8w            = kts.Graphic("+ktp8w.bmp",0,0,0)


-- Menus and Inventory Icons

g_inv_bolt         = kts.Graphic("+inv_bolt.bmp")
g_inv_dagger       = kts.Graphic("+inv_dagger.bmp")
g_inv_key1         = kts.Graphic("+inv_key1.bmp")
g_inv_key2         = kts.Graphic("+inv_key2.bmp")
g_inv_key3         = kts.Graphic("+inv_key3.bmp")
g_inv_lockpicks    = kts.Graphic("+inv_lockpicks.bmp")
g_inv_overdraw_big = kts.Graphic("+inv_overdraw.bmp",0,0,0,0,-14)
g_inv_overdraw_small = kts.Graphic("+inv_overdraw.bmp",0,0,0,0,-6)
g_menu_axe         = kts.Graphic("+menu_axe.bmp")
g_menu_beartrap    = kts.Graphic("+menu_beartrap.bmp")
g_menu_blade_trap  = kts.Graphic("+menu_blade_trap.bmp")
g_menu_crossbow    = kts.Graphic("+menu_crossbow.bmp")
g_menu_dagger      = kts.Graphic("+menu_dagger.bmp")
g_menu_drop        = kts.Graphic("+menu_drop.bmp")
g_menu_drop_gem    = kts.Graphic("+menu_drop_gem.bmp")
g_menu_fist        = kts.Graphic("+menu_fist.bmp")
g_menu_lockpicks   = kts.Graphic("+menu_lockpicks.bmp")
g_menu_open_close  = kts.Graphic("+menu_open_close.bmp")
g_menu_pickup      = kts.Graphic("+menu_pickup.bmp")
g_menu_poison_trap = kts.Graphic("+menu_poison_trap.bmp")
g_menu_suicide     = kts.Graphic("+menu_suicide.bmp")


-- Tiles

g_barrel           = kts.Graphic("+barrel.bmp")
g_broken_wood_1    = kts.Graphic("+broken_wood_1.bmp")
g_broken_wood_2    = kts.Graphic("+broken_wood_2.bmp")
g_broken_wood_3    = kts.Graphic("+broken_wood_3.bmp")
g_broken_wood_4    = kts.Graphic("+broken_wood_4.bmp")
g_broken_wood_5    = kts.Graphic("+broken_wood_5.bmp")
g_cage             = kts.Graphic("+cage.bmp")
g_chair_north      = kts.Graphic("+chair_north.bmp")
g_chair_south      = kts.Graphic("+chair_south.bmp")
g_chair_east       = kts.Graphic("+chair_east.bmp")
g_chair_west       = kts.Graphic("+chair_west.bmp")
g_chest_east       = kts.Graphic("+chest_east.bmp")
g_chest_north      = kts.Graphic("+chest_north.bmp")
g_chest_south      = kts.Graphic("+chest_south.bmp")
g_chest_west       = kts.Graphic("+chest_west.bmp")
g_crystal_ball     = kts.Graphic("+crystal_ball.bmp")
g_dead_zombie      = kts.Graphic("+dead_zombie.bmp")
g_door_hgc         = kts.Graphic("+door_hgc.bmp",0,0,0)
g_door_hgo         = kts.Graphic("+door_hgo.bmp",0,0,0)
g_door_vgc         = kts.Graphic("+door_vgc.bmp",0,0,0)
g_door_vgo         = kts.Graphic("+door_vgo.bmp",0,0,0)
g_door_hic         = kts.Graphic("+door_hic.bmp",0,0,0)
g_door_hio         = kts.Graphic("+door_hio.bmp",0,0,0)
g_door_vic         = kts.Graphic("+door_vic.bmp",0,0,0)
g_door_vio         = kts.Graphic("+door_vio.bmp",0,0,0)
g_door_hwc         = kts.Graphic("+door_hwc.bmp",0,0,0)
g_door_hwo         = kts.Graphic("+door_hwo.bmp",0,0,0)
g_door_vwc         = kts.Graphic("+door_vwc.bmp",0,0,0)
g_door_vwo         = kts.Graphic("+door_vwo.bmp",0,0,0)
g_floor1           = kts.Graphic("+floor1.bmp")
g_floor2           = kts.Graphic("+floor2.bmp")
g_floor3           = kts.Graphic("+floor3.bmp")
g_floor4           = kts.Graphic("+floor4.bmp")
g_floor5           = kts.Graphic("+floor5.bmp")
g_floor6           = kts.Graphic("+floor6.bmp")
g_floor7           = kts.Graphic("+floor7.bmp")
g_floor8           = kts.Graphic("+floor8.bmp")
g_floor9           = kts.Graphic("+floor9.bmp")
g_floor10          = kts.Graphic("+floor10.bmp")
g_haystack         = kts.Graphic("+haystack.bmp")
g_hdoor_background = kts.Graphic("+hdoor_background.bmp")
g_home_east        = kts.Graphic("+home_east.bmp")
g_home_north       = kts.Graphic("+home_north.bmp")
g_home_south       = kts.Graphic("+home_south.bmp")
g_home_west        = kts.Graphic("+home_west.bmp")
g_open_chest_east  = kts.Graphic("+open_chest_east.bmp")
g_open_chest_north = kts.Graphic("+open_chest_north.bmp")
g_open_chest_south = kts.Graphic("+open_chest_south.bmp")
g_open_chest_west  = kts.Graphic("+open_chest_west.bmp")
g_pentagram        = kts.Graphic("+pentagram.bmp")
g_pillar           = kts.Graphic("+pillar.bmp")
g_pit_c            = kts.Graphic("+pit_c.bmp")
g_pit_o            = kts.Graphic("+pit_o.bmp")
g_pitv_c           = kts.Graphic("+pitv_c.bmp")
g_pitv_o           = kts.Graphic("+pitv_o.bmp")
g_pith_c           = kts.Graphic("+pith_c.bmp")
g_pith_o           = kts.Graphic("+pith_o.bmp")
g_pressure_plate   = kts.Graphic("+pressure_plate.bmp")
g_skull_left       = kts.Graphic("+skull_left.bmp")
g_skull_right      = kts.Graphic("+skull_right.bmp")
g_skull_up         = kts.Graphic("+skull_up.bmp")
g_skull_down       = kts.Graphic("+skull_down.bmp")
g_small_skull      = kts.Graphic("+small_skull.bmp")
g_stairs_east      = kts.Graphic("+stairs_east.bmp")
g_stairs_north     = kts.Graphic("+stairs_north.bmp")
g_stairs_south     = kts.Graphic("+stairs_south.bmp")
g_stairs_top       = kts.Graphic("+stairs_top.bmp")
g_stairs_west      = kts.Graphic("+stairs_west.bmp")
g_switch_down      = kts.Graphic("+switch_down.bmp")
g_switch_up        = kts.Graphic("+switch_up.bmp")
g_large_table_horiz = kts.Graphic("+large_table_horiz.bmp")
g_large_table_vert  = kts.Graphic("+large_table_vert.bmp")
g_table_north      = kts.Graphic("+table_north.bmp")
g_table_small      = kts.Graphic("+table_small.bmp")
g_table_south      = kts.Graphic("+table_south.bmp")
g_table_vert       = kts.Graphic("+table_vert.bmp")
g_table_west       = kts.Graphic("+table_west.bmp")
g_table_horiz      = kts.Graphic("+table_horiz.bmp")
g_table_east       = kts.Graphic("+table_east.bmp")
g_vdoor_background = kts.Graphic("+vdoor_background.bmp")
g_wall             = kts.Graphic("+wall.bmp")
g_wooden_floor     = kts.Graphic("+wooden_floor.bmp")
g_wooden_pit       = kts.Graphic("+wooden_pit.bmp")


-- Vampire Bats

g_vbat_1    = kts.Graphic("+vbat1.bmp", 0,0,0)
g_vbat_2    = kts.Graphic("+vbat2.bmp", 0,0,0)
g_vbat_3    = kts.Graphic("+vbat3.bmp", 0,0,0)
g_vbat_bite = kts.Graphic("+vbatbite.bmp", 0,0,0)


-- Zombies

g_zom1n = kts.Graphic("+zom1n.bmp", 0,0,0)
g_zom1e = kts.Graphic("+zom1e.bmp", 0,0,0)
g_zom1s = kts.Graphic("+zom1s.bmp", 0,0,0)
g_zom1w = kts.Graphic("+zom1w.bmp", 0,0,0)
g_zom2n = kts.Graphic("+zom2n.bmp", 0,0,0)
g_zom2e = kts.Graphic("+zom2e.bmp", 0,0,0)
g_zom2s = kts.Graphic("+zom2s.bmp", 0,0,0)
g_zom2w = kts.Graphic("+zom2w.bmp", 0,0,0)
g_zom3n = kts.Graphic("+zom3n.bmp", 0,0,0, 0,16)
g_zom3e = kts.Graphic("+zom3e.bmp", 0,0,0)
g_zom3s = kts.Graphic("+zom3s.bmp", 0,0,0)
g_zom3w = kts.Graphic("+zom3w.bmp", 0,0,0, 16,0)
g_zom4n = kts.Graphic("+zom4n.bmp", 0,0,0)
g_zom4e = kts.Graphic("+zom4e.bmp", 0,0,0)
g_zom4s = kts.Graphic("+zom4s.bmp", 0,0,0)
g_zom4w = kts.Graphic("+zom4w.bmp", 0,0,0)


-- Ogre graphics supplied by Loki Trickster
-- (Not used yet.)
-- (TODO: Move these into their own module, because they are not part of Classic Knights.)

--g_ogre_stand_2n  = kts.Graphic("+ogre_stand_2n.bmp",  255,255,255, 0,0, 3,1)
--g_ogre_stand_2e  = kts.Graphic("+ogre_stand_2e.bmp",  255,255,255, 0,0, 3,1)
--g_ogre_stand_2s  = kts.Graphic("+ogre_stand_2s.bmp",  255,255,255, 0,0, 3,1)
--g_ogre_stand_2w  = kts.Graphic("+ogre_stand_2w.bmp",  255,255,255, 0,0, 3,1)

--g_ogre_strike_1n = kts.Graphic("+ogre_strike_1n.bmp", 255,255,255, 0,0, 3,1)
--g_ogre_strike_1e = kts.Graphic("+ogre_strike_1e.bmp", 255,255,255, 0,0, 3,1)
--g_ogre_strike_1s = kts.Graphic("+ogre_strike_1s.bmp", 255,255,255, 0,0, 3,1)
--g_ogre_strike_1w = kts.Graphic("+ogre_strike_1w.bmp", 255,255,255, 0,0, 3,1)

--g_ogre_strike_3n = kts.Graphic("+ogre_strike_3n.bmp", 255,255,255, 0,12, 3,1)
--g_ogre_strike_3e = kts.Graphic("+ogre_strike_3e.bmp", 255,255,255, 0,0,  3,1)
--g_ogre_strike_3s = kts.Graphic("+ogre_strike_3s.bmp", 255,255,255, 0,0,  3,1)
--g_ogre_strike_3w = kts.Graphic("+ogre_strike_3w.bmp", 255,255,255, 12,0, 3,1)

--g_ogre_walk_1n   = kts.Graphic("+ogre_walk_1n.bmp",   255,255,255, 0,0, 3,1)
--g_ogre_walk_1e   = kts.Graphic("+ogre_walk_1e.bmp",   255,255,255, 0,0, 3,1)
--g_ogre_walk_1s   = kts.Graphic("+ogre_walk_1s.bmp",   255,255,255, 0,0, 3,1)
--g_ogre_walk_1w   = kts.Graphic("+ogre_walk_1w.bmp",   255,255,255, 0,0, 3,1)





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
