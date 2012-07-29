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


-- Default value of Dsetup. We reset Dsetup to this each time a new game starts

dungeon_setup = {
   special_segments   = {},  -- list of { segment_list }
   required_items     = {},  -- list of { itemtype, number, weight_table }
   stuff              = {},  -- list of { category, probability, generator }
   initial_monsters   = {}   -- list of { montype, number_initially }
}


-- Quests

function quest_retrieve(itemtypes, qty, msg_singular, msg_plural)
   kts.AddQuest("approach_exit",
                make_retrieve_handler( itemtypes, qty, msg_singular, msg_plural ))
end

function quest_destroy(booklist, 
                       wandlist, wrong_wand_msg, 
                       tilelist, not_in_pentagram_msg)
   kts.AddQuest("hit_book",
                make_destroy_handler( booklist, wandlist, wrong_wand_msg,
                                      tilelist, not_in_pentagram_msg ))
end

function quest_deathmatch()
   kts.SetDeathmatchMode(true)
end

function hint(msg, order, group)
   kts.AddHint(msg, order, group)
end


-- Dungeon Customization

function set_layout(layout)
   Dsetup.layout = layout
end

function add_segment(segment_list)
   table.insert(Dsetup.special_segments, segment_list)
end

function set_entry(entrytype)
   Dsetup.entry_type = entrytype
end

function set_exit(exittype)
   Dsetup.exit_type = exittype
end

function add_item(itemtype, qty, weights)
   table.insert(Dsetup.required_items, { itemtype, qty, weights })
end

function add_stuff(category, probability, generator)
   table.insert(Dsetup.stuff, { category, probability, generator })
end

function set_stuff_respawning(items, time_in_ms)
   kts.SetStuffRespawning(items, time_in_ms)
end

function set_keys(num, lockpicks, keylist, weights)
   Dsetup.num_keys = num

   -- Add one set of lockpicks, and one each of the keys
   add_item(lockpicks, 1, weights)
   for i = 1,num do
      add_item(keylist[i], 1, weights)
   end

   -- Start spawning lockpicks after a certain time
   kts.SetLockpickSpawn(lockpicks, 1000 * 60 * 6, 1000 * 60 * 2)
end

function set_pretrapped()
   Dsetup.pretrapped = true
end


-- Monsters

function add_initial_monsters(montype, number)
   table.insert(Dsetup.initial_monsters, { montype, number })
end

function add_monster_generator(montype, tiletypes, probability)
   kts.AddMonsterGenerator(montype, tiletypes, probability)
end

function add_monster_limit(montype, number)
   kts.LimitMonster(montype, number)
end

function set_total_monster_limit(n)
   kts.LimitTotalMonsters(n)
end

function set_zombie_activity(probability, activity_table)
   kts.SetZombieActivity(probability, activity_table)
end


-- Knight Starting Parameters

function set_premapped()
   kts.SetPremapped()
end

function add_gear(gear)
   kts.AddStartingGear(table.unpack(gear))
end


-- Time Limit

function set_time_limit(secs)
   -- TODO
end


-- Dungeon Generation Code

function try_layout()
   -- This function does the basic dungeon layout.

   -- Choose which special segments to use
   local special_segs = { }

   for k,v in ipairs(Dsetup.special_segments) do
      -- v is a list of special segments. Pick one at random.
      local choice = kts.RandomRange(1, #v)
      table.insert(special_segs, v[choice])
   end

   -- Do the basic dungeon layout
   kts.LayoutDungeon{
      layout = Dsetup.layout.func(),  -- generate a random layout

      wall = t_wall_normal,
      horiz_door = { t_door_horiz, t_hdoor_background },
      vert_door = { t_door_vert, t_vdoor_background },

      segments = standard_rooms,
      special_segments = special_segs,

      entry_type = Dsetup.entry_type
   }
end

function setup_keys_locks_traps()
   -- Set up locks and traps
   kts.GenerateLocksAndTraps(Dsetup.num_keys, Dsetup.pretrapped)
end

function setup_items()
   -- Generate required items
   for i,v in ipairs(Dsetup.required_items) do
      kts.AddItem(v[1], v[2], v[3])
   end

   -- Generate "stuff" items
   kts.AddStuff(Dsetup.stuff)
end

function setup_monsters()
   -- Add initial monsters
   for i,v in ipairs(Dsetup.initial_monsters) do
      kts.AddMonsters(v[1], v[2])
   end
end

function final_checks()
   kts.ConnectivityCheck(Dsetup.num_keys)
end

function try_to_generate_dungeon()
   -- This function tries once to generate a dungeon.
   -- If it fails, kts.DUNGEON_ERROR will be non-nil.

   -- First wipe out any previous dungeon (this also clears DUNGEON_ERROR)
   kts.WipeDungeon()

   -- Do basic dungeon layout
   try_layout()
   if kts.DUNGEON_ERROR then return end
   
   -- Fill in the details
   setup_keys_locks_traps()
   setup_items()
   setup_monsters()
   final_checks()
end

function generate_dungeon()
   -- This function makes multiple attempts to generate a dungeon,
   -- until we either succeed, or give up. 

   local NUM_ATTEMPTS = 25
   local orig_layout = Dsetup.layout

   while true do
      for i = 1, NUM_ATTEMPTS do

         -- Make a dungeon generation attempt
         try_to_generate_dungeon()

         -- If it was successful, then exit the for loop
         if not kts.DUNGEON_ERROR then
            break
         end
      end

      if kts.DUNGEON_ERROR then
         -- All of the dungeon generation attempts failed.
         -- Go on to next layout.
         Dsetup.layout = Dsetup.layout.next

         -- If no next layout exists, we have to give up.
         if Dsetup.layout == nil then return end

      else
         -- Successful dungeon, can quit the while loop
         break
      end
   end

   if Dsetup.layout ~= orig_layout then
      print("Could not generate a \"" .. orig_layout.name .. 
            "\" dungeon for this quest. Using \"" .. Dsetup.layout.name ..
            "\" instead.")
   end
end


-- Game Startup Code.

function deep_copy(old)
   if type(old) == "table" then
      local new = {}
      for k,v in pairs(old) do
         new[k] = deep_copy(v)
      end
      return new
   else
      return old
   end
end

function prepare_game(S)
   -- Make a deep copy of dungeon_setup table
   Dsetup = deep_copy(dungeon_setup)
end

function start_game()
   -- Generate the dungeon
   generate_dungeon()
end
