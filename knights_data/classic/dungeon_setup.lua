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


-- Table of default settings

Dsetup = {
   special_segments   = {},  -- list of { segment_list }
   required_items     = {},  -- list of { itemtype, number, weight_table }
   stuff              = {},  -- list of { category, probability, generator }
   initial_monsters   = {},  -- list of { montype, number_initially }
}


-- Quests

function Dsetup:quest_retrieve(itemtypes, qty, msg_singular, msg_plural)
   kts.AddQuest("approach_exit",
                make_retrieve_handler( itemtypes, qty, msg_singular, msg_plural ))
end

function Dsetup:quest_destroy(booklist, 
                              wandlist, wrong_wand_msg, 
                              tilelist, not_in_pentagram_msg)
   kts.AddQuest("hit_book",
                make_destroy_handler( booklist, wandlist, wrong_wand_msg,
                                      tilelist, not_in_pentagram_msg ))
end

function Dsetup:quest_deathmatch()
   kts.SetDeathmatchMode(true)
end

function Dsetup:hint(msg, order, group)
   kts.AddHint(msg, order, group)
end


-- Dungeon Customization

function Dsetup:set_layout(layout)
   self.layout = layout
end

function Dsetup:add_segment(segment_list)
   table.insert(self.special_segments, segment_list)
end

function Dsetup:set_entry(entrytype)
   self.entry_type = entrytype
end

function Dsetup:set_exit(exittype)
   self.exit_type = exittype
end

function Dsetup:add_item(itemtype, qty, weights)
   table.insert(self.required_items, { itemtype, qty, weights })
end

function Dsetup:add_stuff(category, probability, generator)
   table.insert(self.stuff, { category, probability, generator })
end

function Dsetup:set_stuff_respawning(items, time_in_ms)
   kts.SetStuffRespawning(items, time_in_ms)
end

function Dsetup:set_keys(num, lockpicks, keylist, weights)
   self.num_keys = num

   -- Add one set of lockpicks, and one each of the keys
   self:add_item(lockpicks, weights)
   for i = 1,num do
      self:add_item(keylist[i], 1, weights)
   end

   -- Start spawning lockpicks after a certain time
   kts.SetLockpickSpawn(lockpicks, 1000 * 60 * 6, 1000 * 60 * 2)
end

function Dsetup:set_pretrapped()
   self.pretrapped = true
end


-- Monsters

function Dsetup:add_initial_monsters(montype, number)
   table.insert(self.initial_monsters, { montype, number })
end

function Dsetup:add_monster_generator(montype, tiletypes, probability)
   kts.AddMonsterGenerator(montype, tiletypes, probability)
end

function Dsetup:add_monster_limit(montype, number)
   kts.LimitMonster(montype, number)
end

function Dsetup:set_total_monster_limit(n)
   kts.LimitTotalMonsters(n)
end

function Dsetup:set_zombie_activity(probability, activity_table)
   kts.SetZombieActivity(probability, activity_table)
end


-- Knight Starting Parameters

function Dsetup:set_premapped()
   -- Premapped has to be done after the dungeon has been generated,
   -- therefore just set a flag and do the actual kts call later.
   self.premapped = true
end

function Dsetup:add_gear(gear)
   kts.AddStartingGear(table.unpack(gear))
end



-- Dungeon Generation Code

function try_layout(D)
   -- This function does the basic dungeon layout.

   -- Choose which special segments to use
   local special_segs = { }

   for k,v in ipairs(D.special_segments) do
      -- v is a list of special segments. Pick one at random.
      local choice = kts.RandomRange(1, #v)
      table.insert(special_segs, v[choice])
   end

   -- Do the basic dungeon layout
   kts.LayoutDungeon{
      layout = D.layout.func(),  -- generate a random layout

      wall = t_wall_normal,
      horiz_door = { t_door_horiz, t_hdoor_background },
      vert_door = { t_door_vert, t_vdoor_background },

      segments = standard_rooms,
      special_segments = special_segs,

      entry_type = D.entry_type,
      exit_type = D.exit_type
   }
end

function setup_keys_locks_traps(D)
   -- Set up locks and traps
   kts.GenerateLocksAndTraps(D.num_keys, D.pretrapped)
end

function setup_items(D)
   -- Generate required items
   for i,v in ipairs(D.required_items) do
      kts.AddItem(v[0], v[1], v[2])
   end

   -- Generate "stuff" items
   kts.AddStuff(D.stuff)
end

function setup_monsters(D)
   -- Add initial monsters
   for i,v in ipairs(D.initial_monsters) do
      kts.AddMonsters(v[0], v[1])
   end
end

function final_checks(D)
   kts.ConnectivityCheck(D.num_keys)
end

function try_to_generate_dungeon(D)
   -- This function tries once to generate a dungeon.
   -- If it fails, kts.DUNGEON_ERROR will be non-nil.

   -- First wipe out any previous dungeon (this also clears DUNGEON_ERROR)
   kts.WipeDungeon()

   -- Do basic dungeon layout
   try_layout(D)
   if kts.DUNGEON_ERROR then return end
   
   -- Fill in the details
   setup_keys_locks_traps(D)
   setup_items(D)
   setup_monsters(D)
   final_checks(D)
end

function generate_dungeon(D)
   -- This function makes multiple attempts to generate a dungeon,
   -- until we either succeed, or give up. 

   local NUM_ATTEMPTS = 25
   local orig_layout = D.layout

   while true do
      for i = 1, NUM_ATTEMPTS do

         -- Make a dungeon generation attempt
         try_to_generate_dungeon(D)

         -- If it was successful, then exit the for loop
         if not kts.DUNGEON_ERROR then
            break
         end
      end

      if kts.DUNGEON_ERROR then
         -- All of the dungeon generation attempts failed.
         -- Go on to next layout.
         D.layout = D.layout.next

         -- If no next layout exists, we have to give up.
         if D.layout == nil then return end

      else
         -- Successful dungeon, can quit the while loop
         break
      end
   end

   if D.layout ~= orig_layout then
      print("Could not generate a \"" .. orig_layout.name .. 
            "\" dungeon for this quest. Using \"" .. D.layout.name ..
            "\" instead.")
   end
end


-- Game Startup Code.

function start_game(D)
   -- Generate the dungeon
   generate_dungeon(D)

   -- Premap it if required.
   if D.premapped then
      kts.MagicMappingAll() 
   end
end
