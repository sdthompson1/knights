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

kts.CONTROLS = {
    kts.Control {
       action         = function() kts.Drop(i_gem); snd_click() end,
       possible       = function() return kts.Can_Drop(i_gem) end,
       menu_icon      = g_menu_drop_gem,
       menu_direction = "down",
       action_bar_slot = 7,
       name           = "Drop Gem"
    },
    kts.Control {
       action         = function() kts.PickUp(); snd_click() end,
       possible       = kts.Can_PickUp,
       menu_icon      = g_menu_pickup,
       menu_direction = "down",
       tap_priority   = 3,
       action_bar_slot = 3,
       name           = "Pick Up"
    },
    kts.Control {
       action         = function() kts.DropHeld(); snd_click() end,
       possible       = kts.Can_DropHeld,
       menu_icon      = g_menu_drop,
       menu_direction = "right",
       tap_priority   = 2,
       action_bar_slot = 6,
       name           = "Drop"
    },
    kts.Control {
        action         = kts.SwingOrDrop,
        possible       = kts.Can_SwingOrDrop,
        continuous     = true,
        action_bar_slot = 1,
        menu_icon = g_menu_fist,
        menu_direction = "left",
        name           = "Attack",
        can_do_while_moving = true
    },
    kts.Control {
        action         = kts.Suicide,
        action_bar_slot = 0,
        menu_icon      = g_menu_suicide,
        name           = "Suicide (press both mouse buttons)",
        menu_special = 4,   -- Does not appear on Action Menu
        can_do_while_moving = true,
        can_do_while_stunned = true
    }
}
