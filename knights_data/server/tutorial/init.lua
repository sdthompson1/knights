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

module(...)

dofile("messages.lua")

local C = require("classic")

-- Debug
C.t_floor1.on_walk_over = function() 
   if kts.IsKnight(cxt.actor) then
      print(cxt.tile_pos.x - 1, " ", cxt.tile_pos.y - 1)
   end
end

----------------------------------------------------------------------
-- New tiles and items
----------------------------------------------------------------------

-- This is a "fake" stuff bag item that gives you a gem (among other
-- things) when you pick it up.
stuff_with_gem = kts.ItemType {
   type = "magic",
   graphic = C.g_stuff_bag,
   on_pick_up = function()
      -- Give the actor the stuff
      kts.GiveItem(cxt.actor, C.i_gem, 1)
      kts.GiveItem(cxt.actor, C.i_bolts, 3)
      kts.GiveItem(cxt.actor, C.i_daggers, 4)
      kts.GiveItem(cxt.actor, C.i_blade_trap, 2)
      kts.GiveItem(cxt.actor, C.i_poison_trap, 2)

      -- Display the tutorial message
      local n = kts.GetNumHeld(cxt.actor, C.i_gem)
      local m = { title = stuff_title, graphics = { C.g_floor1, C.g_stuff_bag }, body = stuff_msg }
      if n == 5 then
         m.body = m.body .. stuff_msg_final
      else
         m.body = m.body .. string.format(stuff_msg_nonfinal, n, 5-n)
      end
      kts.PopUpWindow(m)
   end
}

-- Prevent bats flying over switches
new_acc = C.switch_acc & { flying="blocked" }
C.t_switch_up.access = new_acc
C.t_switch_down.access = new_acc

-- A version of t_switch_up that appears as a wall on the mini map.
switch_new = kts.Tile( C.t_switch_up.table & { map_as = "wall" })

-- Potions/scrolls with fixed effects
function make_potion(effect, hint)
   return kts.ItemType {
      type = "magic",
      graphic = C.g_potion,
      fragile = true,
      on_pick_up = function()
         C.snd_drink()
         kts.Delay(750)
         effect()
         tutorial_msg(hint)
      end
   }
end

function make_scroll(effect, hint)
   return kts.ItemType {
      type = "magic",
      graphic = C.g_scroll,
      on_pick_up = function() 
         C.zap() 
         effect()
         tutorial_msg(hint)
      end
   }
end

poison_potion = make_potion(C.poison, 61)
regeneration_potion = make_potion(C.regeneration, 60)
super_potion = make_potion(C.super, 42)
quickness_scroll = make_scroll(C.quickness, 71)

low_damage_bolt = kts.ItemType( C.i_bolts.table & { missile_damage = C.rng_range(1,2) } )

-- Modify the pentagram to teleport to a fixed location instead of randomly.
-- Quickest way to do this is to modify "my_teleport" in classic module.
function C.my_teleport()
   C.snd_teleport()
   kts.TeleportTo(cxt.actor, {x=33, y=31})
end   


----------------------------------------------------------------------
-- Load tutorial_map.txt
----------------------------------------------------------------------

tiles = {}
for i,v in pairs(C.tile_table) do
   tiles[i] = v
end
tiles[96]  = { C.t_table_small, C.i_key1 }
tiles[97]  = { C.t_floor1, C.i_hammer }
tiles[98]  = { C.t_floor1, C.i_lockpicks }
tiles[99]  = { C.t_floor1, C.i_poison_trap }
tiles[100] = { C.t_floor1, C.i_blade_trap }
tiles[101] = { C.t_floor1, C.i_staff }
tiles[102] = { C.t_floor1, C.i_bear_trap }
tiles[103] = { C.t_floor1, C.i_axe }
tiles[104] = { C.t_floor1, {C.i_dagger,8} }
tiles[105] = { C.t_floor1, C.i_crossbow }
tiles[106] = { C.t_floor1, {C.i_bolts,8} }
tiles[108] = { C.t_floor1, quickness_scroll }
tiles[110] = { C.t_floor6, stuff_with_gem }
tiles[111] = { C.t_floor1, C.m_zombie }
tiles[113] = { C.t_floorpp, C.i_gem }
tiles[114] = { C.t_table_south, C.i_gem }
tiles[115] = switch_new
tiles[116] = { C.t_floor1, C.i_gem }

tutorial_segs = kts.LoadSegments(tiles, "tutorial_map.txt")


----------------------------------------------------------------------
-- Custom respawn function
----------------------------------------------------------------------

respawn_x = 7
respawn_y = 2
respawn_dir = "south"

function respawn_func()

   -- Turn off the "chamber of bats" if it is active
   if bat_task_active then
      -- turn off bat spawning, remove all bats
      reset_bat_task()

      bat_failures = bat_failures + 1

      -- Print appropriate message
      if bat_level == 1 then 
         if bat_failures == 1 then
            tutorial_msg(29)
         elseif bat_failures == 2 then
            tutorial_msg(30)
         elseif bat_failures == 3 then
            tutorial_msg(31)
         else
            generic_bat_failed_msg()
         end
      else
         generic_bat_failed_msg()
      end

      -- if lvl 2 or above (or >=3 failures), open the doors.
      if bat_level > 1 or bat_failures >= 3 then
         open_bat_doors()
      end

      -- respawn outside the switch
      return 26, 34, "south"
   end

   return respawn_x, respawn_y, respawn_dir
end

function setup_respawn(x, y, dir)
   local tile = kts.Tile { 
      on_walk_over = function()
         respawn_x = cxt.tile_pos.x
         respawn_y = cxt.tile_pos.y
         respawn_dir = dir
      end,
      depth = 1
   }
   kts.AddTile({x=x,y=y}, tile)
end


----------------------------------------------------------------------
-- Game start function
----------------------------------------------------------------------

function start_tutorial()

   -- Install the Chamber of Bats background task
   kts.AddTask(bat_task)

   -- Create the basic dungeon layout
   kts.LayoutDungeon {
      layout = {
         width = 1,
         height = 1,
         data = { { type = "block" } }
      },
      wall = C.t_wall_normal,
      segments = tutorial_segs,
      entry_type = "none",
      allow_rotate = false
   }

   -- Setup the skull puzzle
   setup_skull(skull_x + 2)
   setup_skull(skull_x + 4)
   setup_skull(skull_x + 6)

   -- Setup locked doors
   kts.LockDoor({x=3,y=11}, "pick_only")
   kts.LockDoor({x=23,y=20}, 1)

   -- Place items in chests
   kts.PlaceItem({x=31,y=33}, super_potion)
   kts.PlaceItem({x=32,y=33}, C.i_gem)
   kts.SetBladeTrap({x=33,y=33}, C.i_blade_trap, low_damage_bolt, "north")
   kts.PlaceItem({x=4,y=36}, poison_potion)
   kts.PlaceItem({x=5,y=36}, regeneration_potion)
   kts.PlaceItem({x=8,y=34}, C.i_gem)
   kts.LockDoor({x=8,y=34}, "pick_only")

   -- Install custom respawn function
   kts.SetRespawnFunction(respawn_func)

   -- Set up the respawn points
   setup_respawn(24, 10, "south")
   setup_respawn(20, 22, "west")
   setup_respawn(12, 25, "south")

   -- Setup tutorial messages
   add_msgs()
end


----------------------------------------------------------------------
-- Menu
----------------------------------------------------------------------

kts.MENU = {
   text = "TUTORIAL",
   start_game_func = start_tutorial,
   items = { 
      {
         id = "quest",
         text = "Quest",
         choices = { 
            {
               id = "tutorial",
               text = "Tutorial"
            }
         }
      }
   }
}


----------------------------------------------------------------------
-- Switch Puzzle
----------------------------------------------------------------------

switches_hit = {}
num_switches_hit = 0
door1_pos = {x=22, y=13}
door2_pos = {x=22, y=11}
secret_door_pos = {x=22, y=9}

function open_gate(pos)
   -- Open the gate.
   -- We assume there is only one tile on the gate square.
   -- We remove that tile, then add its "open_to" tile instead.
   local tiles = kts.GetTiles(pos)
   if tiles[1].open_to ~= nil then
      kts.RemoveTile(pos, tiles[1])
      kts.AddTile(pos, tiles[1].open_to)
   end
end

function _G.rf_sw_puzzle()

   -- Find out whether we have hit this particular switch before
   -- (Use x coord of switch as index into "switches_hit" table)
   local hit_before = switches_hit[cxt.tile_pos.x] ~= nil
   
   -- Remember that we have hit this switch
   switches_hit[cxt.tile_pos.x] = true

   if not hit_before then 
      num_switches_hit = num_switches_hit + 1

      if num_switches_hit == 1 then
         tutorial_msg(15) 
      elseif num_switches_hit == 2 then
         tutorial_msg(16)
      else
         tutorial_msg(77)
      end
   end

   -- Open all relevant gates/doors
   if num_switches_hit >= 1 then
      open_gate(door1_pos)
   end

   if num_switches_hit >= 2 then
      open_gate(door2_pos)
   end

   if num_switches_hit >= 3 then
      local tiles = kts.GetTiles(secret_door_pos)
      kts.RemoveTile(secret_door_pos, tiles[1])
      kts.AddTile(secret_door_pos, C.t_floor1)
   end
end


----------------------------------------------------------------------
-- Chamber of Vampire Bats
----------------------------------------------------------------------

bat_door_1 = { x = 26, y = 27 }
bat_door_2 = { x = 30, y = 31 }
bat_door_3 = { x = 22, y = 31 }

function close_bat_doors()
   kts.CloseDoor(bat_door_1)
   kts.CloseDoor(bat_door_2)
   kts.CloseDoor(bat_door_3)
end

function open_bat_doors()
   kts.OpenDoor(bat_door_1)
   kts.OpenDoor(bat_door_2)
   kts.OpenDoor(bat_door_3)
end

bat_pits = { {x=23, y=28}, {x=29,y=28},
             {x=25, y=29}, {x=28,y=29},
             {x=24, y=30},
             {x=24, y=32}, {x=28,y=32},
             {x=26, y=33},
             {x=23, y=34}, {x=29,y=34} }
num_bat_pits = 10

bat_count = 0
bat_list = {}

function spawn_bat()

   local kt_pos = kts.GetPos(bat_actor)
   if kt_pos == nil then return end  -- Suspend bat spawning if player is dead.

   -- 10 attempts
   for i=1,10 do
      
      local n = kts.RandomRange(1, num_bat_pits)
      local pos = bat_pits[n]

      -- Reduce chance of spawning immediately next to the knight 
      -- (includes diagonals)
      if math.abs(pos.x-kt_pos.x) > 1 or math.abs(pos.y-kt_pos.y) > 1 or
      kts.RandomChance(bat_level*0.2 - 0.2) then
         
         local bat = kts.AddMonster(bat_pits[n], C.m_vampire_bat)
         if bat ~= nil then
            bat_count = bat_count + 1
            table.insert(bat_list, bat)
            return
         end
      end
   end
end

function update_bat_list()
   if bat_count > 0 then
      local new_list = {}
      bat_count = 0
      for k,v in pairs(bat_list) do
         if kts.IsAlive(v) then
            table.insert(new_list, v)
            bat_count = bat_count + 1
         end
      end
      bat_list = new_list
   end
end

bat_task_active = false
bats_remaining = 0  -- Number that need to be killed to win.
next_spawn_at = 0   -- In milliseconds.

-- Difficulty settings
bat_level = 1
bat_schedule = { def=10 }  -- In seconds.
bat_failures = 0

function update_quest_rqmts()
   kts.ClearHints()
   if bats_remaining > 0 then
      local msg
      if bats_remaining == 1 then
         msg = bat_objective_sing
      else
         msg = string.format(bat_objective_pl, bats_remaining)
      end
      kts.AddHint(msg, 1, 1)
      kts.AddHint("", 2, 1)
   else
      kts.AddHint(normal_objective_1, 1, 1)
      kts.AddHint(normal_objective_2, 2, 1)
   end
   kts.ResendHints()
end

function schedule_bat()
   local sched = bat_schedule[bat_count]
   if sched == nil then 
      sched = bat_schedule.def 
   end

   sched = kts.RandomRange(500*sched, 1000*sched)

   next_spawn_at = math.min(next_spawn_at, 
                            kts.GameTime() + sched)
end

function bat_task()

   if bat_task_active then
      
      -- Update bats_remaining
      local old_bat_count = bat_count
      update_bat_list()

      if old_bat_count ~= bat_count then
         local num_killed = old_bat_count - bat_count
         bats_remaining = bats_remaining - num_killed
         update_quest_rqmts()
         
         -- Reschedule next spawn if required
         schedule_bat()

         -- Check if they just won
         if bats_remaining <= 0 then

            if bat_level == 1 then
               tutorial_msg(28)
            elseif bat_level == 5 then
               tutorial_msg(37)
            else
               generic_bat_success_msg()
            end

            -- Open the doors
            open_bat_doors()

            -- Increase level for next time
            bat_level = bat_level + 1

            -- Deactivate the bat task.
            bat_task_active = false
         end
      end

      -- Check if it is time to spawn a new bat
      if bat_count < bats_remaining and kts.GameTime() >= next_spawn_at then
         next_spawn_at = 1e99
         spawn_bat()
         schedule_bat()
      end
   end

   -- Sleep for a bit
   coroutine.yield(250)
   return bat_task()  -- tail call
end

-- This is called from respawn_func(), if player died while bats active.
function reset_bat_task()
   bat_task_active = false
   bat_count = 0
   bats_remaining = 0
   for k,v in pairs(bat_list) do
      kts.DestroyCreature(v, "zombie")
   end
   bat_list = {}
   update_quest_rqmts()
end
   
function _G.rf_vampire_bats()

   -- pressing the switch shifts your respawn point to the middle of the bat chamber
   respawn_x = 26
   respawn_y = 31
   respawn_dir = "south"

   -- start the bat game (if it is not already in progress)
   if not bat_task_active then

      bat_actor = cxt.actor

      if bat_level > 5 then
         print("That's enough messing around with bats. Now go finish the Tutorial like "
               .. "you are supposed to be doing.")
         return
      end

      print ("Level " .. bat_level .. ". Fight!")

      -- Seal them in the room
      close_bat_doors()
      
      -- Signal the bat spawning task to start.
      bat_task_active = true

      -- Set difficulty settings.
      if bat_level == 1 then
         bats_remaining = 5
         bat_schedule = { 10 }
      elseif bat_level == 2 then
         bats_remaining = 8
         bat_schedule = { 1.7, 6 }
      elseif bat_level == 3 then
         bats_remaining = 10
         bat_schedule = { 1.5, 3, 6 }
      elseif bat_level == 4 then
         bats_remaining = 12
         bat_schedule = { 1, 2, 4, 6 }
      else
         bats_remaining = 15
         bat_schedule = { 1, 2, 3, 4, 6 }
      end

      bat_schedule[0] = 1 - 0.1 * (bat_level-1)
      bat_schedule.def = 8

      update_quest_rqmts()

      bat_start_msg()
   end
end


bat_2_msg_seen = false
bat_5_msg_seen = false

function bat_start_msg()

   if bat_level == 2 and not bat_2_msg_seen then
      tutorial_msg(32)
      bat_2_msg_seen = true
   elseif bat_level == 5 and not bat_5_msg_seen then 
      tutorial_msg(36)
      bat_5_msg_seen = true
   else
      local t
      if bat_level == 1 then
         t = bat_mission_title
      else
         t = string.format(bat_mission_title_2, bat_level)
      end
      kts.PopUpWindow{title = t, 
                      graphics = {C.g_wooden_floor, C.g_vbat_1},
                      body = string.format(bat_mission_text, bats_remaining)}
   end
end

function generic_bat_failed_msg()
   kts.PopUpWindow{title = bat_failed_title,
                   graphics = {C.g_wooden_floor, C.g_vbat_1},
                   body = bat_failed_text}
end

function generic_bat_success_msg()
   kts.PopUpWindow{title = bat_success_title,
                   graphics = {C.g_wooden_floor, C.g_vbat_1},
                   body = bat_success_text}
end

----------------------------------------------------------------------
-- Skull Puzzle 
----------------------------------------------------------------------

skull_x = 11

skull_tile = kts.Tile {
   on_walk_over = function()
      kts.AddMissile( {x=skull_x, y=cxt.tile_pos.y},
                      "east",
                      low_damage_bolt,
                      false )
      C.click_sound(cxt.pos)
      C.crossbow_sound(cxt.pos)
      tutorial_msg(50)
   end,

   depth = 1
}

function setup_skull_at(x, y)
   kts.AddTile( {x=x,y=y}, skull_tile )
end

function setup_skull(x)
   local choice = kts.RandomRange(0, 2)
   setup_skull_at(x, 21 + choice)
   setup_skull_at(x, 21 + (choice+1) % 3)
end


----------------------------------------------------------------------
-- Victory Conditions
----------------------------------------------------------------------

function check_gems()
   if kts.GetNumHeld(cxt.actor, C.i_gem) >= 5 then
      kts.WinGame(cxt.actor)
   else
      kts.FlashMessage("5 Gems Required")
      tutorial_msg(11)
   end
end

-- Modify the home on_approach actions, because we don't use the
-- standard Dsetup system.
-- (Also add tutorial hints...)
C.t_home_north.on_approach = check_gems
C.t_home_south.on_approach = nil

C.t_home_north.on_hit = function() tutorial_msg(11) end


----------------------------------------------------------------------
-- Tutorial Messages
----------------------------------------------------------------------

function tutorial_msg(x)
   print("Tutorial " .. x)
   local t = messages[x]

   if t ~= nil then
      if t[1] then 
         -- List of messages
         for _,v in pairs(t) do
            kts.PopUpWindow(v)
         end
      else
         -- Single message
         kts.PopUpWindow(t)
      end

      -- Each message should only be shown once
      messages[x] = nil
   end
end


function add_msg(x, y, n)
   local pos = {x=x,y=y}
   local tiles = kts.GetTiles(pos)
   
   -- For 'chest' and 'switch' tiles, we want to use on_activate
   -- Otherwise use on_walk_over
   local f = "on_walk_over"
   for _,v in pairs(tiles) do
      if v.table == C.t_chest_north.table
         or v.table == C.t_chest_east.table
         or v.table == C.t_chest_south.table
         or v.table == C.t_chest_west.table
         or v.table == C.t_switch_up.table
         or v.table == C.t_switch_down.table then
            f = "on_activate"
            break
      end
   end
   
   local t = { depth=2 }
   if type(n) == "function" then
      t[f] = n
   else
      t[f] = function() tutorial_msg(n) end
   end
   kts.AddTile(pos, kts.Tile(t))
end

function gem_hint()
   if kts.GetNumHeld(cxt.actor, C.i_gem) == 0 then
      tutorial_msg(20)
   end
end

function almost_home_msg()
   if kts.IsDoorOpen{x=3,y=11} then
      if kts.GetNumHeld(cxt.actor, C.i_gem) >= 5 then
         tutorial_msg(68)
      else
         tutorial_msg(69)
      end
   end
end

function add_msgs()
   add_msg(  7,  2,  1 )
   add_msg(  7,  7,  2 )
   add_msg(  7, 12,  4 )
   add_msg(  6, 13,  5 )
   add_msg(  7, 15,  8 )
   add_msg( 11, 12,  8 )
   add_msg(  1, 20, 12 )
   add_msg( 16, 17, 13 )
   add_msg( 18, 17, 13 )
   add_msg( 20, 17, 13 )
   add_msg( 16, 11, 14 )
   add_msg( 18, 11, 14 )
   add_msg( 20, 11, 14 )
   add_msg( 22, 10, 17 )
   add_msg( 22,  9, 19 )
   add_msg( 24, 13, gem_hint )
   add_msg( 31, 17, 21 )
   add_msg( 32, 17, 22 )
   add_msg( 31, 20, 23 )
   add_msg( 25, 20, 24 )
   add_msg( 27, 20, 24 )
   add_msg( 27, 27, 26 )
   add_msg( 22, 31, 38 )
   add_msg( 30, 31, 40 )
   add_msg( 31, 33, 41 )
   add_msg( 32, 33, 43 )
   add_msg( 33, 33, 44 )
   add_msg( 34, 31, 45 )
   add_msg( 19, 22, 49 )
   add_msg( 12, 25, 51 )
   add_msg( 24, 23, 52 )
   add_msg( 16, 30, 52 )
   add_msg( 11, 29, 52 )
   add_msg(  6, 29, 52 )
   add_msg(  3, 17, 52 )
   add_msg( 18, 36, 53 )
   add_msg( 16, 30, 55 )
   add_msg( 18, 28, 55 )
   add_msg( 14, 27, 56 )
   add_msg( 18, 28, 56 )
   add_msg( 20, 26, 57 )
   add_msg(  9, 28, 58 )
   add_msg(  6, 29, 58 )
   add_msg(  5, 33, 59 )
   add_msg( 10, 36, 64 )
   add_msg(  4, 35, 65 )
   add_msg(  5, 26, 66 )
   add_msg(  5, 22, 67 )
   add_msg(  6, 10, almost_home_msg )
end

-- Messages when certain items are picked up
C.i_hammer.on_pick_up = function() tutorial_msg(6) end
C.i_key1.on_pick_up = function() tutorial_msg(39) end
C.i_lockpicks.on_pick_up = function() tutorial_msg(54) end
C.i_bear_trap_open.on_pick_up = function() tutorial_msg(72) end
C.i_poison_trap.on_pick_up = function() tutorial_msg(73) end
C.i_blade_trap.on_pick_up = function() tutorial_msg(74) end

-- Message when try to open locked iron door near start point
old_unlock_fail = C.t_iron_door_horiz.on_unlock_fail
C.t_iron_door_horiz.on_unlock_fail = function()
   old_unlock_fail()
   print("HERE")
   if cxt.actor_pos.y == 10 then
      tutorial_msg(3)
   elseif kts.GetNumHeld(cxt.actor, C.i_lock_picks) >= 1 then
      tutorial_msg(75)
   else
      tutorial_msg(76)
   end
end

-- Message when try to open locked door to skull room
old_unlock_fail_vert = C.t_iron_door_vert.on_unlock_fail
C.t_iron_door_vert.on_unlock_fail = function()
   old_unlock_fail_vert()
   tutorial_msg(48)
end

-- Message when try to open locked wooden door
old_unlock_fail_2 = C.t_door_vert_locked.on_unlock_fail
C.t_door_vert_locked.on_unlock_fail = function()
   old_unlock_fail_2()
   tutorial_msg(9)
end

-- Message when try to open locked chest
old_unlock_fail_chest = C.t_chest_south.on_unlock_fail
C.t_chest_south.on_unlock_fail = function()
   old_unlock_fail_chest()
   tutorial_msg(62)
end

-- Message when hit closed iron door
old_on_hit_v = C.t_iron_door_vert.on_hit
C.t_iron_door_vert.on_hit = function()
   old_on_hit_v()
   tutorial_msg(10)
end

old_on_hit_h = C.t_iron_door_horiz.on_hit
C.t_iron_door_horiz.on_hit = function()
   old_on_hit_h()
   tutorial_msg(10)
end


-- Messages when gems picked up
gem_pickup_flag = { false,false,false,false,false }
C.i_gem.on_pick_up = function()
   local n = kts.GetNumHeld(cxt.actor, C.i_gem)

   -- Show each gem msg only once
   if gem_pickup_flag[n] then return end
   gem_pickup_flag[n] = true

   if n == 1 then
      tutorial_msg(18)
   else
      local m = { title = gem_title,
                  graphics = {C.g_floor1, C.g_gem} }
      if n < 4 then
         m.body = string.format(gem_msg, n, 5 - n)
      elseif n == 4 then
         m.body = penultimate_gem_msg
      else
         m.body = last_gem_msg
      end
      kts.PopUpWindow(m)
   end
end

