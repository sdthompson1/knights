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


-- General config settings
MISC_CONFIG = {

-- UDP port (used by Host LAN Game)
port_number = 16399;


--
-- Game config settings
--

-- Maximum number of chat messages to store
max_chat_lines = 300;

-- Animation settings
bat_anim_timescale = 65;

-- Control settings
menu_delay = 500;            -- delay between pressing fire and menu appearing

-- Graphics settings
death_draw_map_time = 1500; -- how long to show the map after death

-- Sound settings 
sound_volume = 35;          -- 0 to 100. NB too high may cause 'crackling'

-- Main game FPS (maximum)
fps = 120;

-- Font
font_size = 15;       -- Font size for title screens, menus etc
font_antialias = 1;
dpy_font_base_size = 600;   -- Controls the in-game font size
dpy_font_min_size = 14;   -- Smallest allowed font size in game

-- Display layout
dpy_viewport_width = 160;
dpy_viewport_height = 273;
dpy_gutter = 10;
dpy_pixels_per_square = 16;

dpy_dungeon_width = 160;    -- (10 squares)
dpy_dungeon_height = 176;   -- (11 squares)

game_over_r = 180;
game_over_g = 60;
game_over_b = 60;
game_over_msg = "CLICK MOUSE TO CONTINUE";
game_over_fade_time = 300;
game_over_black_time = 300;

dpy_action_bar_left = 0;
dpy_action_bar_top = 1;

-- Note: dpy_inventory_width / dpy_inventory_slots
-- should equal the width of each inventory graphic...
dpy_inventory_left = 20;
dpy_inventory_top = 33;
dpy_inventory_width = 128;
dpy_inventory_height = 64;
dpy_inventory_gem_height = 16;
dpy_inventory_slots = 8;
dpy_inventory_spacing = 2;

dpy_map_top = 33;
dpy_map_left = 104;
dpy_map_width = 48;       -- Large enough for the menu (48x48) as well as the map.
dpy_map_height = 48;
mini_map_flash_time = 140;

dpy_skulls_left = 0;
dpy_skulls_top = 36;
dpy_potion_left = 18;     -- skulls require 16 width.
dpy_potion_top  = 78;
dpy_potion_top_with_time = 70;
dpy_time_x = 25;
dpy_time_y = 59;

dpy_action_slot_size = 16;

message_on_time = 400;
message_off_time = 200;
msg_alpha = 160;    -- alpha of the black background for in-game msgs.
screen_flash_time = 200;
screen_flash_r = 170;
screen_flash_g = 170;
screen_flash_b = 119;
pot_flash_time = 160;      -- for super, poison immun. (controls duration of each flash)
pi_flash_delay = 6;        -- for poison immun. (controls how frequent flashes are)

-- Pause mode colours
pause1r = 200;
pause1g = 80;
pause1b = 80;

pause2r = 160;
pause2g = 40;
pause2b = 40;

pause3r = 170;
pause3g = 170;
pause3b = 119;

pause4r = 136;
pause4g = 136;
pause4b = 85;

pausealpha = 220;

invisalpha = 140;       -- 0 to 255

}

-- Menus
MENU_CENTRE    = Graphic("menu_centre.bmp")
MENU_EMPTY     = Graphic("menu_empty.bmp")
MENU_HIGHLIGHT = Graphic("menu_highlight.bmp",0,0,0)


-- Winner/Loser images
WINNER_IMAGE = Graphic("winner.bmp", 0,0,0, 0,0)
LOSER_IMAGE  = Graphic("loser.bmp",  0,0,0, 0,0)

-- Skulls and Potion Bottle
g_skull1        = Graphic("skull1.bmp",0,0,0,-1,-9)
g_skull2        = Graphic("skull2.bmp",0,0,0, 0,-7)
g_skull3        = Graphic("skull3.bmp",0,0,0, 0,-3)
g_skull4        = Graphic("skull4.bmp",0,0,0, 0, 0)
g_health0       = Graphic("health0.bmp",0,0,0)
g_health1       = Graphic("health1.bmp",0,0,0)
g_health2       = Graphic("health2.bmp",0,0,0)
g_health3       = Graphic("health3.bmp",0,0,0)
g_health4       = Graphic("health4.bmp",0,0,0)

POTION_SETUP = {
    graphics = {g_health0, g_health1, g_health2, g_health3, g_health4};
    colours = {
        {0x88, 0, 0};    -- No potion
        {0, 0, 0};       -- Invisibility
        {0, 0, 0x77};    -- Strength
        {0x44, 0, 0x66}; -- Quickness
        {0xEE, 0x44, 0}; -- Regeneration
        {0, 0x44, 0};    -- Paralyzation
    }
}

SKULL_SETUP = {
    graphics = {g_skull1, g_skull2, g_skull3, g_skull4};
    -- rows, columns give the pixel coordinates at which to draw the skulls.
    rows = {45, 30, 15, 0};
    columns = {0, 8, 4};
}


-- Maximum number of chat lines that will be remembered
max_chat_lines = 200

-- Speech bubble image
SPEECH_BUBBLE = Graphic("speech_bubble.bmp", 0,255,0, -7,5)
