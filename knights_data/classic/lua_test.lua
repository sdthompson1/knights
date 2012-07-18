
-- "LIBRARY" FUNCTIONS:

function add_pos(pos, dx, dy)
  return { x = pos.x + dx,
           y = pos.y + dy  }
end




-- MISC SERVER CONFIG


kts.MISC_CONFIG = {

-- Game settings
respawn_delay = 2200;       -- how long to wait after death before respawning
walk_time = 420;            -- time for a knight to walk one full square (at standard speed).
walk_limit = 600;           -- how far you can walk before it becomes impossible to turn back (in 1000ths of a square)
action_delay = 125;         -- how long to pause after each action
turn_delay = 125;           -- how long to pause after turning
crossbow_delay = 250;       -- how long to pause after firing a crossbow
approach_offset = 250;      -- how far to move when approaching a chest or closed door etc (in 1000ths of a square)

healing_time = 800;         -- time between "healings" when approaching your own entry point
healing_amount = 1;         -- amount to add to health per "healing"
regen_time = 750;           -- ditto for regeneration 
regen_amount = 1;           -- ditto for regeneration
super_regen_time = 1500;    -- ditto for super
super_regen_amount = 1;     -- ditto for super
quickness_factor = 150;     -- quickness speed mulitplier in 100ths (e.g. 150 = 1.5x faster)

attack_threshold = 130;     -- threshold time for two impacts to be considered simultaneous.
melee_delay_time = 140;     -- waiting time after an attack finishes
att_mov_delay_time = 40;    -- controls timing of attack-while-moving (higher=less delay)
att_mov_anim_time = 250;    -- max time to show backswing anim while moving
parry_delay = 140;          -- How long knights are stunned after parrying.
door_closed_damage = 1;     -- Amount of damage done when a door is closed on top of a knight.
knight_hitpoints = 4;       -- Total number of hitpoints that knights have.

-- Item check task
-- This is for replacing destroyed quest items (Trac #33)
item_replacement_interval = 200;  -- How often to randomly replace destroyed items?
item_check_interval = 1000;       -- How often to check for destroyed items?

-- Item respawn task
-- This is for the "Item Respawning" setting on the quest settings menu.
item_respawn_interval = 2000;  -- How often to check for item respawns.

monster_radius = 10;      -- How far away from player can monsters be generated
monster_interval = 112;   -- How often to run the monster generator.
monster_respawn_wait = 45;   -- How many multiples of monster_interval after a zombie dies, before
                             -- it can be reanimated. (Trac #152)
monster_limit = 15;       -- Total monsters allowed in the dungeon. (15 in original Knights.)
monster_wait_time = 200;  -- How long monsters can 'pause' for
monster_wait_chance = 0.2; -- Chance of a monster 'pausing', if nobody can see it.
flying_monster_targetting_offset = 450;   -- how close do you have to be (in 1000ths of a square) to a bat so that he can bite you.
flying_monster_bite_wait = 400;           -- After attacking, bat will be unable to attack again for this long (in ms)
walking_monster_damage_delay = 1000;      -- Timing for zombie animation (when a zombie is damaged) (in ms).

-- Timing stuff
control_poll_interval = 50;    -- how frequently to poll the controller
player_task_interval = 200;    -- How often to recheck the mini-map?
missile_check_interval = 12;   -- How often to check for missile collisions.

-- Graphics settings
blood_icon_duration = 300;  -- how long to show blood for (when a knight is hit)
invuln_r = 170;             -- the colour of an invulnerable knight
invuln_g = 170;             --   (each component must be in range 0..255)
invuln_b = 170;

-- Miscellaneous game messages
paralyzation_msg = "Paralyzation";
required_msg = "Required";

-- Time penalty for using daggers in the new control system.
-- This is meant to offset the advantage of the new controls when throwing
-- daggers (because you don't have to wait half a second while opening the menu).
dagger_time_delay = 500
}



-- SOUNDS

function click_sound(pos)     kts.play_sound(pos, s_click, 20000)  end
function crossbow_sound(pos)  kts.play_sound(pos, s_door, 35000)   end
function door_sound(pos)      kts.play_sound(pos, s_door, 20000)   end
function pentagram_sound(pos) kts.play_sound(pos, s_door, 30000)   end
function teleport_sound(pos)  kts.play_sound(pos, s_squelch, 4000) end


-- USED BY KNIGHTS_ROOMS.TXT

-- fires a crossbow bolt
-- (usually attached to a tile, e.g. a switch; cxt.pos is the position of that tile.)
function shoot(x, y, direction, itemtype)

  -- account for map rotation/reflection
  local from = kts.rotate_add_pos(cxt.pos, x, y)
  local dt = kts.rotate_direction(cxt.pos, direction)

  -- add the missile
  kts.add_missile(from, dt, itemtype, false)
  click_sound(cxt.pos)
  crossbow_sound(from)
end

-- teleports a knight
function teleport_actor(x, y)

  local from = kts.get_pos(cxt.actor)
  local to = kts.rotate_add_pos(from, x, y)  -- add offset, accounting for map rotation/reflection.

  kts.teleport(cxt.actor, to)
  pentagram_sound(from)
  teleport_sound(from)
  teleport_sound(to)
end

-- implements open, close, toggle
function toggle_impl(x, y, door_function, tile_function, sound_flag)

   -- we could use cxt.pos in the following line, but I decided to explicitly
   --  use cxt.tile_pos, because 'toggle_impl' only makes sense for tiles anyway.
  local pos = kts.rotate_add_pos(cxt.tile_pos, x, y)

  door_function(pos)

  local tiles = kts.get_tiles(pos)
  for k,v in ipairs(tiles) do
    local new_tile = nil
    local utable = kts.user_table(v)
    if utable then new_tile = tile_function(utable) end
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
  local function my_tile_function(utable)
    new_tile = utable.open
    if (not new_tile) then new_tile = utable.close end
    return new_tile
  end
  toggle_impl(x, y, kts.open_or_close_door, my_tile_function, true)
end

function toggle_no_sound(x, y)
  local function my_tile_function(utable)
    new_tile = utable.open
    if (not new_tile) then new_tile = utable.close end
    return new_tile
  end
  toggle_impl(x, y, kts.open_or_close_door, my_tile_function, false)
end

function open(x, y)
  local function my_tile_function(utable)
    return utable.open
  end
  toggle_impl(x, y, kts.open_door, my_tile_function, true)
end

function close(x, y)
  local function my_tile_function(utable)
    return utable.close
  end
  toggle_impl(x, y, kts.close_door, my_tile_function, true)
end


-- TILE USER-TABLES

t_closed_pit_normal = kts.kconfig_tile("t_closed_pit_normal")
t_closed_pit_vert = kts.kconfig_tile("t_closed_pit_vert")
t_closed_pit_horiz = kts.kconfig_tile("t_closed_pit_horiz")
t_closed_pit_wooden = kts.kconfig_tile("t_closed_pit_wooden")
t_crystal_ball = kts.kconfig_tile("t_crystal_ball")
t_dead_pentagram = kts.kconfig_tile("t_dead_pentagram")
t_gate_horiz = kts.kconfig_tile("t_gate_horiz")
t_gate_vert = kts.kconfig_tile("t_gate_vert")
t_live_pentagram = kts.kconfig_tile("t_live_pentagram")
t_open_gate_horiz = kts.kconfig_tile("t_open_gate_horiz")
t_open_gate_vert = kts.kconfig_tile("t_open_gate_vert")
t_open_pit_normal = kts.kconfig_tile("t_open_pit_normal")
t_open_pit_vert = kts.kconfig_tile("t_open_pit_vert")
t_open_pit_horiz = kts.kconfig_tile("t_open_pit_horiz")
t_open_pit_wooden = kts.kconfig_tile("t_open_pit_wooden")
t_wall_pillar = kts.kconfig_tile("t_wall_pillar")

kts.user_table(t_wall_pillar).open = t_crystal_ball
kts.user_table(t_crystal_ball).close = t_wall_pillar

kts.user_table(t_open_pit_normal).close = t_closed_pit_normal
kts.user_table(t_closed_pit_normal).open = t_open_pit_normal

kts.user_table(t_open_pit_vert).close = t_closed_pit_vert
kts.user_table(t_closed_pit_vert).open = t_open_pit_vert

kts.user_table(t_open_pit_horiz).close = t_closed_pit_horiz
kts.user_table(t_closed_pit_horiz).open = t_open_pit_horiz

kts.user_table(t_open_pit_wooden).close = t_closed_pit_wooden
kts.user_table(t_closed_pit_wooden).open = t_open_pit_wooden

kts.user_table(t_dead_pentagram).open = t_live_pentagram
kts.user_table(t_live_pentagram).close = t_dead_pentagram

kts.user_table(t_gate_horiz).open = t_open_gate_horiz
kts.user_table(t_open_gate_horiz).close = t_gate_horiz

kts.user_table(t_gate_vert).open = t_open_gate_vert
kts.user_table(t_open_gate_vert).close = t_gate_vert


-- CONTROLS

door_control_low_pri = kts.kconfig_control("door_control_low_pri")
door_control = kts.kconfig_control("door_control")
chest_control_low_pri = kts.kconfig_control("chest_control_low_pri")
chest_control = kts.kconfig_control("chest_control")
