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


------ Part I: Helper functions for quest handlers ------

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
   local num_held = 0
   for _,itype in ipairs(itemtypes) do
      num_held = num_held + kts.GetNumHeld(cxt.actor, itype)
   end

   return num_needed - num_held
end

-- Check the knight is holding the required item, in the required quantity.
--   If so:     returns true
--   Otherwise: calls FlashMessage with a suitable message, then returns false
function check_item(itemtypes, num_needed, msg_key)
    local shortfall = how_many_more_needed(itemtypes, num_needed)
    if shortfall > 0 then
        kts.FlashMessage({key=msg_key, params={num_needed}, plural=num_needed})
        return false
    else
        return true
    end
end

-- Check a required tile exists at a given square
-- Returns true/false
function is_tile_one_of(pos, tilelist, fail_msg)
   local tiles_here = kts.GetTiles(pos)
   for _, t in ipairs(tiles_here) do
      if is_one_of(t, tilelist) then
         return true
      end
   end
   return false
end


------ Part II: "Make quest handler" functions, called by dungeon_setup.lua ------

-- make_retrieve_handler:
-- Handles "retrieve item" type quests
-- Used for both "retrieve X and escape", and any gem requirements in "destroy book with wand" quests.
--
-- itemtypes = list of acceptable itemtypes
--    e.g. { i_gem } or { i_some_wand, i_some_other_wand } etc.
-- qty = number required
-- msg_key = Localization key for message; {0} and "plural" are both set to the number required
function make_retrieve_handler(itemtypes, qty, msg_key)
   return function()
      return check_item(itemtypes, qty, msg_key)
   end
end

   
-- make_destroy_handler:
-- Handles "destroy book with wand" quests
--
-- booklist should be a list of book itemtypes (that can be destroyed to win)
-- wandlist should be a list of wand itemtypes, that can be used to destroy the book
-- wrong_wand_msg is displayed if you strike the book with the wrong weapon.
-- tilelist should be a list of special pentagram tiles, where the book can be destroyed
-- not_special_pentagram_msg is displayed if you strike the book in the wrong tile
function make_destroy_handler(booklist,
                              wandlist, 
                              tilelist, 
                              not_special_pentagram_msg)
   return function()
      -- Note: This runs as part of "on_hit" event of the book. So cxt.item_type is the book.

      -- Check the item being hit is one of the "books" -- if not, just exit w/o any msg
      if not is_one_of(cxt.item_type, booklist) then
         return false
      end

      -- Check the weapon being used is the wand (if not, exit w/o any message)
      local weapon = kts.GetItemInHand(cxt.actor)
      if weapon == nil or not is_one_of(weapon, wandlist) then
         return false
      end

      -- Check the tile being hit is the special pentagram
      if not is_tile_one_of(cxt.item_pos, tilelist) then
         kts.FlashMessage(not_special_pentagram_msg)
         return false
      end

      -- Finally, check you have enough gems
      do_quest_check(Dsetup.retrieve_handlers)
   end
end


------ Part III: Quest checking implementation ------

function is_correct_exit()
   local exit = Dsetup.exit_type
   local player = kts.GetPlayer(cxt.actor)
   local e

   if exit == "self" then
      -- Exit is my own home
      e = kts.GetHomeFor(player)

   elseif exit == "other" then
      -- Exit is the other player's home
      -- (this only works for 2 players)
      local players = kts.GetAllPlayers()
      if #players ~= 2 then return false end
      if player == players[1] then
         e = kts.GetHomeFor(players[2])
      else
         e = kts.GetHomeFor(players[1])
      end

   else
      e = Dsetup.exit_location
   end

   if e == nil then return false end   -- No escape

   return ( e.x == cxt.actor_pos.x
            and e.y == cxt.actor_pos.y
            and e.facing == kts.GetFacing(cxt.actor) )
end

function do_quest_check(handlers)
   
   local result = true
   
   for _, v in ipairs(handlers) do
      result = v() and result
   end
   
   -- if all handlers returned true, then win the game.
   if result then
      kts.WinGame(cxt.actor)
   end
end


------ Part IV: Quest checking functions, called by items.lua and tiles.lua ------

function check_escape_quest()
   if is_correct_exit() then
      do_quest_check(Dsetup.retrieve_handlers)
   end
end

function check_destroy_quest()
   -- We check that at least one handler is present
   -- (otherwise, this isn't a destroy book with wand quest!)

   if Dsetup.destroy_handlers[1] ~= nil then

      -- Call the destroy handler
      -- Note: this in turn will call retrieve handlers (to check gems).
      do_quest_check(Dsetup.destroy_handlers)
   end
end
