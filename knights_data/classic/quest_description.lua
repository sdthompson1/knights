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

function describe_exit_point(S)
   if S.exit == "same" then
      return "your entry point"
   elseif S.exit == "other" then
      return "your opponent's entry point"
   elseif S.exit == "random" then
      return "an unknown exit point"
   elseif S.exit == "guarded" then
      return "the guarded exit"
   else
      return "(??)"
   end
end

function describe_mission(S)
   local result

   if S.mission == "duel_to_the_death" then
      result = "You must secure all entry points to prevent your opponents entering the dungeon"
   elseif S.mission == "destroy_book" then
      result = "You must strike the book with the wand in the special pentagram"
   else
      if S.mission == "escape" then
         result = "Your mission is to "
      else
         result = "You must retrieve the "
         if S.mission == "retrieve_book" then
            result = result .. "book"
         else
            result = result .. "wand"
         end
         result = result .. " and "
      end
      result = result .. "escape via " .. describe_exit_point(S)
   end
   
   if S.gems_needed > 0 then
      result = result .. " with " .. S.gems_needed .. " out of " .. S.num_gems .. " gems"
   end

   result = result .. "."

   return result
end

function describe_book(S)
   if S.book == "ashur" then
      return "The Lost Book of Ashur is somewhere in the dungeon."
   elseif S.book == "gnomes" then
      return "The Tome of Gnomes is hidden behind unknown traps and riddles."
   elseif S.book == "knowledge" then
      return "The Book of Knowledge reveals knowledge of the dungeon."
   elseif S.book == "necronomicon" then
      return "The Necronomicon is sealed behind locked doors. It has powers to raise the undead."
   end
end

function describe_wand(S)
   if S.wand == "destruction" then
      return "The Wand of Destruction terminates targets."
   elseif S.wand == "open_ways" then
      return "The Wand of Open Ways may be used to open any item."
   elseif S.wand == "securing" then
      return "The Wand of Securing is used to secure entry points."
   elseif S.wand == "undeath" then
      return "The Wand of Undeath controls and slays the undead."
   end
end

function describe_time(S)
   if S.time > 0 then

      local minutes
      if S.time == 1 then 
         minutes = " minute."
      else
         minutes = " minutes." 
      end

      if S.mission == "deathmatch" then
         return "The game will last for " .. S.time .. minutes
      else
         return "You must complete this quest within " .. S.time .. minutes
      end
   end
end

function describe_quest(S)

   local result

   if S.quest ~= "custom" and quest_table[S.quest].description ~= nil then
      result = quest_table[S.quest].description(S)

   elseif S.mission == "deathmatch" then
      result = "Deathmatch\n\n"
         .. "Players get 1 point for killing an enemy knight, and -1 for a suicide. "
         .. "(Being killed by a monster doesn't affect your score.) "
         .. "The player with the highest score when time runs out is the winner."

   else
      if S.quest == "custom" then 
         result = "Custom Quest\n\n"
      end

      result = result .. describe_mission(S)

      local book_desc = describe_book(S)
      if book_desc then
         result = result .. "\n\n" .. book_desc 
      end

      local wand_desc = describe_wand(S)
      if wand_desc then
         result = result .. "\n\n" .. wand_desc
      end
   end

   local time_desc = describe_time(S)
   if time_desc then
      result = result .. "\n\n" .. time_desc
   end

   return result
end
   