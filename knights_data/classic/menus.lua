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


-- This function displays "0" as "None" in some menu fields (e.g. Number of Gems)
function show_zero_as_none(n)
   if n == 0 then
      return "None"
   else
      return n
   end
end

kts.MENU = {

   text = "QUEST SELECTION",

   initialize_func = function(S)
      -- Setup a quest for gems to begin with.
      S.quest = "gems"
      predefined_quest_func(S, "quest")  -- Loads the "gems" quest
   end,

   prepare_game_func   = prepare_game,     -- defined in dungeon_setup.lua
   start_game_func     = start_game,       -- defined in dungeon_setup.lua
   describe_quest_func = describe_quest,   -- defined in quest_description.lua


   on_select = function(S, what)
      -- Change Quest to Custom if anything is changed 
      -- (except the time limit, and Quest itself).
      if what ~= "quest" and what ~= "time" then
         S.quest = "custom"
      end
   end,

   items = {
      {
         id = "quest",
         text = "Quest",
         on_select = predefined_quest_func,   -- defined in preset_quests.lua
         choices = make_quest_choices(),      -- defined in preset_quests.lua
         randomize = function(S, num_players, num_teams)
            -- Quest should always be Custom for a random quest.
            return "custom"
         end
      },

      "spacer",

      {
         id = "mission",
         text = "Mission Type",
         choices = {
            {
               id = "escape",
               text = "Escape from the Dungeon",
               constrain = function(S)
                  S.IsNot("exit", "none")
                  S.Is("book", "none")
               end
            },
            {
               id = "retrieve_book",
               text = "Retrieve Book and Escape",
               constrain = function(S)
                  S.IsNot("exit", "none")   -- must have exit
                  S.IsNot("book", "none")   -- must have book
               end,
               features = function(S)
                  quest_retrieve(all_books, 1, "Book Required")
                  hint("Retrieve the book", 2, 1)
               end
            },
            {
               id = "retrieve_wand",
               text = "Retrieve Wand and Escape",
               constrain = function(S)
                  S.IsNot("exit", "none")
                  S.Is("book", "none")
                  S.IsAtLeast("num_wands", 1)
               end,
               features = function(S)
                  quest_retrieve(all_wands, 1, "Wand Required")
                  hint("Retrieve the wand", 2, 1)
               end
            },
            {
               id = "destroy_book",
               text = "Destroy Book with Wand",
               constrain = function(S)
                  S.Is("exit", "none")
                  S.IsNot("book", "none")
                  S.IsAtLeast("num_wands", 1)
               end,
               features = function(S)
                  quest_destroy(all_books, 
                                all_wands, "Wand Required",
                                all_special_pentagrams, "Not in Special Pentagram")
                  hint("Place the book in the special pentagram", 3, 1)
                  hint("Strike the book with the wand", 4, 1)
                  add_segment(special_pentagrams)
               end
            },
            {
               id = "duel_to_death",
               text = "Duel to the Death",
               min_teams = 2,
               constrain = function(S)
                  S.Is("exit", "none")
                  S.Is("book", "none")
                  S.IsAtLeast("num_wands", 1)
                  S.Is("wand", "securing")
                  S.Is("num_gems", 0)
               end
            },
            {
               id = "deathmatch",
               text = "Deathmatch",
               min_teams = 2,
               constrain = function(S)
                  S.Is("exit", "none")
                  S.Is("book", "none")
                  S.Is("num_gems", 0)
               end,
               features = function(S)
                  quest_deathmatch()
                  hint("Score points by killing enemy knights", 1, 1)
               end
            }
         }
      },
      {
         id = "book",
         text = "Type of Book",
         choices = {
            {
               id = "none",
               text = "No Book in the Dungeon"
            },
            {
               id = "knowledge",
               text = "Ancient Book of Knowledge",
               features = function(S)
                  add_item(i_book_of_knowledge, 1, item_weights)
               end
            },
            {
               id = "ashur",
               text = "Lost Book of Ashur",
               features = function(S)
                  add_item(i_basic_book, 1, item_weights)
               end
            },
            {
               id = "necronomicon",
               text = "Necronomicon",
               features = function(S)
                  add_segment(liche_tombs)
               end
            },
            {
               id = "gnomes",
               text = "Tome of Gnomes",
               features = function(S)
                  add_segment(gnome_rooms)
               end
            }
         }
      },
      {
         id = "wand",
         text = "Type of Wand",
         choices = {
            {
               id = "none",
               text = "No Wands in the Dungeon"
            },
            {
               id = "destruction",
               text = "Wand of Destruction",
               features = function(S)
                  add_item(i_wand_of_destruction, S.num_wands, item_weights)
               end
            },
            {
               id = "open_ways",
               text = "Wand of Open Ways",
               features = function(S)
                  add_item(i_wand_of_open_ways, S.num_wands, item_weights)
               end
            },
            {
               id = "securing",
               text = "Wand of Securing",
               features = function(S)
                  add_item(i_wand_of_securing, S.num_wands, item_weights)
                  hint("Secure all entry points using the Wand of Securing", 1, 2)
                  hint("Destroy all enemy knights", 2, 2)
               end
            },
            {
               id = "undeath",
               text = "Wand of Undeath",
               features = function(S)
                  add_item(i_wand_of_undeath, S.num_wands, item_weights)
               end
            }
         }
      },
      {
         id = "num_wands",
         text = "Number of Wands",
         choice_min = 0,
         choice_max = 8,
         show = show_zero_as_none,
         constrain = function(S)
            if S.num_wands == 0 then
               S.Is("wand", "none")
            else
               S.IsNot("wand", "none")
            end
         end,
         randomize = function(S, num_players, num_teams)
            -- In a random quest, limit the number of wands to at most num_players+2.
            return kts.RandomRange(0, num_players + 2)
         end
      },
      {
         id = "num_gems",
         text = "Number of Gems",
         choice_min = 0,
         choice_max = 6,
         show = show_zero_as_none,
         constrain = function(S)
            S.IsAtMost("gems_needed", S.num_gems)
         end,
         features = function(S)
            add_item(i_gem, S.num_gems, item_weights)
         end
      },
      {
         id = "gems_needed",
         text = "Gems Needed",
         choice_min = 0,
         choice_max = 6,
         show = show_zero_as_none,
         features = function(S)
            quest_retrieve(i_gem, S.num_gems, "Gem Required", "%d Gems Required")
            if S.num_gems == 1 then
               hint("Retrieve 1 gem", 1, 1)
            else
               hint(string.format("Retrieve %d gems", S.num_gems), 1, 1)
            end
         end
      },

      "spacer",

      {
         id = "dungeon",
         text = "Dungeon Type",
         choices = {
            { id = "tiny",
              text = "Tiny",
              features = function(S) 
                 set_layout(d_tiny)
              end
            },
            { id = "small",
              text = "Small",
              features = function(S)
                 set_layout(d_small)
              end
            },
            { id = "basic",
              text = "Basic",
              features = function(S)
                 set_layout(d_basic)
              end
            },
            { id = "big",
              text = "Big",
              features = function(S)
                 set_layout(d_big)
              end
            },
            { id = "huge",
              text = "Huge",
              features = function(S)
                 set_layout(d_huge)
              end
            },
            { id = "snake",
              text = "Snake",
              features = function(S)
                 set_layout(d_snake)
              end
            },
            { id = "long_snake",
              text = "Long Snake",
              features = function(S)
                 set_layout(d_long_snake)
              end
            },
            { id = "ring",
              text = "Ring",
              features = function(S)
                 set_layout(d_ring)
              end
            }
         }
      },
      {
         id = "premapped",
         text = "Premapped",
         choices = {
            {
               id = false,
               text = "No"
            },
            {
               id = true,  
               text = "Yes",
               features = function(S)
                  set_premapped()
               end
            }
         }
      },
      {
         id = "entry",
         text = "Entry Point",
         choices = {
            { 
               id = "random",
               text = "Total Random",
               features = function(S)
                  set_entry("random")
               end
            },
            {
               id = "close",
               text = "Close to Other",
               features = function(S)
                  set_entry("close")
               end,
               max_players = 4    -- due to a dungeon generator limitation (#14)
            },
            {
               id = "away",
               text = "Away from Other",
               features = function(S)
                  set_entry("away")
               end,
               max_players = 4    -- due to a dungeon generator limitation (#15)
            },
            {
               id = "different",
               text = "Different every time",
               features = function(S)
                  set_entry("different")
               end
            }
         },
      },
      {
         id = "exit",
         text = "Exit Point",
         choices = {
            {
               id = "none",
               text = "No Escape",
               features = function(S)
                  set_exit("none")
               end
            },
            {
               id = "same",
               text = "Same as Entry",
               features = function(S)
                  set_exit("self")
                  hint("Escape via your entry point", 5, 1)
               end
            },
            {
               id = "other",
               text = "Other's Entry",
               min_players = 2,
               max_players = 2,
               features = function(S)
                  set_exit("other")
                  hint("Escape via your opponent's entry point", 5, 1)
               end
            },
            {
               id = "random",
               text = "Total Random",
               features = function(S)
                  set_exit("random")
                  hint("Escape via the unknown exit point", 5, 1)
               end
            },
            {
               id = "guarded",
               text = "Guarded",
               features = function(S)
                  add_segment(guarded_exits)
                  set_exit("special")
                  hint("Escape via the guarded exit", 5, 1)
               end
            }
         }
      },
      {
         id = "gear",
         text = "Starting Gear",
         choices = {
            {
               id = "none",
               text = "Sword Only"
            },
            {
               id = "daggers",
               text = "Daggers",
               features = function(S)
                  add_gear(starting_daggers)
               end
            },
            {
               id = "traps",
               text = "Traps",
               features = function(S)
                  add_gear(starting_poison_traps)
                  add_gear(starting_blade_traps)
               end
            },
            { 
               id = "both",
               text = "Daggers and Traps",
               features = function(S)
                  add_gear(starting_daggers)
                  add_gear(starting_poison_traps)
                  add_gear(starting_blade_traps)
               end
            }
         }
      },
      {
         id = "num_keys",
         text = "Number of Keys",
         choice_min = 1,
         choice_max = 3,
         features = function(S)
            set_keys( S.num_keys, 
                      i_lockpicks, 
                      { i_key1, i_key2, i_key3 },
                      item_weights )
         end
      },
      {
         id = "pretrapped",
         text = "Pretrapped Chests",
         choices = {
            { 
               id = false, 
               text = "No"
            },
            { id = true,  
              text = "Yes", 
              features = function(S) 
                 set_pretrapped()
              end
            }
         }
      },
      {
         id = "stuff",
         text = "Amount of Stuff",
         choice_min = 1,
         choice_max = 5,
         features = function(S)
            add_stuff("chest",       0.8,                ig_pot)
            add_stuff("small_table", 0.75,               ig_small_table)
            add_stuff("table",       0.67,               ig_table)
            add_stuff("barrel",      1/(8  -   S.stuff), ig_trap)
            add_stuff("floor",       1/(18 - 3*S.stuff), ig_big)
         end
      },
      {
         id = "stuff_respawn",
         text = "Stuff Respawning",
         choices = {
            { 
               id = "none", 
               text = "None"
            },
            { 
               id = "slow", 
               text = "Slow", 
               features = function(S)
                  set_stuff_respawning(respawn_items_list, 960 * 1000)
               end
            },
            { 
               id = "medium", 
               text = "Medium", 
               features = function(S)
                  set_stuff_respawning(respawn_items_list, 480 * 1000)
               end
            },
            { 
               id = "fast", 
               text = "Fast", 
               features = function(S)
                  set_stuff_respawning(respawn_items_list, 240 * 1000)
               end
            }
         },
      },

      "spacer",

      {
         id = "zombies",
         text = "Zombie Activity",
         choice_min = 0,
         choice_max = 5,
         features = function(S)
            set_zombie_activity(0.04 * S.zombies * S.zombies,   -- quadratic from 0 to 100%
                                zombie_activity_table)
         end
      },
      {
         id = "bats",
         text = "Vampire Bats",
         choice_min = 0,
         choice_max = 5,
         features = function(S)
            -- Vampire bats are generated randomly at pit tiles
            add_monster_generator(m_vampire_bat, 
                                  all_open_pit_tiles, 
                                  0.2 * S.bats)

            -- Add a few bats in the dungeon initially
            local num_bats = 0
            if S.bats > 0 then
               num_bats = 2 * S.bats + 1
            end
            add_initial_monsters(m_vampire_bat,
                                 num_bats)

            -- The total number of bats is limited
            add_monster_limit(m_vampire_bat, 
                              3 * S.bats)
         end
      },

      "spacer",

      {
         id = "time",
         text = "Time Limit",
         type = "numeric",
         digits = 2,
         suffix = "mins",
         features = function(S)
            set_time_limit(60 * S.time)
         end
      }
   }
}
