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

Zombie_Weapon = kts.ItemType {
   -- Like an axe, but it takes slightly longer to do the backswing; also there is a slightly
   -- lower tile damage. (don't want zombies smashing up the dungeon too quickly.)
   type = "held",
   melee_backswing_time = 3*ts,
   melee_downswing_time = 3*ts,
   melee_damage         = rng_range(1,3),
   melee_stun_time      = rng_time_range(2,5),
   melee_tile_damage    = 3
}
