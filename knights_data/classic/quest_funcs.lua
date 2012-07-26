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


------ Part I: Helper functions ------

-- Check whether 'thing' is one of the things in 'thinglist'
-- (Returns true/false)
function is_one_of(thing, thinglist)
   for _,t2 in ipairs(thinglist) do
      -- if two "things" have the same table, then consider them to be the "same"
      if thing.table == t2.table then
         -- Found it
         return true
      end
   end
   return false
end

-- Returns how many more of the given item the knight requires.
-- (Will be zero, or negative, if they already have enough.)
function how_many_more_needed(itemtypes, num_needed)
   local num_held 
   for _,itype in ipairs(itemtypes) do
      num_held = num_held + kts.GetNumHeld(cxt.actor, itype)
   end

   return num_needed - num_held
end

-- Check the knight is holding the required item, and if not,
-- call QuestFail with a suitable msg
function check_item(itemtypes, num_needed, sing, pl)
   local shortfall = how_many_more_needed(itemtypes, num_needed)
   if shortfall > 1 and pl ~= nil then
      kts.QuestFail(string.format(pl, shortfall))
   elseif shortfall > 0 then
      kts.QuestFail(sing)
   end
end

-- Check a required tile exists at a given square
-- Returns true/false
function is_tile_one_of(pos, tilelist, fail_msg)
   local tiles_here = kts.GetTiles(pos)
   for _, t in tiles_here do
      if is_one_of(t, tilelist) then
         tile_found = true
         break
      end
   end
   return tile_found
end


------ Part II: The actual quest handlers ------


-- make_retrieve_handler:
-- Handles "retrieve item" type quests
-- Used for both "retrieve X and escape", and any gem requirements in "destroy book with wand" quests.
--
-- itemtypes = list of acceptable itemtypes
--    e.g. { i_gem } or { i_some_wand, i_some_other_wand } etc.
-- qty = number required
-- sing_msg = singular message e.g. "Gem Required"
-- pl_msg   = plural message   e.g. "%d Gems Required"  (can be nil)
function make_retrieve_handler(itemtypes, qty, sing_msg, pl_msg)
   return function()
      check_item(itemtypes, qty, sing_msg, pl_msg)
   end
end

   
-- make_destroy_book_handler:
-- Handles "destroy book with wand" quests
--
-- booklist should be a list of book itemtypes (that can be destroyed to win)
-- wandlist should be a list of wand itemtypes, that can be used to destroy the book
-- wrong_wand_msg is displayed if you strike the book with the wrong weapon.
-- tilelist should be a list of special pentagram tiles, where the book can be destroyed
-- not_special_pentagram_msg is displayed if you strike the book in the wrong tile
function make_destroy_book_handler(booklist,
                                   wandlist, 
                                   wrong_wand_msg,
                                   tilelist, 
                                   not_special_pentagram_msg)
   return function()
      -- Note: This runs as part of "on_hit" event of the book. So cxt.item is the book.

      -- Check the item being hit is one of the "books" -- if not, just exit w/o any msg
      if not is_one_of(cxt.item, booklist) then
         kts.QuestFail()
         return
      end

      -- Check the tile being hit is the special pentagram
      if not is_tile_one_of(cxt.item_pos, tilelist) then
         kts.QuestFail(not_special_pentagram_msg)
      end

      -- Check the weapon being used is the wand
      if not is_one_of(cxt.item, wandlist) then
         kts.QuestFail(wrong_wand_msg)
      end
   end
end
