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
          return {"quest_for_gems", "desc_quest_for_gems_1", "desc_quest_for_gems_2"}
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
          return {"big_quest_for_gems", "desc_big_quest_for_gems_1", "desc_big_quest_for_gems_2"}
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
          return {"shock_assault", "desc_shock_assault_1", "desc_shock_assault_2"}
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
          return {"war_of_survival", "desc_war_of_survival"}
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
          return {"dungeon_of_death", "desc_dungeon_of_death"}
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
          return {"lost_book_of_gnome", "desc_lost_book_of_gnome_1", "desc_lost_book_of_gnome_2"}
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
          return {"tomb_of_liche", "desc_tomb_of_liche"}
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
          return {"ancient_wand_of_death",
                  "desc_ancient_wand_of_death_1",
                  {"desc_ancient_wand_of_death_2", describe_exit_point(S)}}
          -- note: exit point might not be Other's Entry (there may be >2 players)
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
          return {"way_to_paradise", "desc_way_to_paradise_1", "desc_way_to_paradise_2"}
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
          return {"quest_of_giants",
                  "desc_quest_of_giants_1",
                  {"desc_quest_of_giants_2", describe_exit_point(S)}}
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
          return {"run_for_freedom", "desc_run_for_freedom_1", "desc_run_for_freedom_2"}
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
