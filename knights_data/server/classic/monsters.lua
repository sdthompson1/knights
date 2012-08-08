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

zombie_weapon = kts.ItemType {
   -- Like an axe, but it takes slightly longer to do the backswing; also there is a slightly
   -- lower tile damage. (don't want zombies smashing up the dungeon too quickly.)
   type = "held",
   melee_backswing_time = 3*ts,
   melee_downswing_time = 3*ts,
   melee_damage         = rng_range(1,3),
   melee_stun_time      = rng_time_range(2,5),
   melee_tile_damage    = 3
}


m_vampire_bat = kts.MonsterType {
   type = "flying",
   
   -- general properties:
   health = rng_range(1,2),     -- bats are not very tough
   speed = 86,                  -- slightly slower than a knight
   anim = a_vbat,
   corpse_tiles = { t_dead_vbat_1, t_dead_vbat_2, t_dead_vbat_3 },
   sound = snd_bat_screech,

   -- properties specific to flying monsters:
   attack_damage = 1,
   attack_stun_time = rng_time_range(2, 3),
}

m_zombie = kts.MonsterType {
   type = "walking",

   -- general properties:
   health = rng_range(2, 6),    -- zombies are tough (need 3 sword hits on average)
   speed = 67,                  -- zombies are slow
   anim = a_zombie,
   corpse_tiles = { t_dead_zombie },
   sound = snd_zombie,

   -- properties specific to walking monsters:
   weapon = zombie_weapon,

   -- list of tiles that zombies don't want to walk onto:
   ai_avoid = all_open_pit_tiles,

   -- list of items that zombies will whack with sword instead of walking onto:
   ai_hit = {i_bear_trap_open},

   -- list of items that zombies will run away from (if a knight is carrying one of them):
   ai_fear = {i_wand_of_undeath}
}


-- Zombie Activity

zombie_activity_table = { 

   -- tiles that decay into other tiles (e.g. dead knights turn into dead zombie tiles):

   { t_dead_knight_1, t_dead_zombie },
   { t_dead_knight_2, t_dead_zombie },
   { t_dead_knight_3, t_dead_zombie },
   { t_dead_knight_4, t_dead_zombie },
   { t_floor6,        t_dead_zombie },

   -- tiles that reanimate into monsters (e.g. dead zombie turns into an actual zombie):

   { t_dead_zombie,   m_zombie }

}

