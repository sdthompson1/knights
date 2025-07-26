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

a_purple_knight = kts.Anim {
    g_ktp1n, g_ktp1e, g_ktp1s, g_ktp1w,  -- Normal
    g_ktp2n, g_ktp2e, g_ktp2s, g_ktp2w,  -- Melee backswing
    g_ktp3n, g_ktp3e, g_ktp3s, g_ktp3w,  -- Melee downswing
    g_ktp4n, g_ktp4e, g_ktp4s, g_ktp4w,  -- Parrying
    g_ktp5n, g_ktp5e, g_ktp5s, g_ktp5w,  -- Throwing backswing
    g_ktp6n, g_ktp6e, g_ktp6s, g_ktp6w,  -- Throwing follow-through
    g_ktp7n, g_ktp7e, g_ktp7s, g_ktp7w,  -- Holding crossbow
    g_ktp8n, g_ktp8e, g_ktp8s, g_ktp8w,  -- Reloading crossbow.
}

a_vbat = kts.Anim {
   g_vbat_1, g_vbat_2, g_vbat_3, g_vbat_bite,
   bat=true
}

a_zombie = kts.Anim {
    g_zom1n, g_zom1e, g_zom1s, g_zom1w,  -- Normal
    g_zom2n, g_zom2e, g_zom2s, g_zom2w,  -- Melee backswing
    g_zom3n, g_zom3e, g_zom3s, g_zom3w,  -- Melee downswing
    g_zom4n, g_zom4e, g_zom4s, g_zom4w   -- Just Hit.
}

-- ogre (not used yet)
-- TODO: Move this into its own module (it is not part of Classic Knights)
--a_ogre = kts.Anim {
--    g_ogre_walk_1n,   g_ogre_walk_1e,   g_ogre_walk_1s,   g_ogre_walk_1w,   -- normal
--    g_ogre_strike_1n, g_ogre_strike_1e, g_ogre_strike_1s, g_ogre_strike_1w, -- backswing
--    g_ogre_strike_3n, g_ogre_strike_3e, g_ogre_strike_3s, g_ogre_strike_3w, -- downswing
--    g_ogre_stand_2n,  g_ogre_stand_2e,  g_ogre_stand_2s,  g_ogre_stand_2w   -- just hit.
--}

a_axe = kts.Anim {
    g_axe_north, g_axe_east, g_axe_south, g_axe_west
}

a_bolt = kts.Anim {
    g_bolt_vert, g_bolt_horiz, g_bolt_vert, g_bolt_horiz
}

a_dagger = kts.Anim {
    g_dagger_north, g_dagger_east, g_dagger_south, g_dagger_west
}
