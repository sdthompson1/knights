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

local C = require("classic")


----------------------------------------------------------------------
-- New tiles and items
----------------------------------------------------------------------

-- Prevent bats flying over switches
new_acc = C.switch_acc & { flying="blocked" }
C.t_switch_up.access = new_acc
C.t_switch_down.access = new_acc

-- A version of t_switch_up that appears as a wall on the mini map.
switch_new = kts.Tile( C.t_switch_up.table & { map_as = "wall" })

-- Potions/scrolls with fixed effects
function make_potion(effect)
   return kts.ItemType {
      type = "magic",
      graphic = C.g_potion,
      fragile = true,
      on_pick_up = function()
         C.snd_drink()
         kts.Delay(750)
         effect()
      end
   }
end

function make_scroll(effect)
   return kts.ItemType {
      type = "magic",
      graphic = C.g_scroll,
      on_pick_up = function() 
         C.zap() 
         effect()
      end
   }
end

poison_potion = make_potion(C.poison)
regeneration_potion = make_potion(C.regeneration)
super_potion = make_potion(C.super)
quickness_scroll = make_scroll(C.quickness)

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
tiles[97]  = { C.t_floor7, C.i_hammer }
tiles[98]  = { C.t_floor1, C.i_lockpicks }
tiles[99]  = { C.t_floor1, C.i_poison_trap }
tiles[100] = { C.t_floor1, C.i_blade_trap }
tiles[101] = { C.t_floor1, C.i_staff }
tiles[102] = { C.t_floor1, C.i_bear_trap }
tiles[103] = { C.t_floor1, C.i_axe }
tiles[104] = { C.t_floor1, {C.i_dagger,8} }
tiles[105] = { C.t_floor1, C.i_crossbow }
tiles[106] = { C.t_floor1, C.i_bolts }
tiles[108] = { C.t_floor1, quickness_scroll }
tiles[111] = { C.t_floor1, C.m_zombie }
tiles[113] = { C.t_floorpp, C.i_gem }
tiles[114] = { C.t_table_south, C.i_gem }
tiles[115] = switch_new

tutorial_segs = kts.LoadSegments(tiles, "tutorial_map.txt")


----------------------------------------------------------------------
-- Custom respawn function
----------------------------------------------------------------------

respawn_x = 26 --  7
respawn_y = 34 --  2
respawn_dir = "south"

function respawn_func()

   -- Turn off the "chamber of bats" if it is active
   if bat_task_active then
      -- turn off bat spawning, remove all bats
      reset_bat_task()

      -- if lvl 2 or above, open the doors.
      if bat_level > 1 then
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


   -- TODO: Set "home" so they can heal from starting point.
   -- TODO: Set exit point to "same as entry"
   -- TODO: Set Gems Required = 4
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

function _G.rf_sw_puzzle()

   -- Find out whether we have hit this particular switch before
   -- (Use x coord of switch as index into "switches_hit" table)

   -- NOTE: If they have already hit two switches then this test is
   -- disabled, this allows them to re-open the second gate after it
   -- was shut behind them.

   if num_switches_hit >= 2   -- Already hit two switches,
   or switches_hit[cxt.tile_pos.x] == nil    -- or, first time we've hit this switch
   then

      -- Remember that we have hit this switch
      switches_hit[cxt.tile_pos.x] = true
      num_switches_hit = num_switches_hit + 1

      -- Work out which gate to open
      local pos
      if num_switches_hit == 1 then
         pos = door1_pos
      else
         pos = door2_pos
      end

      -- Open the gate.
      -- We assume there is only one tile on the gate square.
      -- We remove that tile, then add its "open_to" tile instead.
      local tiles = kts.GetTiles(pos)
      if tiles[1].open_to ~= nil then
         kts.RemoveTile(pos, tiles[1])
         kts.AddTile(pos, tiles[1].open_to)
      end

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

function update_bat_rqmts()
   kts.ClearHints()
   if bats_remaining > 0 then
      local msg = "Kill " .. bats_remaining .. " bat"
      if bats_remaining ~= 1 then
         msg = msg .. "s" 
      end
      kts.AddHint(msg, 1, 1)
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
         update_bat_rqmts()
         
         -- Reschedule next spawn if required
         schedule_bat()

         -- Check if they just won
         if bats_remaining <= 0 then

            print("Mission Complete.")

            -- Open the doors
            kts.OpenDoor(bat_door_1)
            kts.OpenDoor(bat_door_2)
            kts.OpenDoor(bat_door_3)

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

function reset_bat_task()
   bat_task_active = false
   bat_count = 0
   bats_remaining = 0
   for k,v in pairs(bat_list) do
      kts.DestroyCreature(v, "zombie")
   end
   bat_list = {}
   update_bat_rqmts()
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
         print("You have completed all the available bat levels. Now go and finish the Tutorial like "
               .. "you are supposed to be doing.")
         return
      end

      print ("Level " .. bat_level .. ". Fight!")

      -- Seal them in the room
      kts.CloseDoor(bat_door_1)
      kts.CloseDoor(bat_door_2)
      kts.CloseDoor(bat_door_3)
      
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

      update_bat_rqmts()
   end
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