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


-- SOUND FILES

s_click = kts.Sound("+click.wav")
s_door = kts.Sound("+door.wav")
s_drink = kts.Sound("+drink.wav")
s_parry = kts.Sound("+parry.wav")
s_screech = kts.Sound("+screech.wav")
s_squelch = kts.Sound("+squelch.wav")
s_ugh = kts.Sound("+ugh.wav")
s_zombie2 = kts.Sound("+zombie2.wav")
s_zombie3 = kts.Sound("+zombie3.wav")


-- FUNCTIONS TO PLAY VARIOUS SOUNDS

function snd_bat_screech() 
   kts.PlaySound(cxt.pos, s_screech, 15000)   -- Sound made by bats when taking damage
end

function snd_bear_trap()
   kts.PlaySound(cxt.pos, s_parry, 4000, 1)   -- Stepping on a bear trap (can be heard by all players)
end

function snd_click()
   kts.PlaySound(cxt.pos, s_click, 20000)     -- Generic sound for various actions. (pick up, drop, etc.)
end

function snd_crossbow()
   kts.PlaySound(cxt.pos, s_door, 35000)      -- Crossbow fired
end

function snd_daggerfall()
   kts.PlaySound(cxt.pos, s_parry, 40000)     -- Missile hits wall and falls to floor.
end

function snd_door()
   kts.PlaySound(cxt.pos, s_door, 20000)      -- Opening or closing a door, chest or switch
end

function snd_downswing()
   kts.PlaySound(cxt.pos, s_ugh, 14000)       -- Swoosh of weapon downswing; also dagger throwing.
end

function snd_drink()
   kts.PlaySound(cxt.pos, s_drink, 10000)     -- Drinking a potion
end

function snd_lock()
   kts.PlaySound(cxt.pos, s_parry, 40000)     -- "Chink" sound of locked doors / lock picks.
end

function snd_parry()
   kts.PlaySound(cxt.pos, s_parry, 10000)     -- Parrying
end

function snd_pentagram()
   kts.PlaySound(cxt.pos, s_door, 30000)      -- Stepping on a pentagram, reading a scroll, or zapping a wand
end

function snd_squelch()
   kts.PlaySound(cxt.pos, s_squelch, 15000)   -- Knight takes weapon damage, or monster killed by weapon
end

function snd_teleport()
   kts.PlaySound(cxt.pos, s_squelch, 4000)    -- Teleportation
end

function snd_tile_bash()
   kts.PlaySound(cxt.pos, s_click, 2000)      -- Tile was hit
end

function snd_tile_clunk()
   kts.PlaySound(cxt.pos, s_click, 1000)      -- "Heavy" tile was hit
end

function snd_tile_destroy()
   kts.PlaySound(cxt.pos, s_door, 6000)       -- Wooden tile destroyed
end

function snd_ugh()
   kts.PlaySound(cxt.pos, s_ugh, 9000)        -- Knight takes damage, is poisoned, or suicides
end

function snd_zombie()
   -- Sounds made by zombies
   local choice = kts.RandomRange(1, 3)
   if choice == 1 then
      kts.PlaySound(cxt.pos, s_ugh,     kts.RandomRange(4000, 6000))
   elseif choice == 2 then
      kts.PlaySound(cxt.pos, s_zombie2, kts.RandomRange(8000, 11000))
   else
      kts.PlaySound(cxt.pos, s_zombie3, kts.RandomRange(8000, 11000))
   end
end

function snd_dart()
   -- used for skull traps.
   snd_click()
   snd_crossbow()
end


-- FUNCTIONS TO PLAY A SOUND AT A GIVEN POSITION

function click_sound(pos)     kts.play_sound(pos, s_click, 20000)  end
function crossbow_sound(pos)  kts.play_sound(pos, s_door, 35000)   end
function door_sound(pos)      kts.play_sound(pos, s_door, 20000)   end
function pentagram_sound(pos) kts.play_sound(pos, s_door, 30000)   end
function teleport_sound(pos)  kts.play_sound(pos, s_squelch, 4000) end


-- HOOKS

-- (a) Knight swings weapon. (b) Knight parries a weapon blow.
kts.HOOK_WEAPON_DOWNSWING = snd_downswing
kts.HOOK_WEAPON_PARRY = snd_parry

-- (a) Knight takes damage, suicides or is poisoned. 
-- (b) Any creature takes damage >0.
kts.HOOK_KNIGHT_DAMAGE = snd_ugh
kts.HOOK_CREATURE_SQUELCH = snd_squelch

-- Crossbow bolt was fired.
kts.HOOK_SHOOT = snd_crossbow

-- Dagger or crossbow bolt hits a wall (or other obstacle) and drops to floor.
kts.HOOK_MISSILE_MISS = snd_daggerfall


