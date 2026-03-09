--
-- This file is part of Knights.
--
-- Copyright (C) Stephen Thompson, 2006 - 2026.
-- Copyright (C) Kalle Marjola, 1994.
--

kts.CONTROLS = {
    kts.Control {
       action         = function() kts.Drop(i_gem); snd_click() end,
       possible       = function() return kts.Can_Drop(i_gem) end,
       menu_icon      = g_menu_drop_gem,
       menu_direction = "down",
       action_bar_slot = 7,
       name_key       = "drop_gem"
    },
    kts.Control {
       action         = function() kts.PickUp(); snd_click() end,
       possible       = kts.Can_PickUp,
       menu_icon      = g_menu_pickup,
       menu_direction = "down",
       tap_priority   = 3,
       action_bar_slot = 3,
       name_key       = "pick_up"
    },
    kts.Control {
       action         = function() kts.DropHeld(); snd_click() end,
       possible       = kts.Can_DropHeld,
       menu_icon      = g_menu_drop,
       menu_direction = "right",
       tap_priority   = 2,
       action_bar_slot = 6,
       name_key       = "drop"
    },
    kts.Control {
        action         = kts.SwingOrDrop,
        possible       = kts.Can_SwingOrDrop,
        continuous     = true,
        action_bar_slot = 1,
        menu_icon = g_menu_fist,
        menu_direction = "left",
        name_key       = "attack",
        can_do_while_moving = true
    },
    kts.Control {
        action         = kts.Suicide,
        action_bar_slot = 0,
        menu_icon      = g_menu_suicide,
        name_key       = "suicide_mouse",
        menu_special = 4,   -- Does not appear on Action Menu
        can_do_while_moving = true,
        can_do_while_stunned = true
    }
}
