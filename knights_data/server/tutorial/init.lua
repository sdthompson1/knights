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

-- This is a "fake" stuff bag item that gives you a gem when you pick
-- it up.
stuff_with_gem = kts.ItemType {
   type = "magic",
   graphic = C.g_stuff_bag,
   on_pick_up = function()
      -- Give the actor one gem
      kts.GiveItem(cxt.actor, C.i_gem, 1)
   end
}

-- A version of t_switch_up that appears as a wall on the mini map.
switch_new = kts.Tile( C.t_switch_up.table & { map_as = "wall" })


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
tiles[107] = { C.t_floor1, C.i_potion }
tiles[108] = { C.t_floor1, C.i_scroll }
tiles[109] = { C.t_table_small, C.i_gem }
tiles[110] = { C.t_floor1, stuff_with_gem }
tiles[111] = { C.t_floor1, C.m_zombie }
tiles[113] = { C.t_floorpp, C.i_gem }
tiles[114] = { C.t_table_south, C.i_gem }
tiles[115] = switch_new

tutorial_segs = kts.LoadSegments(tiles, "tutorial_map.txt")


----------------------------------------------------------------------
-- Respawn function
----------------------------------------------------------------------

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

   --return 7, 2, "south"  -- start point
   --return 16, 18, "east"  -- start of switch puzzle
   --return 24, 10, "south"  -- room beyond first gem.
   --return 32, 31, "south"  -- chest room, east of bat chamber
   --return 29,31,"west" -- in bat chamber (by east door)
   return 12,25,"south"  -- after skull room
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

   -- Install custom respawn function
   kts.SetRespawnFunction(respawn_func)


   -- TODO: Set "home" so they can heal from starting point.
   -- TODO: Set exit point to "same as entry"
   -- TODO: Set Gems Required
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
             {x=27, y=31},
             {x=24, y=32}, {x=28,y=32},
             {x=26, y=33},
             {x=23, y=34}, {x=29,y=34} }
num_bat_pits = 11

bat_count = 0
bat_list = {}

function spawn_bat()
   local n = kts.RandomRange(1, num_bat_pits)
   local bat = kts.AddMonster(bat_pits[n], C.m_vampire_bat)
   bat_count = bat_count + 1
   table.insert(bat_list, bat)
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
bat_schedule = { 1, 10, def=10 }   -- In seconds.

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

   if not bat_task_active then 

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
         bat_schedule = { 2, 6 }
      elseif bat_level == 3 then
         bats_remaining = 10
         bat_schedule = { 2, 3, 6 }
      else
         bats_remaining = 12 + 2 * (bat_level-4)
         bat_schedule = { 2, 2, 3, 5, 6 }
         for i = 5,bat_level do
            bat_schedule[i-2] = 2
         end
      end

      bat_schedule[0] = 1.2 - 0.2*bat_level
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
                      C.i_bolt_trap,
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
