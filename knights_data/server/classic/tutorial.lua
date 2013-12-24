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


-- These values must agree with tutorial_manager.cpp
TUT_ITEM = 1
TUT_DEATH = 2
TUT_GEM = 3
TUT_TWO_GEMS = 4
TUT_THREE_GEMS = 5

-- The other TUT_ values are arbitrary (but they must be unique).

-- Items
TUT_AXE = 21
TUT_HAMMER = 22
TUT_STAFF = 23
TUT_CROSSBOW = 24
TUT_BOLTS = 25
TUT_DAGGERS = 26
TUT_POISON_TRAP = 27
TUT_BLADE_TRAP = 28
TUT_BEAR_TRAP = 29
TUT_POTION = 30
TUT_SCROLL = 31
TUT_KEY = 32
TUT_LOCKPICKS = 33

-- Tiles
TUT_HOME = 51
TUT_PIT = 52
TUT_SWITCH = 53
TUT_PENTAGRAM = 54
TUT_CRYSTAL_BALL = 55
TUT_DOOR = 56
TUT_IRON_DOOR = 57
TUT_PORTCULLIS = 58
TUT_CHEST = 59



tutorial_table = {

    TUT_ITEM,
         "ITEM",
         "To pick up an item, stand on top of it, then left-click (or use the Action Bar).^To pick up items from tables, you must walk towards the table (by holding the movement key) before clicking.^In this tutorial, when you pick up an item, its description will appear here. Try picking up a few items to see what they do.",

    TUT_DEATH,
         "DEATH",
         "Oh dear, you seem to have died. Maybe it was a trapped chest that got you, or perhaps a poisoned potion?^After death, you will respawn back at your entry point, but any items you were carrying will be left behind at the place where you died.",


    -- Tiles

    TUT_HOME, 
         "ENTRY POINT",
         "The Entry Point is where your knight enters the dungeon.^Be sure to remember where your entry point is, as you will need to return here, once you have collected three gems, to complete your quest.^If you are injured, then returning to your entry point will heal you -- slowly.",

    TUT_CHEST,
         "TREASURE CHEST", 
         "Treasure chests often contain useful items. To open, walk towards the chest and click the left mouse button (or use the Action Bar).^Beware that some chests may be trapped. Using a staff is one way to avoid traps. Destroying the chest is another, although this risks damaging the contents.^Some chests are locked, and will require a key (or a well placed hammer blow) to open.",

    TUT_DOOR,
         "DOOR",
         "This is a door. To open it, walk up to it, then either use the Action Bar, or just left-click (while walking towards the door).^If the door is locked, you can either look for a key, or else try to bash it down (by hitting it enough times).",

    TUT_IRON_DOOR,
         "IRON DOOR",
         "Iron doors can be opened in the same way as ordinary doors (i.e. by moving towards the door and left-clicking).^Iron doors cannot be destroyed -- the only way to get past a locked iron door is to find the key, or use lock picks.",

    TUT_PIT, 
         "PIT",
         "Many a careless knight has been lost, after stumbling into one of these Bottomless Pits. Do not make the same mistake.",

    TUT_SWITCH,
         "SWITCH", 
         "Switches have a variety of effects. Some open or close doors or pits, while others set off hidden traps.^To activate a switch, move towards it and left-click (or use the Action Bar).",

    TUT_PENTAGRAM,
         "PENTAGRAM",
         "Pentagrams contain powerful, if rather dangerous magic. They can grant invisibility or invulnerability, teleport you next to another knight, or even turn you into a zombie. There are also some pentagrams which are merely painted onto the ground, and have no special powers.^To use a pentagram simply walk over it.",

    TUT_CRYSTAL_BALL, 
         "CRYSTAL BALL", 
         "A crystal ball allows you to see the location of other knights, as flashing dots on your map. To use it, just walk towards it.^(Note that in this tutorial, you are the only player in the dungeon, so the crystal ball will have no effect.)",

    TUT_PORTCULLIS, 
         "PORTCULLIS", 
         "This closed portcullis will block your path. Portcullises cannot be opened directly, but sometimes they can be controlled using switches elsewhere in the dungeon.",


    -- Items

    TUT_AXE, 
         "AXE", 
         "The axe does more damage than the sword, but is slightly slower. It can also be thrown, or used to smash wooden objects.^When you right-click, your knight will either swing or throw the axe, depending on the situation. If you use the Action Bar you can choose whether to swing or throw.",

    TUT_HAMMER,
          "HAMMER", 
          "The hammer is the slowest weapon, but also the most powerful. A hammer can destroy any wooden object in a single hit.^Remember, you can use your hammer by right-clicking. Give it a try, you know you want to :)",

    TUT_STAFF,
          "STAFF",
          "A staff cannot be used in combat, but is instead used for disarming traps. If you open a door or chest while holding a staff, then no trap will harm you.",

    TUT_CROSSBOW, 
         "CROSSBOW", 
         "The crossbow is a powerful weapon -- if it is loaded.^To load a crossbow you will first need to find some bolts. Then stand still, while holding the crossbow, and your knight will start loading it automatically. Once loading is complete, click the right mouse button (or use the Action Bar) to fire.",

    TUT_BOLTS, 
         "BOLTS",
         "Bolts are used as ammunition for the crossbow. Bolts are useless without a crossbow, and vice versa.",

    TUT_DAGGERS, 
         "THROWING DAGGERS",
         "Daggers do little damage on their own, but can be deadly if used in large numbers.^To throw, left-click the dagger icon on the Action Bar (you will need to hold down the mouse button).",

    TUT_POISON_TRAP, 
         "POISON NEEDLE TRAP",
         "You can set poison needle traps on any door or chest. Any knight who tries to open the trapped object will be killed instantly (unless they are immune to poison, or using a staff).^To set traps, move towards a closed door or chest (and keep the movement key held down), then click the trap icon on the Action Bar.",

    TUT_BLADE_TRAP,
         "SPRING BLADE TRAP",
         "You can set spring blade traps on any door or chest. Any knight who tries to open or strike the trapped object will cause the blade to fire (towards the direction from which the trap was originally placed).^To set traps, move towards a closed door or chest (and keep the movement key held down), then click the trap icon on the Action Bar.",

    TUT_BEAR_TRAP,
         "BEAR TRAP", 
         "A bear trap can be set on any open dungeon square. To set the trap, just click the bear trap icon on the Action Bar.^Any knight who walks into an open bear trap will be injured and stuck for a while. Open bear traps can be safely disarmed by striking them with a weapon.^Bear traps are useful as 'alarms'. The sound of a bear trap closing is loud enough to be heard across the dungeon, so you can set the trap in a strategic place, and then be alerted when someone sets it off.",

    TUT_POTION, 
         "POTION",
         "Potions are automatically drunk when you pick them up. Potions have various effects, usually helpful, but sometimes harmful.^Your Potion Bottle (lower left corner of screen) will change colour while the potion is in effect.",

    TUT_SCROLL, 
         "SCROLL",
         "Scrolls have various magical effects. When picked up, a scroll's powers are consumed and it disappears.",

    TUT_GEM,
         "GEM", 
         "Congratulations, you have found your first gem! Collect two more, then return to your starting point, to complete your quest.^Gems can be dropped using the Action Bar. This can be useful for hiding gems away from your enemies.",

    TUT_TWO_GEMS,
         "GEM",
         "Two gems collected. One more to go!",

    TUT_THREE_GEMS,
         "THREE GEMS COLLECTED",
         "Congratulations, you have found three gems. Now all you have to do is return to your starting point to complete your quest!^What's that, you can't remember where your starting point was? Alas, many forgetful knights find themselves in this situation. In this case you will just have to try walking into different entry points until you find the right one!^(If you are really stuck, you can always commit suicide, and see where you respawn. But this is a desperate move, as you will drop all gems when you die.)",

    TUT_KEY, 
         "KEYS",
         "Keys are used to open locked doors or chests. There are three different types of key, and each one will open a different set of locks.",

    TUT_LOCKPICKS,
        "LOCK PICKS",
        "This is a set of lock picks. To use it, walk towards a locked door, then click the 'Pick Lock' icon on the Action Bar. Keep the mouse button held down while your knight picks the lock. Using lock picks takes some time, so be patient."

}
