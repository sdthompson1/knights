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


-- Lua functions for working with tiles.
-- Some of these are used by the dungeon room files.


-- fires a crossbow bolt
-- (usually attached to a tile, e.g. a switch; cxt.pos is the position of that tile.)
-- NOTE: this is hardwired to use i_bolt_trap
function shoot(x, y, dir_as_num)
   
   local direction = select(dir_as_num, "north", "east", "south", "west")

   -- account for map rotation/reflection
   local from = kts.rotate_add_pos(cxt.pos, x, y)
   local dt = kts.rotate_direction(cxt.pos, direction)

   -- add the missile
   kts.add_missile(from, dt, i_bolt_trap, false)
   click_sound(cxt.pos)
   crossbow_sound(from)
end

-- teleports a knight
function teleport_actor(x, y)

  local from = kts.GetPos(cxt.actor)
  local to = kts.rotate_add_pos(from, x, y)  -- add offset, accounting for map rotation/reflection.

  kts.teleport(cxt.actor, to)
  pentagram_sound(from)
  teleport_sound(from)
  teleport_sound(to)
end

-- setup a pair of open/closed tiles
-- (used by tiles.lua)
function set_open_closed(t_open, t_closed)
   t_open.close_to = t_closed
   t_closed.open_to = t_open
end

-- implements open, close, toggle
function toggle_impl(x, y, door_function, tile_function, sound_flag)

  -- get position of the tile that triggered the 'toggle',
  -- then add the xy offset
  local pos = kts.rotate_add_pos(cxt.tile_pos, x, y)

  door_function(pos)

  local tiles = kts.GetTiles(pos)
  for k,v in ipairs(tiles) do
    local new_tile = tile_function(v)
    if new_tile then
      kts.remove_tile(pos, v)
      kts.add_tile(pos, new_tile)
    end
  end

  if sound_flag then
    click_sound(cxt.tile_pos)
    door_sound(cxt.tile_pos)
    door_sound(pos)
  end
end

function toggle(x, y)
  local function my_tile_function(t)
    new_tile = t.open_to
    if not new_tile then
       new_tile = t.close_to
    end
    return new_tile
  end
  toggle_impl(x, y, kts.open_or_close_door, my_tile_function, true)
end

function toggle_no_sound(x, y)
  local function my_tile_function(t)
    new_tile = t.open_to
    if not new_tile then
       new_tile = t.close_to
    end
    return new_tile
  end
  toggle_impl(x, y, kts.open_or_close_door, my_tile_function, false)
end

function open(x, y)
  local function my_tile_function(t)
    return t.open_to
  end
  toggle_impl(x, y, kts.open_door, my_tile_function, true)
end

function close(x, y)
  local function my_tile_function(t)
    return t.close_to
  end
  toggle_impl(x, y, kts.close_door, my_tile_function, true)
end
