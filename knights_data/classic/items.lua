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


-- Time Scale
ts = 140

function zap()
   kts.FlashScreen() 
   snd_pentagram()
end

function wandzap()
   kts.FlashScreen() 
   snd_pentagram()
end

function rng_range(low, high)
   return function()
            return kts.RandomRange(low, high)
          end
end

function rng_time_range(low, high)
   return function() 
            return kts.RandomRange(low, high) * ts 
          end
end


--
-- Weapons
--

i_sword = kts.ItemType {
    type = "held",
    overlay = kts.Overlay { g_sword_north, g_sword_east, g_sword_south, g_sword_west },

    melee_backswing_time = ts*2,
    melee_downswing_time = ts*2,
    melee_damage = rng_range(1, 2),
    melee_stun_time = rng_time_range(2, 4),
    melee_tile_damage = 1,
    parry_chance = 0.86
}

i_hammer = kts.ItemType {
    type = "held",
    graphic = g_hammer,
    overlay = kts.Overlay { g_hammer_north, g_hammer_east, g_hammer_south, g_hammer_west },

    melee_backswing_time = 3*ts,
    melee_downswing_time = 4*ts,
    melee_damage = rng_range(1, 4),
    melee_stun_time = rng_time_range(4, 6),
    melee_tile_damage = 1000,

    tutorial = TUT_HAMMER
}

i_staff = kts.ItemType {
    type = "held",
    graphic = g_staff,
    overlay = kts.Overlay { g_staff_north, g_staff_east, g_staff_south, g_staff_west },
    parry_chance = 0.9,
    open_traps = true,
    tutorial = TUT_STAFF
}

i_axe = kts.ItemType {
    type = "held",
    graphic = g_axe,
    overlay = kts.Overlay { g_axe_north, g_axe_east, g_axe_south, g_axe_west },

    melee_backswing_time = 2*ts,
    melee_downswing_time = 3*ts,
    melee_damage         = rng_range(1, 3),     -- Beefed up compared to Amiga Knights (which has d2)
    melee_stun_time      = rng_time_range(2, 5),
    melee_tile_damage    = 5,      -- Slightly less effective than in Amiga Knights, I think
    parry_chance         = 0.8,

    can_throw         = true,
    missile_anim      = kts.kconfig_anim("a_axe"),
    missile_range     = 5,
    missile_speed     = 250,
    missile_hit_multiplier = 5,
    missile_access_chance = 0,
    missile_backswing_time = 2*ts,
    missile_downswing_time = 3*ts,
    missile_damage    = rng_range(0, 2),
    missile_stun_time = rng_time_range(2, 4),

    tutorial = TUT_AXE,

    control = kts.Control {
       action = kts.ThrowOrShoot,
       menu_icon = g_menu_axe,
       continuous = true,
       action_bar_slot = 4,
       name = "Throw Axe",
       menu_special = 4 -- Does not appear on Action Menu
    }
}

i_dagger = kts.ItemType {
    type = "backpack",
    graphic = g_dagger,
    stack_graphic = g_daggers,
    backpack_graphic = g_inv_dagger,
    backpack_overdraw = g_inv_overdraw_small,
    backpack_slot = 13,
    overlay = kts.Overlay { g_dagger_north, g_dagger_east, g_dagger_south, g_dagger_west },

    max_stack = 10,

    can_throw         = true,
    missile_anim      = kts.kconfig_anim("a_dagger"),
    missile_range     = 10,
    missile_speed     = 350,
    missile_hit_multiplier = 1,
    missile_access_chance = 0.5,
    missile_backswing_time = ts,
    missile_downswing_time = ts,
    missile_damage    = 1,
    missile_stun_time = rng_time_range(2, 3),

    control = kts.Control {
        action = kts.Throw,
        menu_icon = g_menu_dagger,
        menu_direction = "right",
        menu_special = 1,
        continuous = true,
        action_bar_slot = 5,
        name = "Throw Daggers"
    },

    tutorial = TUT_DAGGERS
}

-- used by the crossbow
i_bolts = kts.ItemType {
    type = "backpack",
    graphic = g_bolts,
    backpack_graphic = g_inv_bolt,
    backpack_overdraw = g_inv_overdraw_small,
    backpack_slot = 14,
    max_stack = 10,

    missile_anim = kts.kconfig_anim("a_bolt"),
    missile_range = 50,
    missile_speed = 440,
    missile_hit_multiplier = 1,
    missile_access_chance = 0.5,
    missile_damage = rng_range(1, 4),
    missile_stun_time = rng_time_range(3, 6),

    tutorial = TUT_BOLTS
}

i_crossbow = kts.ItemType {
    type = "held",
    graphic = g_crossbow,

    ammo = i_bolts,
    reload_time = ts*26,
    reload_action      = snd_click,
    reload_action_time = ts,

    tutorial = TUT_CROSSBOW,

    control = kts.Control {
       action = kts.ThrowOrShoot,
       menu_icon = g_menu_crossbow,
       continuous = true,
       action_bar_slot = 4,
       name = "Fire Crossbow",
       menu_special = 4   -- Does not appear on Action Menu
    }
}


-- used by traps etc
i_bolt_trap = i_bolts

--
-- Wands
--

basic_wand = {
    type = "held",
    graphic = g_wand,
    overlay = kts.Overlay { g_wand_north, g_wand_east, g_wand_south, g_wand_west },

    melee_backswing_time = ts,
    melee_downswing_time = ts,
    melee_tile_damage = 0,
    melee_action = wandzap,
    on_pick_up = kts.UpdateQuestStatus,
    on_drop = kts.UpdateQuestStatus,

    name = "A wand"
}

i_wand_of_destruction = kts.ItemType(basic_wand & {
    melee_damage = 1000,
    melee_stun_time = rng_time_range(2, 3),
    melee_tile_damage = 1000
})

i_wand_of_undeath = kts.ItemType(basic_wand & {
    melee_damage = 0,
    melee_stun_time = ts,
    melee_action = function()
       wandzap()
       local zombified = kts.ZombifyTarget(m_zombie)
       local killed = kts.ZombieKill(m_zombie)
       if zombified or killed then
          snd_zombie()
       end
    end
})

i_wand_of_open_ways = kts.ItemType(basic_wand & {
    melee_damage = rng_range(1, 3),
    melee_stun_time = rng_time_range(2, 5),
    melee_action = function()
       kts.OpenWays()
       wandzap()
    end,
    allow_strength = false  -- Prevent the usual destruction of doors/chests when knight has strength
})

i_wand_of_securing = kts.ItemType(basic_wand & {
    melee_damage = function()
       if kts.RandomChance(0.25) then return 0 else return 1 end
    end,
    melee_stun_time = ts,
    melee_action = function()
       local success = kts.Secure(t_wall_normal) 
       if success then wandzap() end
    end
})

all_wands = { i_wand_of_destruction, i_wand_of_undeath, i_wand_of_open_ways, i_wand_of_securing }


--
-- Books
--

basic_book = { 
    -- Tome of Gnomes or Lost Book of Ashur
    type = "held",
    graphic = g_book,
    overlay = kts.Overlay { g_book_north, g_book_east, g_book_south, g_book_west },
    editor_label = "G",
    on_pick_up = kts.UpdateQuestStatus,
    on_drop = kts.UpdateQuestStatus,
    name = "The book"
}

i_basic_book = kts.ItemType(basic_book)

i_book_of_knowledge = kts.ItemType(basic_book & {
    -- Ancient Book of Knowledge
    on_pick_up = function()
        kts.MagicMapping()
        kts.RevealStart()
        kts.UpdateQuestStatus()
    end,
    on_drop   =  function()
        kts.RevealStop()
        kts.UpdateQuestStatus()
    end,
    editor_label = "K"
})

i_necronomicon = kts.ItemType(basic_book & { 
    -- Necronomicon
    -- Note on kts.Necromancy: first number is the number of zombies to generate (I have increased
    -- this slightly compared to the original Knights), and second number is the range (in squares). 
    on_pick_up = function()
        kts.Necromancy(10, 10)    -- Attempt to raise nearby zombies. (Will only work the first time.)
        kts.OnSuccess(wandzap)    -- Flash screen if necromancy actually took place.
        kts.FullZombieActivity()  -- Always set full zombie activity while necronomicon is held.
        kts.UpdateQuestStatus()
    end,
    on_drop = function()
        kts.NormalZombieActivity()
        kts.UpdateQuestStatus()
    end,
    editor_label = "N"
})

all_books = { i_basic_book, i_book_of_knowledge, i_necronomicon }


--
-- Traps
--

i_bear_trap = kts.ItemType {
    type = "held",
    graphic = g_closed_bear_trap,
    overlay = kts.Overlay { g_beartrap_north, g_beartrap_east, g_beartrap_south, g_beartrap_west },
    control = kts.Control { 
        action = function() 
           kts.SetBearTrap(i_bear_trap_open)
           snd_click()
        end,
        menu_icon = g_menu_beartrap,
        menu_direction = "left",
        action_bar_slot = 8,
        name = "Set Bear Trap"
    },
    tutorial = TUT_BEAR_TRAP
}

i_bear_trap_open = kts.ItemType {
    type = "nopickup",
    graphic = g_open_bear_trap,
    on_walk_over = function()
       local stun_time = ts * kts.RandomInt(15, 20)
       kts.Damage(1, stun_time, 1)
       kts.ChangeItem(i_bear_trap)
       snd_bear_trap()
    end,
    on_hit = function()
       kts.ChangeItem(i_bear_trap)
       snd_bear_trap()
    end
}

i_poison_trap = kts.ItemType {
    type = "backpack",
    graphic = g_poison_trap,
    backpack_graphic = g_menu_poison_trap,
    backpack_overdraw = g_inv_overdraw_big,
    backpack_slot = 11,
    control = kts.Control {
       action = function()
          kts.SetPoisonTrap()
          snd_click()
       end,
       menu_icon = g_menu_poison_trap,
       menu_direction = "left",
       action_bar_slot = 8,
       name = "Set Poison Needle Trap",
    },
    tutorial = TUT_POISON_TRAP
}

i_blade_trap = kts.ItemType {
    type = "backpack",
    graphic = g_blade_trap,
    backpack_graphic = g_menu_blade_trap,
    backpack_overdraw = g_inv_overdraw_big,
    backpack_slot = 12,
    control = kts.Control {
       action = function()
          kts.SetBladeTrap(i_bolt_trap)
          snd_click()
       end,
       menu_icon = g_menu_blade_trap,
       menu_direction = "right",
       action_bar_slot = 9,
       name = "Set Spring Blade Trap"
    },
    tutorial = TUT_BLADE_TRAP
}


--
-- Keys
--

basic_key = {
    type = "backpack",
    graphic = g_key,
    tutorial = TUT_KEY
}

i_key1 = kts.ItemType (basic_key & {
    backpack_graphic = g_inv_key1,
    backpack_slot = 20,
    key = 1
})

i_key2 = kts.ItemType(basic_key & {
    backpack_graphic = g_inv_key2,
    backpack_slot = 21,
    key = 2
})

i_key3 = kts.ItemType(basic_key & {
    backpack_graphic = g_inv_key3,
    backpack_slot = 22,
    key = 3
})

i_lockpicks = kts.ItemType(basic_key & {
    backpack_graphic = g_inv_lockpicks,
    backpack_slot = 23,
    key = -1,
    control = kts.Control {
       action = function()
          kts.PickLock(2, 140)
          snd_lock()
       end,
       continuous = true,
       menu_icon = g_menu_lockpicks,
       menu_direction = "up",
       action_bar_slot = 8,
       name = "Use Lock Picks"
    },
    tutorial = TUT_LOCKPICKS
})


--
-- Gems
--

i_gem = kts.ItemType {
    type = "backpack",
    graphic = g_gem,
    backpack_graphic = g_menu_drop_gem,
    backpack_slot = 30,
    on_pick_up = kts.UpdateQuestStatus,
    on_drop = kts.UpdateQuestStatus,
    tutorial = TUT_GEM,
    name = "A gem"
}


--
-- Magic Items
--

-- durations for potions and other magics
function pot_dur()
   return 1000 * kts.RandomRange(11,110)           -- 11s - 1m50s, av 1m
end

function pi_dur()
   return 1000 * kts.RandomRange(21,160)           -- 21s - 2m40s, av 1m30s
end

function invuln_dur()
   -- 6s - 1m, av 23s
   local choice = kts.RandomRange(1,4)
   if choice == 1 then return 1000 * kts.RandomRange(6, 15)
   elseif choice == 2 then return 1000 * kts.RandomRange(9, 18)
   elseif choice == 3 then return 1000 * kts.RandomRange(16, 30)
   else return 1000 * kts.RandomRange(31, 60)
   end
end

function para_dur()
   -- 1s  - 30s, av 10.5s
   local choice = kts.RandomRange(1,3)
   if choice == 1 then return 1000 * kts.RandomRange(1, 10)
   elseif choice == 2 then return 1000 * kts.RandomRange(1, 20)
   else return 1000 * kts.RandomRange(1, 30)
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
   kts.Teleport()
   snd_teleport()
end

function my_attractor()
   snd_teleport()
   kts.Attractor()
   snd_teleport()
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
   kts.Regeneration(pot_dur(), "Regeneration")
end

function strength()
   kts.Strength(pot_dur(), "Strength")
end

function super()
   kts.Super(pot_dur(), "Super")
end

function sense_items()
   kts.SenseItems(see_item_dur())
   kts.FlashMessage("Sense Items")
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


-- Now we can define the actual potion and scroll items.

i_potion = kts.ItemType {
    type       = "magic",
    graphic    = g_potion,
    fragile    = true,    -- Smashing a potion-containing chest destroys the potion.
    on_pick_up = function()
       snd_drink()
       kts.Delay(750)
       potion_effect()
    end,
    tutorial = TUT_POTION
}

i_scroll = kts.ItemType {
    type       = "magic",
    graphic    = g_scroll,
    on_pick_up = function()
       zap()
       scroll_effect()
    end,
    tutorial = TUT_SCROLL
}
