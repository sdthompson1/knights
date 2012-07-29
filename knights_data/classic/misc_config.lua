
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
