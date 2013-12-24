--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2013.
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


-- Helper functions for random numbers

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
    missile_anim      = a_axe,
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
       possible = kts.Can_ThrowOrShoot,
       menu_icon = g_menu_axe,
       continuous = true,
       action_bar_slot = 4,
       name = "Throw Axe",
       menu_special = 4, -- Does not appear on Action Menu

       can_do_while_moving = true
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
    missile_anim      = a_dagger,
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
        possible = kts.Can_Throw,
        menu_icon = g_menu_dagger,
        menu_direction = "right",
        menu_special = 1,
        continuous = true,
        action_bar_slot = 5,
        name = "Throw Daggers",

        can_do_while_moving = true
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

    missile_anim = a_bolt,
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
       possible = kts.Can_ThrowOrShoot,
       menu_icon = g_menu_crossbow,
       continuous = true,
       action_bar_slot = 4,
       name = "Fire Crossbow",
       menu_special = 4,   -- Does not appear on Action Menu
       can_do_while_moving = true
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

    critical = "A wand"
}

i_wand_of_destruction = kts.ItemType(
  basic_wand & {
    melee_damage = 1000,
    melee_stun_time = rng_time_range(2, 3),
    melee_tile_damage = 1000
  }
)

i_wand_of_undeath = kts.ItemType(
  basic_wand & {
    melee_damage = 0,
    melee_stun_time = ts,
    melee_action = function()
       wandzap()
       if kts.ZombifyTarget(m_zombie) then snd_zombie() end
       kts.ZombieKill(m_zombie)
    end
  }
)

i_wand_of_open_ways = kts.ItemType(
  basic_wand & {
    melee_damage = rng_range(1, 3),
    melee_stun_time = rng_time_range(2, 5),
    melee_action = function()
       kts.OpenWays()
       wandzap()
    end,
    allow_strength = false  -- Prevent the usual destruction of doors/chests when knight has strength
  }
)

i_wand_of_securing = kts.ItemType(
  basic_wand & {
    melee_damage = function()
       if kts.RandomChance(0.25) then return 0 else return 1 end
    end,
    melee_stun_time = ts,
    melee_action = function()
       local success = kts.Secure(t_wall_normal) 
       if success then wandzap() end
    end
  }
)

all_wands = { i_wand_of_destruction, i_wand_of_undeath, i_wand_of_open_ways, i_wand_of_securing }


--
-- Books
--

basic_book = { 
    -- Tome of Gnomes or Lost Book of Ashur
    type = "held",
    graphic = g_book,
    overlay = kts.Overlay { g_book_north, g_book_east, g_book_south, g_book_west },
    on_hit = check_destroy_quest,
    critical = "The book"
}

i_basic_book = kts.ItemType(basic_book)

i_book_of_knowledge = kts.ItemType(
  basic_book & {
    -- Ancient Book of Knowledge
    on_pick_up = function()
        kts.MagicMapping()
        kts.RevealStart()
    end,
    on_drop   =  function()
        kts.RevealStop()
    end
  }
)

i_necronomicon = kts.ItemType(
  basic_book & { 
    -- Necronomicon
    -- Note on kts.Necromancy: first number is the number of zombies to generate (I have increased
    -- this slightly compared to the original Knights), and second number is the range (in squares). 
    on_pick_up = function()
        local success = kts.Necromancy(10, 10)    -- Attempt to raise nearby zombies. (Will only work the first time.)
        if success then
           wandzap()    -- Flash screen if necromancy actually took place.
        end
        kts.FullZombieActivity()  -- Always set full zombie activity while necronomicon is held.
    end,
    on_drop = function()
        kts.NormalZombieActivity()
    end
  }
)

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
        possible = function() 
           return kts.Can_SetBearTrap(i_bear_trap_open)
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
       local stun_time = ts * kts.RandomRange(15, 20)
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
          kts.SetPoisonTrapOld()
          snd_click()
       end,
       possible = kts.Can_SetPoisonTrapOld,
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
          kts.SetBladeTrapOld(i_bolt_trap)
          snd_click()
       end,
       possible = function()
          return kts.Can_SetBladeTrapOld(i_bolt_trap)
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

i_key1 = kts.ItemType (
  basic_key & {
    backpack_graphic = g_inv_key1,
    backpack_slot = 20,
    key = 1
  }
)

i_key2 = kts.ItemType(
  basic_key & {
    backpack_graphic = g_inv_key2,
    backpack_slot = 21,
    key = 2
  }
)

i_key3 = kts.ItemType(
  basic_key & {
    backpack_graphic = g_inv_key3,
    backpack_slot = 22,
    key = 3
  }
)

i_lockpicks = kts.ItemType(
  basic_key & {
    backpack_graphic = g_inv_lockpicks,
    backpack_slot = 23,
    key = -1,
    control = kts.Control {
       action = function()
          kts.PickLock(0.02, 140)
          snd_lock()
       end,
       possible = function()
          return kts.Can_PickLock(0.02, 140)
       end,
       continuous = true,
       menu_icon = g_menu_lockpicks,
       menu_direction = "up",
       action_bar_slot = 8,
       name = "Use Lock Picks"
    },
    tutorial = TUT_LOCKPICKS
  }
)


--
-- Gems
--

i_gem = kts.ItemType {
    type = "backpack",
    graphic = g_gem,
    backpack_graphic = g_menu_drop_gem,
    backpack_slot = 30,
    tutorial = TUT_GEM,
    critical = "A gem"
}

--
-- Potion and scroll items
--

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
