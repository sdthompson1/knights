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

quest_table = {
   gems = {
      name_key = "quest_for_gems",
      description = function(S)
         return "Quest for Gems\n\n"
            .. "Deep within the dungeon are hidden four gems.\n\n"
            .. "You must recover at least three of them and then return to your entry point."
      end,
      settings = {
         mission = "escape",
         book = "none",
         wand = "none",
         num_wands = 0,
         num_gems = 4,
         gems_needed = 3,
         dungeon = "basic",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "same",
         num_keys = 3,
         pretrapped = true,
         stuff = 3,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 0
      }
   },
   big_gems = {
      name_key = "big_quest_for_gems",
      description = function(S)
         return "The Big Quest for Gems\n\n"
            .. "Deep within the vast dungeon are hidden six gems.\n\n"
            .. "You must recover at least four of them before returning to your entry point."
      end,
      settings = {
         mission = "escape",
         book = "none",
         wand = "none",
         num_wands = 0,
         num_gems = 6,
         gems_needed = 4,
         dungeon = "big",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "same",
         num_keys = 3,
         pretrapped = true,
         stuff = 3,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 0
      }
   },
   shock = {
      name_key = "shock_assault",
      description = function(S)
         return "Shock Assault\n\n"
            .. "You must make a shock assault on the entry point of the other knight. "
            .. "If you succeed before your enemy you are the winner.\n\n"
            .. "This quest is for 2 players only."
      end,
      min_players = 2,
      max_players = 2,
      min_teams = 2,
      max_teams = 2,
      settings = {
         mission = "escape",
         book = "none",
         wand = "none",
         num_wands = 0,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "snake",
         premapped = false,
         entry = "away",
         gear = "traps",
         exit = "other",
         num_keys = 1,
         pretrapped = false,
         stuff = 4,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 0
      }
   },
   survival = {
      name_key = "war_of_survival",
      description = function(S)
         return "War of Survival\n\n"
            .. "You must secure all entry points with the wand so that your opponents cannot use them, "
            .. "and then kill all enemy knights."
      end,
      min_teams = 2,   -- Duel to death doesn't really make sense with only one team.
      settings = {
         mission = "duel_to_death",
         book = "none",
         wand = "securing",
         num_wands = 2,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "big",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "none",
         num_keys = 3,
         pretrapped = true,
         stuff = 3,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 1
      }
   },
   dungeon_death = {
      name_key = "dungeon_of_death",
      description = function(S)
         return "The Dungeon of Death\n\n"
            .. "Out of the vampire bat infested dungeon you must find an exit which "
            .. "is behind locked doors and a swarm of bats."
      end,
      settings = {
         mission = "escape",
         book = "none",
         wand = "none",
         num_wands = 0,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "ring",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "guarded",
         num_keys = 3,
         pretrapped = true,
         stuff = 3,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 4
      }
   },
   gnomes = {
      name_key = "lost_book_of_gnome",
      description = function(S)
         return "The Lost Book of the Gnome King\n\n"
            .. "The ancient Gnome King left his book of wisdom within this dungeon.\n\n"
            .. "You must find it and carry it back to your entry point."
      end,
      settings = {      
         mission = "retrieve_book",
         book = "gnomes",
         wand = "none",
         num_wands = 0,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "big",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "same",
         num_keys = 3,
         pretrapped = true,
         stuff = 4,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 1
      }
   },
   liche = {
      name_key = "tomb_of_liche",
      description = function(S)
         return "The Tomb of the Liche Lord\n\n"
            .. "The Liche Lord is gone but his spell book is still in our universe. "
            .. "You must recover it from the Liche Lord's study."
      end,
      settings = {
         mission = "retrieve_book",
         book = "necronomicon",
         wand = "undeath",
         num_wands = 1,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "big",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "same",
         num_keys = 3,
         pretrapped = true,
         stuff = 5,
         stuff_respawn = "medium",
         zombies = 2,
         bats = 1
      }
   },
   wand_death = {
      name_key = "ancient_wand_of_death",
      description = function(S)
         return "The Ancient Wand of Death\n\n"
            .. "The Wand of Destruction is hidden in this dungeon.\n\n"
            .. "You must recover it and escape via "
            .. describe_exit_point(S)   -- it might not be Other's Entry (there may be >2 players)
            .. "."
      end,
      settings = {
         mission = "retrieve_wand",
         book = "none",
         wand = "destruction",
         num_wands = 1,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "basic",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "other",
         num_keys = 2,
         pretrapped = true,
         stuff = 4,
         stuff_respawn = "medium",
         zombies = 1,
         bats = 0
      }
   },
   paradise = {
      name_key = "way_to_paradise",
      description = function(S)
         return "The Way to the Paradise\n\n"
            .. "Legend has it that any hero who strikes the Book of Knowledge with "
            .. "the Wand of Open Ways in the Special Pentagram will be granted immortality.\n\n"
            .. "Your objective is to attain that status."
      end,
      settings = {
         mission = "destroy_book",
         book = "knowledge",
         wand = "open_ways",
         num_wands = 1,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "huge",
         premapped = false,
         entry = "random",
         gear = "none",
         exit = "none",
         num_keys = 3,
         pretrapped = true,
         stuff = 3,
         stuff_respawn = "medium",
         zombies = 1,
         bats = 0
      }
   },
   giants = {
      name_key = "quest_of_giants",
      description = function(S)
         return "The Quest of Giants\n\n"
            .. "You are put on the huge quest.\n\n"
            .. "You must retrieve the book and four out of six gems and escape via "
            .. describe_exit_point(S)
            .. "."
      end,
      settings = { 
         mission = "retrieve_book",
         book = "ashur",
         wand = "destruction",
         num_wands = 1,
         num_gems = 6,
         gems_needed = 4,
         dungeon = "huge",
         premapped = false,
         entry = "close",
         gear = "daggers",
         exit = "guarded",
         num_keys = 3,
         pretrapped = true,
         stuff = 4,
         stuff_respawn = "medium",
         zombies = 3,
         bats = 3
      }
   },
   freedom = {
      name_key = "run_for_freedom",
      description = function(S)
         return "Run for the Freedom\n\n"
            .. "You are trapped in the dungeon of vampire bats.\n\n"
            .. "You must run to the exit point and get out before your opponents."
      end,
      settings = {
         mission = "escape",
         book = "none",
         wand = "none",
         num_wands = 0,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "long_snake",
         premapped = true,
         entry = "close",
         gear = "none",
         exit = "guarded",
         num_keys = 1,
         pretrapped = false,
         stuff = 1,
         stuff_respawn = "medium",
         zombies = 0,
         bats = 5
      }
   },
   deathmatch = {
      name_key = "knights_deathmatch",
      min_teams = 2,    -- Deathmatch doesn't make sense with only one team (no-one can score any points!)
      settings = {
         mission = "deathmatch",
         book = "none",
         wand = "none",
         num_wands = 0,
         num_gems = 0,
         gems_needed = 0,
         dungeon = "basic",
         premapped = false,
         entry = "different",
         exit = "none",
         gear = "both",
         num_keys = 3,
         pretrapped = true,
         stuff = 4,
         stuff_respawn = "medium",
         zombies = 2,
         bats = 2,
         time = 10
      }
   }
}

-- This table defines the order in which the quests appear in the Quest menu
quest_order = {
   "gems",
   "big_gems",
   "shock",
   "survival",
   "dungeon_death",
   "gnomes",
   "liche",
   "wand_death",
   "paradise",
   "giants",
   "freedom",
   "deathmatch"
}

-- This is the function called by menus.lua to build the Quest menu
function make_quest_choices()
   local result = {
      { id="custom", text_key="custom" }
   }
   for i,v in ipairs(quest_order) do
      local q = quest_table[v]
      local item = {
         id = v,
         text_key = q.name_key,
         min_players = q.min_players,
         max_players = q.max_players,
         min_teams = q.min_teams,
         max_teams = q.max_teams
      }
      table.insert(result, item)
   end
   return result
end

-- This function runs when someone changes the "Quest" setting
function predefined_quest_func(S, what)
   if S.quest ~= "custom" then       
      local q = quest_table[S.quest]
      if q ~= nil then
         for k,v in pairs(q.settings) do
            S[k] = v
         end
      end
   end
end
