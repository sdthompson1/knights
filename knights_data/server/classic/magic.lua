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


--
-- Magic Effects
--

function zap()
   kts.FlashScreen() 
   snd_pentagram()
end

function wandzap()
   kts.FlashScreen() 
   snd_pentagram()
end


-- Durations for potions and other magics.

function pot_dur()
    -- The duration of a "normal" potion (not Poison Immunity,
    -- Paralyzation or Invulnerability).

    -- This was changed in version 027 of Knights to make the random
    -- distribution closer to the original Amiga game. In particular,
    -- we can now generate very low values (as low as 1 second) which
    -- means that potions can sometimes be "duds" that last a very
    -- short amount of time.

    local choice = kts.RandomRange(1, 100)

    if choice <= 37 then
        return 1000 * kts.RandomRange(1, 90)
    elseif choice <= 77 then
        return 1000 * kts.RandomRange(1, 180)
    else
        return 1000 * kts.RandomRange(1, 360)
    end
end

function pi_dur()
    -- Duration of a poison immunity potion. This hasn't been changed
    -- since version 001 of (the remake of) Knights.

    -- I think this is reasonably close to the original Amiga game, at
    -- least in terms of the average, but I think the "spread" is
    -- lower (i.e. we exclude the very short or very long durations
    -- that the original game sometimes produced).

    return 1000 * kts.RandomRange(21, 160)
end

function invuln_dur()
    -- Duration of an invulnerability potion.

    -- Again, this was reworked in version 027 of Knights to give a
    -- distribution closer to the original Amiga game.

    local choice = kts.RandomRange(100)

    if choice <= 30 then
        return 1000 * kts.RandomRange(1, 13)
    elseif choice <= 74 then
        return 1000 * kts.RandomRange(1, 42)
    else
        return 1000 * kts.RandomRange(1, 90)
    end
end

function para_dur()
    -- Duration of a Paralyzation potion.

    -- Again, this hasn't changed since version 001, but I think it is
    -- not a million miles from the Amiga original, and I see no real
    -- reason to change it at this point.

    -- 1s  - 30s, av 10.5s
    local choice = kts.RandomRange(1,3)
    if choice == 1 then
        return 1000 * kts.RandomRange(1, 10)
    elseif choice == 2 then
        return 1000 * kts.RandomRange(1, 20)
    else
        return 1000 * kts.RandomRange(1, 30)
    end
end

function see_kt_dur()
   return 1000 * kts.RandomRange(21, 140)         -- 21s - 2m20s,  av 1m20s
end

function see_item_dur()
   return 1000 * 60 * 5           -- fixed duration of 5m
end


-- Teleportation needs to play a sound effect as well (ditto zombification)
-- N.B. We play the sound both before & after teleport, this ensures that the sound goes to both players.
-- For zombification we play the sound *before* zombifying (i.e. while the actor still exists!)
function my_teleport()
   snd_teleport()
   kts.TeleportRandom()
   snd_teleport(kts.GetPos(cxt.actor))  -- play sound at new position not old!
end

function my_attractor()
   snd_teleport()
   kts.Attractor()
   snd_teleport(kts.GetPos(cxt.actor))  -- play sound at new position not old!
end

function my_zombify()
   snd_zombie()
   kts.ZombifyActor(m_zombie)
end

function dispel_magic()
   kts.DispelMagic("Dispel Magic")
end

function healing()
   kts.Healing("Healing")
end

function invisibility()
   kts.Invisibility(pot_dur(), "Invisibility")
end

function invulnerability()
   kts.Invulnerability(invuln_dur(), "Invulnerability")
end

function paralyzation()
   kts.Paralyzation(para_dur())
end

function poison()
   kts.Poison("Poison")
end

function poison_immunity()
   kts.PoisonImmunity(pi_dur(), "Poison Immunity")
end

function quickness()
   kts.Quickness(pot_dur(), "Quickness")
end

function regeneration()
    if kts.RandomChance(0.5) then
        kts.Regeneration(pot_dur(), "Regeneration", "slow")
    else
        kts.Regeneration(pot_dur(), "Regeneration", "fast")
    end
end

function strength()
   kts.Strength(pot_dur(), "Strength")
end

function super()
   kts.Super(pot_dur(), "Super")
end

function sense_items()
   kts.SenseItems(see_item_dur())
end

function potion_effect()
   local healer = kts.RandomRange(1,5)
   local effect = kts.RandomRange(1,8)

   if effect == 1 and kts.RandomChance(0.5) then
      poison()
      return    -- Poison overrides all other effects.
   end
   
   if effect == 2 then regeneration() end
   if effect == 3 then quickness() end
   if effect == 4 then strength() end
   if effect == 5 then invisibility() end
   if effect == 6 then super() end
   if effect == 7 and healer ~= 5 then paralyzation() end
   
   if healer >= 4 then healing() end
   if healer == 5 then poison_immunity() end
   
end

function scroll_effect()
   local mapper = kts.RandomRange(1,2)
   local effect = kts.RandomRange(1,11)
   
   -- After a long time, the "Sense Items" scroll becomes more common.
   local time = kts.GameTime() / 1000   -- Time in seconds.
   if time > 560 then   -- 9m 20s
      local t = math.floor((time - 560) / 140)    -- 9m20s, t=4; 11m40s, t=5; etc
      if kts.RandomChance((t-3)/(t+1)) then
         effect = 8
      end
   end
   
   if mapper == 2 then
      if kts.RandomChance(1/3) then kts.RevealLocation(see_kt_dur()) end
      if kts.RandomChance(1/2) then kts.SenseKnight(see_kt_dur()) end
   end
   
   if effect == 1 then invulnerability() end
   if effect == 2 then my_teleport() end
   if effect == 3 then my_attractor() end
   if effect == 4 then quickness() end
   if effect == 5 then strength() end
   if effect == 6 then invisibility() end
   if effect == 7 then my_zombify() end
   if effect == 8 then sense_items() end
   if effect == 9 then kts.MagicMapping() end
   if effect == 10 and kts.RandomChance(1/3) then kts.WipeMap() end
   
   if mapper == 1 and effect >= 10 then dispel_magic() end
end

function pentagram_effect()
   local effect = kts.RandomRange(1, 4)
   if effect == 1 then my_teleport() end
   if effect == 2 then invisibility() end
   if effect == 3 then invulnerability() end
   if effect == 4 then my_zombify() end
end
