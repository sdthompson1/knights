--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2025.
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


-- Depths:
-- -4 = open pits, treasure chests, doors.   (We don't want blood etc appearing on top of these)
-- -3 = blood
-- -2 = vampire bat corpses.
-- -1 = knight corpses; zombie corpses; bones
-- 0 = default tile depth.
-- NB RemoteDungeonView currently assumes that depths are between -8 and +7, although that
-- could be changed of course.


-- Tiles can require up to 8 axe hits (32 sword hits)
-- but make the larger amounts progressively less likely.

function tile_hit_points()
   local choice = kts.RandomRange(1,3)
   if choice == 1 then 
      return kts.RandomRange(2, 16)
   elseif choice == 2 then
      return kts.RandomRange(2, 24)
   else 
      return kts.RandomRange(2, 32)
   end
end


function locked_msg()
   kts.FlashMessage("Locked", 2)
end

-- Define some access types

wall_acc = {
    walking = "blocked",
    flying = "blocked",
    missiles = "blocked"
}
door_acc = {
    walking = "approach",
    flying = "approach",
    missiles = "blocked"
}
gate_acc = {
    walking = "approach",
    flying = "approach",
    missiles = "partial"
}
switch_acc = {
    walking = "approach",
    flying = "clear",
    missiles = "partial"
}
barrel_acc = {
    walking = "blocked",
    flying = "clear",
    missiles = "clear"
}
furniture_acc = {
    walking = "approach",
    flying = "clear",
    missiles = "clear"
}
floor_acc = {
    walking = "clear",
    flying = "clear",
    missiles = "clear"
}

-- Tile destruction functions (used below)

-- [Note that t_broken_wood_1, etc, are not defined yet, but that
-- doesn't matter because they don't need to be defined until the
-- destruction function actually runs (i.e. in-game).]

function destroy1()
   kts.AddTile(cxt.pos, t_broken_wood_1)
   snd_tile_destroy()
end
function destroy3()
   kts.AddTile(cxt.pos, t_broken_wood_3)
   snd_tile_destroy()
end
function destroy4()
   kts.AddTile(cxt.pos, t_broken_wood_4)
   snd_tile_destroy()
end
function destroy5()
   kts.AddTile(cxt.pos, t_broken_wood_5)
   snd_tile_destroy()
end



-- Numbers in the following are the original numbering from Knights.
-- Note the following number ranges are unused in Knights:
-- <=1, 20-22, 29-31, 33-34, 46, >=86.


-- Base table for floor tiles

floor = {
    access = floor_acc,
    items  = "floor"
}


-- Walls and pillars.

wall = {
    access = wall_acc
}

t_wall_normal = kts.Tile( table_merge( wall, { graphic=g_wall } ) )   -- 2

t_wall_pillar = kts.Tile( table_merge(wall, {   -- 3
    graphic = g_pillar
}))

t_wall_skull_north = kts.Tile(table_merge( wall, { graphic = g_skull_up    }))
t_wall_skull_east  = kts.Tile(table_merge( wall, { graphic = g_skull_right }))   -- 4
t_wall_skull_south = kts.Tile(table_merge( wall, { graphic = g_skull_down  }))
t_wall_skull_west  = kts.Tile(table_merge( wall, { graphic = g_skull_left  }))   -- 5

kts.SetRotate(t_wall_skull_north, t_wall_skull_east, t_wall_skull_south, t_wall_skull_west)  -- in clockwise order.
kts.SetReflect(t_wall_skull_east, t_wall_skull_west)

t_wall_cage  = kts.Tile(table_merge(wall, { graphic=g_cage }))     -- 6


-- Doors.
-- Those funny letters in door names mean the following:
-- 1. 'h' = horizontal or 'v' = vertical
-- 2. 'w' = wooden, 'i' = iron, 'g' = gate (portcullis).
-- 3. 'c' = closed, 'o' = open.

door_control_tbl = {
    action = kts.Activate,
    possible = kts.Can_Activate,
    menu_icon = g_menu_open_close,
    menu_direction = "up",
    tap_priority = 4,
    action_bar_slot = 2,
    menu_special = 1,
    name = "Open/Close Door"
}
chest_control_tbl = table_merge(door_control_tbl, { name = "Open/Close Chest" })

door_control = kts.Control (door_control_tbl)
chest_control = kts.Control (chest_control_tbl)

-- door_control_low_pri is used for open doors, because we want pickup/drop to take priority over
-- closing a door (but we want opening a door to take priority over pickup/drop).
door_control_low_pri = kts.Control (table_merge(door_control_tbl, { tap_priority = 1 }))
chest_control_low_pri = kts.Control (table_merge(chest_control_tbl, { tap_priority = 1 }))


function door_control_lua_func(pos)
   if kts.IsDoorOpen(pos) then 
      return door_control_low_pri
   else 
      return door_control
   end
end
function chest_control_lua_func(pos)
   if kts.IsDoorOpen(pos) then 
      return chest_control_low_pri
   else 
      return chest_control
   end
end

door_base = {
    type     = "door",
    access   = door_acc,
    on_open_or_close  = snd_door,
    on_unlock_fail    = function() locked_msg(); snd_lock() end,
    control = door_control_lua_func
}
wood_door = table_merge(door_base, {
    hit_points  = tile_hit_points,
    on_hit      = snd_tile_bash,
    on_destroy  = destroy1,
    lock_chance = 0.1,
    keymax      = 3
})
iron_door = table_merge(door_base, {
    lock_chance = 1.0,   -- always locked
    keymax      = 3,
    on_hit      = snd_tile_clunk
})
gate = {
    access      = gate_acc,
    on_hit      = snd_tile_clunk
}
door_horiz = table_merge(wood_door, {
    depth = -4, 
    graphic = g_door_hwc, 
    open_graphic = g_door_hwo
})
door_vert  = table_merge(wood_door, {
    depth = -4, 
    graphic = g_door_vwc, 
    open_graphic = g_door_vwo
})
iron_door_horiz = table_merge(iron_door, {
    depth = -4, 
    graphic = g_door_hic, 
    open_graphic = g_door_hio
})
iron_door_vert = table_merge(iron_door, {
    depth = -4, 
    graphic = g_door_vic, 
    open_graphic = g_door_vio
})

t_door_horiz      = kts.Tile( door_horiz )      -- 7, 50
t_door_vert       = kts.Tile( door_vert  )      -- 8, 51
t_iron_door_horiz = kts.Tile( iron_door_horiz ) -- 9, 52
t_iron_door_vert  = kts.Tile( iron_door_vert )  -- 10, 53
kts.SetRotate(t_door_horiz, t_door_vert)
kts.SetRotate(t_iron_door_horiz, t_iron_door_vert)


-- NOTE: For closed gates set connectivity_check = -1 to force the connectivity check
-- to consider these as "impassable". (They may or may not be openable by a switch, but 
-- we don't really have any way to check this so we have to be conservative and assume
-- they can't be.)

t_gate_horiz      = kts.Tile( table_merge(gate, {  -- 16, 64
    depth = -4, 
    graphic = g_door_hgc, 
    connectivity_check = -1,
}))

t_gate_vert       = kts.Tile( table_merge(gate, {  -- 17, 65
    depth = -4, 
    graphic = g_door_vgc, 
    connectivity_check = -1,
}))

t_open_gate_horiz = kts.Tile( table_merge(floor, {
    graphic = g_door_hgo, 
}))
t_open_gate_vert  = kts.Tile( table_merge(floor, {
    graphic=g_door_vgo,
}))

kts.SetRotate(t_gate_horiz, t_gate_vert)
kts.SetRotate(t_open_gate_horiz, t_open_gate_vert)

set_open_closed(t_open_gate_horiz, t_gate_horiz)
set_open_closed(t_open_gate_vert, t_gate_vert)

t_door_horiz_locked      = kts.Tile( table_merge(door_horiz, { special_lock=1 }))
t_door_vert_locked       = kts.Tile( table_merge(door_vert, { special_lock=1 }))
t_iron_door_horiz_locked = kts.Tile( table_merge(iron_door_horiz, { special_lock = 1 }))
t_iron_door_vert_locked  = kts.Tile( table_merge(iron_door_vert, { special_lock = 1 }))

kts.SetRotate(t_door_horiz_locked, t_door_vert_locked)
kts.SetRotate(t_iron_door_horiz_locked, t_iron_door_vert_locked)

t_hdoor_background = kts.Tile( table_merge(floor, { graphic = g_hdoor_background }))
t_vdoor_background = kts.Tile( table_merge(floor, { graphic = g_vdoor_background }))

kts.SetRotate(t_hdoor_background, t_vdoor_background)


-- Homes. (Facing is the direction TOWARDS the dungeon -- AWAY from the home itself.)

home = {
    access = door_acc,
    type = "home",
    unsecured_colour = 0,      -- black
    on_hit = snd_tile_clunk,
    on_approach = check_escape_quest
}
home_south = table_merge( home, {
    graphic = g_home_south, 
    facing = "south"
})
home_west = table_merge( home, {
    graphic = g_home_west,
    facing = "west"
})
home_north = table_merge( home, {
    graphic = g_home_north, 
    facing = "north"
})
home_east = table_merge( home, {
    graphic = g_home_east,
    facing="east"
})

t_home_south  = kts.Tile( home_south )   -- 11
t_home_west   = kts.Tile( home_west )    -- 12
t_home_north  = kts.Tile( home_north )   -- 13
t_home_east   = kts.Tile( home_east )    -- 14

t_home_south_special = kts.Tile(table_merge( home_south, {   -- 92
    special_exit = true
}))
t_home_west_special  = kts.Tile(table_merge( home_west, {   -- 93
    special_exit = true
}))
t_home_north_special = kts.Tile(table_merge( home_north, {   -- 94
    special_exit = true
}))
t_home_east_special  = kts.Tile(table_merge( home_east, {   -- 95
    special_exit = true
}))

kts.SetRotate(t_home_south, t_home_west, t_home_north, t_home_east)
kts.SetReflect(t_home_west, t_home_east)
kts.SetRotate(t_home_south_special, t_home_west_special, t_home_north_special, t_home_east_special)
kts.SetReflect(t_home_west_special, t_home_east_special)


-- Crystal balls.

t_crystal_ball = kts.Tile {           -- 15
    access = door_acc,
    graphic = g_crystal_ball,
    on_approach  = kts.CrystalStart,
    on_withdraw  = kts.CrystalStop,
    on_hit = snd_tile_clunk
}
set_open_closed(t_crystal_ball, t_wall_pillar)


-- Switches

switch = {
    access = switch_acc,
    on_hit = function() kts.Activate(); snd_tile_bash() end,
    control = kts.Control (table_merge( door_control_tbl, { name = "Pull Lever" })),
    items = 0
}
t_switch_up   = kts.Tile( table_merge( switch, {     -- 18
    graphic = g_switch_up, 
    on_activate = function() kts.ChangeTile(t_switch_down); snd_door() end
}))
t_switch_down = kts.Tile( table_merge( switch, {     -- 19
    graphic = g_switch_down, 
    on_activate = function() kts.ChangeTile(t_switch_up); snd_door() end
}))



-- Various bits of furniture.

furniture = {
    access     = furniture_acc,
    hit_points = tile_hit_points,
    on_hit     = snd_tile_bash
}

furniture4 = table_merge( furniture, { on_destroy = destroy4 })
furniture5 = table_merge( furniture, { on_destroy = destroy5 })

t_haystack = kts.Tile (table_merge( furniture4, {   -- 23
    access  = switch_acc,
    graphic = g_haystack,
}))
t_barrel = kts.Tile (table_merge( furniture4, {   -- 24
    type = "barrel",
    access = barrel_acc,
    graphic = g_barrel,
    items = "barrel"
    -- Note we don't have to explicitly say "items are not allowed" -- indeed this would not
    -- be possible in the current system (since setting "items" to an item generation
    -- category sets items-allowed to true as well) -- but setting the tile-type to "barrel" 
    -- automatically sets items-allowed to false (as a special case) -- see special_tiles.cpp.
}))

chest = {
    type        = "chest",
    items       = "chest",
    access      = furniture_acc,
    trap_chance = 0.5,   -- This only applies if pretrapped chests is on
    traps       = function(pos,dir) 
       if kts.RandomChance(0.5) then
          kts.SetPoisonTrap(pos, i_poison_trap)
       else
          kts.SetBladeTrap(pos, i_blade_trap, i_bolts, dir)
       end
    end,
    lock_chance = 0.5,   -- This only applies if a trap was not generated.
    lock_pick_only_chance = 0.16667,  -- Chance that the lock cannot be opened by any key, and requires lock picks.
    keymax      = 3,                  -- If nkeys < this then there is more chance the chest will be unlocked.
    hit_points  = tile_hit_points,
    on_destroy  = destroy3,
    control = chest_control_lua_func,
    on_open_or_close = snd_door,
    on_unlock_fail   = function() locked_msg(); snd_lock() end,
    on_hit      = snd_tile_bash,
    depth       = -4
}

t_chest_north = kts.Tile( table_merge( chest, {  -- 25, 35
    graphic = g_chest_north, 
    open_graphic = g_open_chest_north, 
    facing = "north"
}))
t_chest_east  = kts.Tile( table_merge( chest, {  -- 26, 36
    graphic = g_chest_east,  
    open_graphic = g_open_chest_east,
    facing="east"
}))
t_chest_south = kts.Tile( table_merge( chest, {  -- 27, 37
    graphic = g_chest_south, 
    open_graphic = g_open_chest_south, 
    facing="south"
}))
t_chest_west  = kts.Tile( table_merge( chest, {  -- 28, 38
    graphic = g_chest_west,  
    open_graphic = g_open_chest_west,
    facing="west"
}))

kts.SetRotate(t_chest_north, t_chest_east, t_chest_south, t_chest_west)
kts.SetReflect(t_chest_east, t_chest_west)

t_small_skull = kts.Tile {    -- 32
    access = furniture_acc,
    items = "small_table",
    graphic = g_small_skull,
    on_hit = snd_tile_clunk
}

table_base = {
    access = furniture_acc,
    hit_points = tile_hit_points,
    items = "table",
    on_hit = snd_tile_bash
}
table4 = table_merge( table_base, { on_destroy = destroy4 })
table5 = table_merge( table_base, { on_destroy = destroy5 })

t_table_small = kts.Tile( table_merge( table5, { graphic=g_table_small, items="small_table" }))  -- 39
t_table_north = kts.Tile( table_merge( table5, { graphic=g_table_north }))  -- 40
t_table_vert  = kts.Tile( table_merge( table5, { graphic=g_table_vert }))   -- 41
t_table_south = kts.Tile( table_merge( table5, { graphic=g_table_south }))  -- 42
t_table_west  = kts.Tile( table_merge( table5, { graphic=g_table_west }))
t_table_horiz = kts.Tile( table_merge( table5, { graphic=g_table_horiz }))
t_table_east  = kts.Tile( table_merge( table5, { graphic=g_table_east }))

kts.SetRotate(t_table_north, t_table_east, t_table_south, t_table_west)
kts.SetReflect(t_table_west, t_table_east)
kts.SetRotate(t_table_vert, t_table_horiz)

t_large_table_horiz = kts.Tile( table_merge( table4, { graphic=g_large_table_horiz }))  -- 43
t_large_table_vert  = kts.Tile( table_merge( table4, { graphic=g_large_table_vert }))

kts.SetRotate(t_large_table_horiz, t_large_table_vert)

t_chair_south = kts.Tile( table_merge( furniture4, { graphic=g_chair_south }))  -- 44
t_chair_north = kts.Tile( table_merge( furniture4, { graphic=g_chair_north }))  -- 45
t_chair_west  = kts.Tile( table_merge( furniture4, { graphic=g_chair_west }))
t_chair_east  = kts.Tile( table_merge( furniture4, { graphic=g_chair_east }))

kts.SetRotate(t_chair_north, t_chair_east, t_chair_south, t_chair_west)
kts.SetReflect(t_chair_west, t_chair_east)


-- Pits.

closed_pit = floor

open_pit = 
    {
        access       = floor_acc,
        on_walk_over = kts.PitKill,
        items        = "destroy",
        
        -- Even though a pit has "clear" access (at all heights) it should still block the 
        -- connectivity check. (Actually some open pits are passable, because they can be 
        -- closed by switches, but we can't easily check this, so we err on the side of 
        -- caution and mark it "impassable" always.)
        connectivity_check = -1
    }

t_open_pit_vert   = kts.Tile( table_merge( open_pit, {     -- 47
    graphic = g_pitv_o,
    depth   = -4,
}))
t_open_pit_horiz = kts.Tile( table_merge( open_pit, {
    graphic = g_pith_o,
    depth   = -4,
}))
kts.SetRotate(t_open_pit_horiz, t_open_pit_vert)

t_open_pit_wooden = kts.Tile( table_merge( open_pit, {     -- 48
    graphic = g_wooden_pit,
    depth   = -4,
}))

t_open_pit_normal = kts.Tile( table_merge( open_pit, {     -- 49
    graphic = g_pit_o,
    depth   = -4,
}))

t_closed_pit_vert = kts.Tile( table_merge( closed_pit, {   -- 78
    graphic = g_pitv_c,
}))
t_closed_pit_horiz = kts.Tile( table_merge( closed_pit, {
    graphic = g_pith_c,
}))
kts.SetRotate(t_closed_pit_vert, t_closed_pit_horiz)

t_closed_pit_wooden = kts.Tile( table_merge( closed_pit, { -- 79
    graphic = g_wooden_floor,
}))

t_closed_pit_normal = kts.Tile( table_merge( closed_pit, { -- 80
    graphic = g_pit_c,
}))

set_open_closed(t_open_pit_normal, t_closed_pit_normal)
set_open_closed(t_open_pit_vert, t_closed_pit_vert)
set_open_closed(t_open_pit_horiz, t_closed_pit_horiz)
set_open_closed(t_open_pit_wooden, t_closed_pit_wooden)


-- This is a list of all "open pit" tiles, used in monsters.lua
all_open_pit_tiles = {t_open_pit_horiz, t_open_pit_vert, t_open_pit_wooden, t_open_pit_normal}


-- Floor tiles of all kinds.

t_broken_wood_1 = kts.Tile(table_merge( floor, { graphic=g_broken_wood_1 }))  -- 54
t_broken_wood_2 = kts.Tile(table_merge( floor, { graphic=g_broken_wood_2 }))  -- 55
t_broken_wood_3 = kts.Tile(table_merge( floor, { graphic=g_broken_wood_3 }))  -- 56
t_broken_wood_4 = kts.Tile(table_merge( floor, { graphic=g_broken_wood_4 }))  -- 57
t_broken_wood_5 = kts.Tile(table_merge( floor, { graphic=g_broken_wood_5 }))  -- 58

t_floor1  = kts.Tile(table_merge( floor, { graphic=g_floor1 }))  -- 66
t_floorpp = kts.Tile(table_merge( floor, { graphic=g_pressure_plate }))  -- 67
t_floor2  = kts.Tile(table_merge( floor, { graphic=g_floor2 }))  -- 68
t_floor3  = kts.Tile(table_merge( floor, { graphic=g_floor3 }))  -- 69
t_floor4  = kts.Tile(table_merge( floor, { graphic=g_floor4 }))  -- 70
t_floor5  = kts.Tile(table_merge( floor, { graphic=g_floor5 }))  -- 71
t_floor6  = kts.Tile(table_merge( floor, { graphic=g_floor6, depth=-1 }))  -- 72
t_floor7  = kts.Tile(table_merge( floor, { graphic=g_floor7 }))  -- 73
t_floor8  = kts.Tile(table_merge( floor, { graphic=g_floor8 }))  -- 74
t_floor9  = kts.Tile(table_merge( floor, { graphic=g_floor9 }))  -- 75
t_floor10 = kts.Tile(table_merge( floor, { graphic=g_floor10 }))  -- 77

pentagram_base = table_merge( floor, {
    graphic      = g_pentagram,
    on_walk_over = function()
       -- zombies can walk over pentagrams with impunity, 
       -- but knights get zapped.
       if kts.ActorIsKnight() then
          zap()
          pentagram_effect()
       end
    end
})

t_dead_pentagram = kts.Tile(table_merge( floor, {   -- 76
    graphic      = g_pentagram
}))
t_live_pentagram = kts.Tile(pentagram_base)   -- 1 in my room tables. (76 in original Knights.)
t_special_pentagram = kts.Tile(table_merge( pentagram_base, {  -- 90
    on_hit = function() end   -- does nothing, but means wand will 'zap' when you hit special pentagram with it.
}))

all_special_pentagrams = { t_special_pentagram }  -- needed for 'destroy book with wand' quest

set_open_closed(t_live_pentagram, t_dead_pentagram)



-- Stairs

t_stairs_top   = kts.Tile(table_merge( floor, { graphic=g_stairs_top, stairs_down="special" }))  -- 81

t_stairs_south = kts.Tile(table_merge( floor, { graphic=g_stairs_south, stairs_down="south" }))  -- 82
t_stairs_west  = kts.Tile(table_merge( floor, { graphic=g_stairs_west,  stairs_down="west"  }))  -- 83
t_stairs_north = kts.Tile(table_merge( floor, { graphic=g_stairs_north, stairs_down="north" }))  -- 84
t_stairs_east  = kts.Tile(table_merge( floor, { graphic=g_stairs_east,  stairs_down="east"  }))  -- 85

kts.SetRotate(t_stairs_north, t_stairs_east, t_stairs_south, t_stairs_west)
kts.SetReflect(t_stairs_west, t_stairs_east)


--------------------------

t_blood_1 = kts.Tile(table_merge( floor, {graphic=g_blood_1, depth=-3}))
t_blood_2 = kts.Tile(table_merge( floor, {graphic=g_blood_2, depth=-3}))
t_blood_3 = kts.Tile(table_merge( floor, {graphic=g_blood_3, depth=-3}))

t_dead_knight_1 = kts.Tile(table_merge( floor, { graphic = g_dead_knight_1, depth = -1 } ))
t_dead_knight_2 = kts.Tile(table_merge( floor, { graphic = g_dead_knight_2, depth = -1 } ))
t_dead_knight_3 = kts.Tile(table_merge( floor, { graphic = g_dead_knight_3, depth = -1 } ))
t_dead_knight_4 = kts.Tile(table_merge( floor, { graphic = g_dead_knight_4, depth = -1 } ))

t_dead_zombie = kts.Tile(table_merge( floor, { graphic=g_dead_zombie, depth = -1 }))    -- 59

t_dead_vbat_1 = kts.Tile(table_merge( floor, { graphic = g_dead_vbat_1, depth = -2 }))
t_dead_vbat_2 = kts.Tile(table_merge( floor, { graphic = g_dead_vbat_2, depth = -2 }))
t_dead_vbat_3 = kts.Tile(table_merge( floor, { graphic = g_dead_vbat_3, depth = -2 }))

--------------------------

