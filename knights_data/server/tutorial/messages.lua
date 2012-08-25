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

local C = require("classic")

messages = {

   [1] = {

      {
         title = "KNIGHTS TUTORIAL",
         body = "Welcome to the Knights tutorial.\n\n"..
            "Knights is a multiplayer game in which players must explore randomly generated "..
            "dungeons and solve various quests.\n\n"..
            "This tutorial will teach you how to play.",
         popup = true
      },

      {
         title = "THE QUEST",
         body = "Each game of Knights revolves around a quest. The object of the game is to be the first player " ..
            "(or team) to complete the quest.\n\n" ..
            "Normally you would choose a quest at the start of each game. However, in this Tutorial, the "..
            "quest has already been chosen: you must find five gems " ..
            "and carry them back to your starting point.",
         popup = true
      },

      {
         title = "CONTROLS",
         body = "To move, use the %M.\n\n" ..
            "Other actions will appear as clickable icons on the \"Action Bar\" (below the main dungeon view). " ..
            "These will be explained later on.\n\n" ..
            "For now, simply follow the instructions at the top right of your screen. Good luck!",
         popup = true
      },

      {
         title = "DUNGEON ENTRY POINT",
         graphics = {C.g_home_north},
         colour_changes = {{0xff0000, 0}},  -- change red to black
         body = "These stairs lead down into the dungeon. Using the %M, move your knight boldly forward..."
      }
   },

   [2] = {
      title = "DOORS",
      graphics = {C.g_hdoor_background, C.g_door_hwc},
      body = "These are wooden doors.\n\n"..
         "To open a door, move your knight towards it, and, while still " ..
         "holding down the movement key, click the left mouse button.\n\n" ..
         "To close a door, turn to face it and left-click."
   },

   [3] = {
      title = "LOCKED DOOR",
      graphics = {C.g_hdoor_background, C.g_door_hic},
      body = "This door is locked. You don't have a key at the moment so you'll " ..
         "have to keep exploring elsewhere."
   },

   [4] = {
      title = "STORE ROOMS",
      graphics = {C.g_barrel},
      body = "Dust rises into the air as you step into these old store rooms. It looks as though "..
         "no-one has been here for a long time.\n\n"..
         "See if you can find a way through."
   },

   [5] = { 
      title = "HAMMER",
      graphics = {C.g_floor1, C.g_hammer},
      body = "You found a hammer!\n\n"..
         "Your knight starts out carrying a sword, but you can find other weapons (like this hammer). "..
         "When you take another weapon, your sword will be sheathed temporarily. When you drop the other weapon, "..
         "your knight will go back to using the sword.\n\n"..
         "To pick up the hammer, move on top of it, then left-click."
   },

   [6] = {
      title = "HAMMER",
      graphics = {C.g_floor1, C.g_hammer},
      body = "Excellent, you are now carrying the hammer. Right-click to swing it, or left-click again to drop it.\n\n"..
         "The hammer is a slow, but powerful, weapon. A hammer can destroy any wooden "..
         "object in a single hit."
   },

   [8] = {
      title = "MINI MAP",
      body = "By now, you will have noticed the mini-map in the bottom right hand corner. "..
         "This shows all rooms that have been mapped by your knight.\n\n"..
         "Normally rooms are mapped automatically, but if a monster or enemy "..
         "knight is in the room with you, then mapping is delayed "..
         "until they have left.\n\n"..
         "Also, certain magical scrolls may complete your map for you, "..
         "or even wipe it completely."
   },

   [9] = {
      title = "LOCKED DOOR",
      graphics = {C.g_hdoor_background, C.g_door_vwc},
      body = "This door seems to be locked. Never mind, since it is made of wood, "..
         "you can just smash it with your hammer..."
   },

   [10] = {
      title = "IRON DOOR",
      graphics = {C.g_hdoor_background, C.g_door_vic},
      body = "You can't break down iron doors. The only way to get past a "..
         "locked iron door is to find the key (or pick the lock)."
   },

   [11] = {
      title = "ENTRY POINT",
      graphics = {C.g_home_north},
      colour_changes = {{0xff0000, 0}},  -- change red to black
      body = "This is your entry point. You can't use it to leave the dungeon yet, because you don't "..
         "have enough gems."
   },

   [12] = {
      title = "ENTRY POINT",
      graphics = {C.g_home_south},
      body = "This is a dungeon entrance. Each knight will have their own separate entry point, "..
         "and there will also be some dungeon entrances (like this one) that do not belong to any knight.\n\n"..
         "You can't do anything at this entry point - you need to return to your own entry point "..
         "(at the top of the map) to win the quest."
   },

   [13] = {
      title = "SWITCH ROOM",
      graphics = {C.g_switch_up},
      body = "You enter a room filled with iron gates and ancient machinery.\n\n"..
         "It looks like you need to reach those levers at the top, "..
         "but the closed gates are blocking your way.\n\n"..
         "See if you can find a way past."
   },

   [14] = {
      title = "SWITCH ROOM",
      graphics = {C.g_switch_up},
      body = "Well done, you have reached the first lever.\n\n"..
         "To pull it, move towards it (push %U), then, while still holding down the key, "..
         "left-click the mouse."
   },

   [15] = {
      title = "SWITCH ROOM",
      graphics = {C.g_switch_up},
      body = "Well done. That seems to have opened one of the gates on the right-hand side. "..
         "See if you can get the other one open."
   },

   [16] = {
      title = "GEM RELEASED",
      graphics = {C.g_floor1, C.g_gem},
      body = "That's it! Both gates are now open and you can reach the first gem!\n\n"..
         "Make your way to the top right hand corner of the room, and pick up the gem."
   },

   [17] = {
      title = "GATE CLOSED!",
      graphics = {C.g_hdoor_background, C.g_door_hgc},
      body = "Oh no! The gate has closed behind us! There is now only one way to go: onwards...\n\n"..
         "At least we got the gem. Left-click to pick it up."
   },

   [18] = {
      title = "GEM",
      graphics = {C.g_floor1, C.g_gem},
      body = "Congratulations, you now have one gem. Notice how a gem icon has appeared at "..
         "the bottom of the screen, to let you know you are carrying it.\n\n"..
         "If you want to drop the gem, you can click \"Drop Gem\" on the "..
         "Action Bar (fourth from the right). You won't need to do that in this Tutorial, "..
         "but in a multiplayer game it can sometimes be useful to hide gems in secret places, "..
         "so that your opponents can't find them."
   },

   [19] = {
      title = "SECRET ROOM",
      body = "Well done, you have found the secret room. Give yourself a pat on the back.\n\n"..
         "Here is a bonus gem for your trouble."
   },

   [20] = {
      title = "GEM",
      graphics = {C.g_floor1, C.g_gem},
      body = "You forgot to pick up the gem. You will need to go back for it, otherwise you "..
         "won't be able to complete the quest!\n\n"..
         "Go back to the room with the gem, and stand on top of it. Then left-click the "..
         "mouse to pick it up."
   },

   [21] = {
      title = "PIT",
      graphics = {C.g_pit_o},
      body = "Many a careless knight has been lost, after stumbling into one of these Bottomless Pits. "..
         "Do not make the same mistake."
   },

   [22] = {
      title = "PIT",
      graphics = {C.g_pit_o},
      body = "Argh, you fell into a Bottomless Pit!\n\n"..
         "Note that your items (including any gems carried) have been left behind at the place where you "..
         "died. Be sure to collect them again before continuing..."
   },

   [23] = {
      title = "GEM",
      graphics = {C.g_table_south, C.g_gem},
      body = "A glint of light catches your eye from the nearby table. You have found another gem!\n\n"..
         "Picking up items from tables is similar to opening doors; you first have to \"approach\" the table "..
         "(by pushing your knight towards it), then left-click the mouse (while still holding the "..
         "movement key)."
   },

   [24] = {
      {
         popup = true,
         title = "COMBAT",
         body = "You have encountered your first monster: a vampire bat. Bats are bloodthirsty "..
            "and their sharp teeth can pierce even a knight's plate armour.\n\n"..
            "Before continuing, we will briefly explain how combat works."         
      },

      {
         popup = true,
         title = "WEAPONS",
         body = "As you know, you can swing your current weapon by right-clicking.\n\n"..
            "Different weapons have different combat characteristics. The sword is the fastest, but it "..
            "does the least damage. By contrast, the hammer is powerful, but slow. Axes "..
            "are somewhere in between.\n\n"..
            "In this case you should drop your hammer (left-click) if you are still carrying it; you will "..
            "need a fast weapon to deal with bats."
      },

      {
         popup = true,
         title = "DAMAGE AND HEALTH",
         body = "As you land blows on monsters (or other knights), their health will decrease. "..
            "When health drops to zero, the creature dies.\n\n"..
            "Your own health level is shown in the Potion Bottle at the lower left. When this empties, you "..
            "will die. Fortunately, it is full at the moment.\n\n"..
            "You can regain lost health by returning to your Entry Point, or by drinking healing potions."
      },

      {
         title = "VAMPIRE BAT",
         graphics = {C.g_floor1, C.g_vbat_1},
         body = "Now pull the lever to release the bat. Then turn towards it, and right-click to kill it.\n\n"..
            "Remember: don't panic. Time your blows carefully and you should have no problems."
      }
   },

   [26] = {
      title = "CHAMBER OF BATS",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Ah, this is the Chamber of Vampire Bats. A dangerous place for a knight, if ever there was one.\n\n"..
         "When you pull the lever, bats will start appearing from "..
         "the holes in the floor. Your mission is to kill five of them, before they kill you.\n\n"..
         "Good luck..."
   },

   [28] = {
      title = "CHAMBER OF BATS",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Mission complete.\n\n"..
         "The exits to this chamber have been unlocked. Go find the rest of those gems!"
   },

   [29] = {
      title = "MISSION FAILED",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Oh well. Pull the lever and have another go."
   },

   [30] = {
      title = "MISSION FAILED",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Never mind... pull the lever and try once more.\n\n"..
         "Remember, the key to killing bats is to keep calm. Face towards the bat, wait until it "..
         "gets into range, then right-click to splat it. If "..
         "it bites you, don't panic... just wait to see which way it moves, turn to "..
         "face it, and try again."
   },

   [31] = {
      title = "MISSION FAILED",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Hmm, the bats seem to be winning this one. Never mind, we will let you "..
         "skip this area if you want. Just go through the doors at the left and right, "..
         "which are now open.\n\n"..
         "If you want to try again, then pull the lever once more."
   },

   [32] = {
      title = "CHAMBER OF BATS - LEVEL 2",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Oh so you want more, do you? Very well, let's make this more interesting. Bats "..
         "will spawn faster this time, and you need to kill 8 instead of 5.\n\n"..
         "(This is an optional bonus mission. You don't need to complete it to win the Tutorial.)"
   },

   [36] = {
      title = "CHAMBER OF BATS - LEVEL 5",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "You truly are a glutton for punishment, I see. Very well, this will be your toughest "..
         "challenge yet.\n\nYour mission is to kill 15 bats. Fight!"
   },

   [37] = {
      title = "CHAMBER OF BATS",
      graphics = {C.g_wooden_floor, C.g_vbat_1},
      body = "Congratulations! You have completed all five of the vampire bat levels."
   },

   [38] = {
      title = "KEY",
      graphics = {C.g_table_small, C.g_key},
      body = "Ah, a key. I wonder what it unlocks?\n\n"..
         "(To pick up the key, approach the table, then, while still holding the movement key, left-click.)"
   },

   [39] = {
      title = "KEY",
      graphics = {C.g_table_small, C.g_key},
      body = "Key collected.\n\n"..
         "In a Knights dungeon, there will be up to three different keys, each opening a different "..
         "set of locks. There will also be one set of lock picks, which can be used to open locks "..
         "even when you don't have the key (although picking a lock does take time)."
   },

   [40] = {
      title = "TREASURE CHESTS",
      graphics = {C.g_chest_north},
      body = "This room contains three treasure chests.\n\n"..
         "You can open a chest in the same way you would a door (in other words, by approaching it "..
         "and left-clicking the mouse)."
   },

   [41] = {
      title = "POTION",
      graphics = {C.g_open_chest_north, C.g_potion},
      body = "This chest contains a magic potion. Potions have various effects -- most helpful, but some harmful. "..
         "Potions are automatically drunk when you pick them up.\n\n"..
         "(To pick up the potion, approach the chest, keep the movement key held down, and left-click.)"
   },

   [42] = {
      title = "SUPER",
      graphics = {C.g_open_chest_north, C.g_potion},
      body = "This potion has given you Super!\n\n"..
         "Super is a combination of extra speed, strength and regeneration.\n\n"..
         "The effect will last for a limited time. While your Super is in effect, the Potion Bottle (at lower left "..
         "of screen) will flash in many different colours."
   },

   [43] = {
      title = "GEM",
      graphics = {C.g_open_chest_north, C.g_gem},
      body = "Well done, you have found another gem! Pick it up by left-clicking (while you are approaching "..
         "the chest)."
   },

   [44] = {
      title = "TRAPPED CHEST",
      graphics = {C.g_open_chest_north},
      body = "Did I tell you that chests can sometimes be trapped?\n\n"..
         "Ah well, I must have forgotten to mention it.\n\n"..
         "This chest had a spring blade traps. There are also poison traps. You "..
         "will discover one way to avoid traps later in this Tutorial."
   },

   [45] = {
      title = "PENTAGRAM",
      graphics = {C.g_pentagram},
      body = "Pentagrams contain powerful, if rather dangerous magic. They can grant invisibility or "..
         "invulnerability, teleport you, or even turn you into a zombie.\n\n"..
         "There are also some pentagrams which are merely painted onto the ground, and have no special powers.\n\n"..
         "To use a pentagram simply walk over it."
   },

   [48] = {
      title = "LOCKED DOOR",
      graphics = {C.g_vdoor_background, C.g_door_vic},
      body = "This door is locked. Iron doors cannot be bashed down, so you will need to find a key."
   },

   [49] = {
      title = "SKULL ROOM",
      graphics = {C.g_skull_right},
      body = "As you enter this room, you notice three huge stone skulls staring across at you. "..
         "You have a bad feeling about this..."
   },

   [50] = {
      title = "SKULL ROOM",
      graphics = {C.g_skull_right},
      body = "Oh no! This room is trapped!\n\n"..
         "There is a safe path through, but can you find it?"
   },

   [51] = {
      title = "GEMS LEFT BEHIND",
      graphics = {C.g_pressure_plate, C.g_stuff_bag},
      body = "You made it through the skull room, but you left your gems behind!\n\n"..
         "Unfortunately you will have to go back and pick them up... without the gems you cannot win. "..
         "Try not to set off the skull traps again!"
   },

   [52] = {
      title = "ZOMBIES",
      graphics = {C.g_floor1, C.g_zom1n},
      body = "Argh! This place is crawling with zombies!\n\n"..
         "Zombies are tough opponents, but they are slow and not too smart. You should be able "..
         "to defeat them quite easily using your sword."
   },

   [53] = {
      title = "KEY",
      graphics = {C.g_floor1, C.g_key},
      body = "Another key is here. Walk over it and left-click to pick it up."
   },

   [54] = {
      title = "LOCK PICKS",
      graphics = {C.g_inv_lockpicks},
      body = "On closer inspection, you find that this is actually a set of lock picks.\n\n"..
         "To use them, first approach a locked door or chest, then click the \"Pick Lock\" icon on the Action Bar. "..
         "Keep the mouse button held down while your knight picks the lock."
   },

   [55] = {
      title = "AXE",
      graphics = {C.g_floor1, C.g_axe},
      body = "This is an axe. Left-click (while standing on it) to pick it up.\n\n"..
         "The axe does more damage than the sword, but is slightly slower. It can also be thrown, "..
         "or used to smash wooden objects (they will need a few hits).\n\n"..
         "When you right-click, your knight will either swing or throw the axe, depending on the situation. "..
         "If you use the Action Bar you can decide whether to swing or throw."
   },
   
   [56] = {
      title = "CROSSBOW AND BOLTS",
      graphics = {C.g_floor1, C.g_crossbow},
      body = "This room contains a crossbow and bolts. The crossbow is a powerful weapon -- if it is loaded.\n\n"..
         "To try out the crossbow, first pick it up. Also pick up the bolts. Then stand still, and your knight "..
         "will start loading the crossbow (you will hear a clicking sound). Wait until loading is complete. "..
         "Then you can fire the crossbow by right-clicking.\n\n"..
         "After the crossbow is fired, you will need to reload before you can fire again."
   },

   [57] = {
      title = "CRYSTAL BALL",
      graphics = {C.g_crystal_ball},
      body = "This is a crystal ball. It can be used to find out the location of other knights in the dungeon.\n\n"..
         "If you approach a crystal ball, then - as long as you are holding the movement key - "..
         "the locations of other knights will be revealed on your mini-map.\n\n"..
         "Note that in this Tutorial, there are no other knights in the dungeon, so the crystal ball will have no "..
         "effect."
   },

   [58] = {
      title = "SCROLL",
      graphics = {C.g_floor1, C.g_scroll},
      body = "This is a magical scroll. Like potions, scrolls have various "..
         "effects. When picked up, a scroll's powers are consumed and it disappears."
   },

   [59] = {
      title = "TREASURE CHESTS",
      graphics = {C.g_chest_north},
      body = "More treasure chests. I wonder what's inside?"
   },

   [60] = {
      title = "REGENERATION",
      graphics = {C.g_open_chest_north, C.g_potion},
      body = "This potion has given you Regeneration. This causes you to slowly recover lost health. While it "..
         "is in effect, your Potion Bottle (lower left of screen) will turn orange."
   },

   [61] = {
      title = "POISON",
      graphics = {C.g_open_chest_north, C.g_potion},
      body = "Oh dear, that potion was poisonous.\n\n"..
         "In general, most potions are helpful, but some will poison or paralyze you.\n\n"..
         "You can also find potions of poison immunity. These will protect you from harmful potions (as well "..
         "as poison traps)."
   },

   [62] = {
      title = "LOCKED CHEST",
      graphics = {C.g_chest_south},
      body = "Just like doors, chests will sometimes be locked.\n\n"..
         "You could try to break into the chest using a hammer or axe "..
         "(or even your sword, if you "..
         "are patient). However, smashing a chest will destroy any potions inside.\n\n"..
         "Alternatively, if you have found lock picks, now is the time to use them. Click the "..
         "\"Pick Lock\" icon on the Action Bar, and hold the mouse button down until the chest opens. "..
         "(This may take some time.)" 
   },

   [64] = {
      title = "TRAPS",
      graphics = {C.g_floor1, C.g_closed_bear_trap},
      body = "In Knights you can set traps for the other players to walk into.\n\n"..
         "There are three types of traps: bear traps, spring blade traps and poison traps. All three are available "..
         "in this room.\n\n"..
         "Pick up each trap to learn more about what it does."
   },

   [65] = {
      title = "STAFF",
      graphics = {C.g_floor1, C.g_staff},
      body = "A staff cannot be used in combat. Instead, it is used for disarming traps. "..
         "If you open a door or chest while holding a staff, then no trap will harm you."
   },

   [66] = {
      title = "THROWING DAGGERS",
      graphics = {C.g_floor1, C.g_daggers},
      body = "Daggers do little damage on their own, but can be deadly if used in large numbers.\n\n"..
         "To throw daggers, click the dagger icon on the Action Bar (fifth from the left). "..
         "You will need to hold down the mouse button."
   },
   
   [67] = {
      title = "STUFF BAG",
      graphics = {C.g_floor1, C.g_stuff_bag},
      body = "It looks like another knight died here, and left some stuff behind. Make sure you pick up "..
         "the bag, to see what they dropped."
   },

   [68] = {
      title = "ALMOST HOME!",
      body = "Victory awaits you! Just go through the corridor to the north, climb the stairs to your "..
         "entry point, and escape from this dungeon at last!"
   },
   
   [69] = {
      title = "NOT ENOUGH GEMS...",
      graphics = {C.g_menu_drop_gem},
      body = "Well done -- you have found a way back to your entry point (it's just to the north).\n\n"..
         "Unfortunately you don't have enough gems to win yet. Go back through the dungeon and find the missing "..
         "gem(s).\n\n"..
         "Remember, you can check the mini-map (lower right of screen) to navigate and check which areas you "..
         "haven't explored yet."
   },

   [71] = {
      title = "QUICKNESS",
      graphics = {C.g_floor1, C.g_scroll},
      body = "This scroll has given you Quickness. This is a temporary effect. While in effect, your "..
         "Potion Bottle (lower left of screen) will turn purple."
   },

   [72] = {
      title = "BEAR TRAP",
      graphics = {C.g_floor1, C.g_closed_bear_trap},
      body = "A bear trap can be set on any open dungeon square. To set the trap, just click the bear "..
         "trap icon on the Action Bar. Any knight who walks into the trap will be injured and stuck for a while.\n\n"..
         "A bear trap set by one of your opponents will always be visible on your screen, "..
         "and you can safely disarm it by striking it with a weapon. "..
         "However, the sound of the bear trap closing will be loud enough to be "..
         "heard across the dungeon. This makes bear traps useful as \"alarms\"; you can set one in a strategic "..
         "place, and be alerted when someone sets it off."
   },

   [73] = {
      title = "POISON NEEDLE TRAP",
      graphics = {C.g_menu_poison_trap},
      body = "You can set poison needle traps on any door or chest. Any knight who tries to open the trapped "..
         "object will be killed instantly (unless they are immune to poison, or using a staff).\n\n"..
         "To set traps, approach a closed door or chest, then (while still holding the movement key) click the "..
         "trap icon on the Action Bar."
   },

   [74] = {
      title = "SPRING BLADE TRAP",
      graphics = {C.g_menu_blade_trap},
      body = "You can set spring blade traps on any door or chest. Any knight who tries to open or strike the "..
         "trapped object will cause the blade to fire (towards the direction from which the trap was originally "..
         "set).\n\n"..
         "To set traps, approach a closed door or chest, then (while still holding the movement key) click the "..
         "trap icon on the Action Bar."
   },


   [75] = {
      title = "LOCKED DOOR",
      graphics = {C.g_hdoor_background, C.g_door_hic},
      body = "This door leads back to your entry point. Unfortunately, it is locked.\n\n"..
         "You can use your lock picks to get past. Just click on the \"Pick Lock\" icon on the Action Bar "..
         "(3rd from the right) and hold down the mouse button until the door opens..."
   },

   [76] = {
      title = "LOCKED DOOR",
      graphics = {C.g_hdoor_background, C.g_door_hic},
      body = "This door leads back to your entry point. Unfortunately, it is locked.\n\n"..
         "You will need to find some way to get past..."
   },
   
   [77] = {
      title = "SECRET DOOR",
      graphics = {C.g_wall},
      body = "You hear a grinding sound as the wall slides away. You have opened a secret door!"
   },

   [78] = {
      title = "ZOMBIFIED!",
      graphics = {C.g_floor1, C.g_zom1w},
      body = "Oh no, you got turned into a zombie! Slay the foul creature with your sword.\n\n"..
         "You will also need to step back on to the pentagram again, to pick up the stuff you were carrying..."
   },

   [79] = {
      title = "DEATH",
      graphics = {C.g_dead_knight_1, C.g_stuff_bag},
      body = "Oh dear, you seem to have died.\n\n"..
         "Fortunately, in Knights, lives are unlimited. However, "..
         "your items have been left behind at the "..
         "place where you died. (If you had multiple items, they will have "..
         "been left in a brown bag like the one shown above.) "..
         "Don't forget to go back and collect them before continuing."
   }
}

-- Some messages that don't use the above system, for one reason or another.

gem_title = "GEM COLLECTED"

-- This is shown for 2nd and 3rd gems
gem_msg = "Congratulations, you now have %d gems! You need another %d before you can win."

-- This is shown for 4th gem
penultimate_gem_msg = "Congratulations, you now have 4 gems! Only one more to go!"

-- This is shown for 5th gem
last_gem_msg = "Congratulations, you now have 5 gems! All you need to do now is find a way back "..
   "to your starting point, and you will be able to win the quest!"



stuff_title = "STUFF BAG"

stuff_msg = "Excellent, this bag contained a gem!\n\n"
stuff_msg_final = "You now have enough gems to win! All you need to do now is find a way to get back to your "..
   "entry point..."
stuff_msg_nonfinal = "You now have %d gems. Only %d more to find, then you will be able to win the quest."

bat_mission_title = "CHAMBER OF BATS"
bat_mission_title_2 = "CHAMBER OF BATS - LEVEL %d"
bat_mission_text = "Fight!"

bat_failed_title = "MISSION FAILED"
bat_failed_text = "Pull the lever again if you want to have another go."

bat_success_title = "MISSION COMPLETE"
bat_success_text = "Mission successful. Well done."

bat_objective_sing = "Kill 1 bat"
bat_objective_pl   = "Kill %d bats"
normal_objective_1 = "Retrieve 5 gems"
normal_objective_2 = "Escape via your entry point"
