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


-- Floors
function ig_big()
   local choice = kts.RandomRange(1, 140)

   if choice <= 30 then return i_axe
   elseif choice <= 55 then return i_hammer

   elseif choice <= 75 then
      local choice2 = kts.RandomRange(1,5)
      if choice2 <= 2 then return i_dagger
      elseif choice2 == 3 then return i_dagger, kts.RandomRange(2,4)
      elseif choice2 == 4 then return i_dagger, kts.RandomRange(2,5)
      else return i_dagger, kts.RandomRange(2,6)
      end

   elseif choice <= 95 then return i_bear_trap
   elseif choice <= 110 then return i_crossbow
   elseif choice <= 120 then return i_staff
   elseif choice <= 124 then return i_poison_trap
   elseif choice <= 128 then return i_blade_trap
   elseif choice <= 132 then return i_bolts, kts.RandomRange(1,4)
   elseif choice <= 136 then return i_potion
   elseif choice <= 140 then return i_scroll
   end
end

-- Chests
function ig_pot()
   local choice = kts.RandomRange(1,16)
   if choice == 1 then return i_poison_trap
   elseif choice == 2 then return i_blade_trap
   elseif choice <= 7 then return ig_trap()   -- delegate to "barrels" generator (5 chances in 16)
   elseif choice <= 12 then return i_potion
   elseif choice <= 16 then return i_scroll
   end
end

-- Barrels
function ig_trap()
   local choice = kts.RandomRange(1,25)
   if choice <= 6 then return i_poison_trap
   elseif choice <= 12 then return i_blade_trap
   elseif choice <= 18 then return i_bolts, kts.RandomRange(1,4)
   elseif choice <= 19 then return i_potion
   elseif choice <= 25 then 
      local choice2 = kts.RandomRange(1,3)
      if choice2 == 1 then return i_dagger, kts.RandomRange(2,4)
      elseif choice2 == 2 then return i_dagger, kts.RandomRange(2,5)
      elseif choice2 == 3 then return i_dagger, kts.RandomRange(2,6)
      end
   end
end

-- (Large) Tables
function ig_table()
   local choice = kts.RandomRange(1,4)
   if choice == 1 then return ig_big()
   elseif choice <= 3 then return ig_pot()
   elseif choice == 4 then return ig_trap()
   end
end

-- Small Skulls / Small Tables
function ig_small_table()
   if kts.RandomChance(2/3) then
      return ig_pot()
   else
      return ig_table()
   end
end


-- This controls where "required items" get placed
item_weights = {
   "chest",       3,
   "small_table", 1,
   "table",       1,
   "floor",       0
}


-- Starting Gear

starting_daggers = { i_dagger, 5, 4, 3, 2 }
starting_poison_traps = { i_poison_trap, 2, 2, 2, 2 }
starting_blade_traps = { i_blade_trap, 3, 3, 3, 3 }


-- Items to be respawned by "Stuff Respawning"

respawn_items_list = { i_potion, i_scroll }
