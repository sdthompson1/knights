/*
 * protocol.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2012.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Knights is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Knights.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/*
 * Defines the network protocol between server and client.
 *
 */

#ifndef PROTOCOL_HPP
#define PROTOCOL_HPP

#include "my_exceptions.hpp"

#include <vector>

//
// Exception class, thrown when a value is out of the expected range,
// or the data stream ends unexpectedly.
//

class ProtocolError : public ExceptionBase {
public:
    explicit ProtocolError(const std::string &msg) : ExceptionBase(msg) { }
};


//
// 1-byte "message codes"
//

// Messages sent by the client
enum ClientMessageCode {
    CLIENT_SET_PLAYER_NAME = 1,      // followed by string (my player name). should be 1st cmd sent.
    CLIENT_JOIN_GAME = 3,            // followed by string (game name I want to join)
    CLIENT_JOIN_GAME_SPLIT_SCREEN = 4,   // followed by string (game name I want to join)
    CLIENT_LEAVE_GAME = 5,           // (I want to leave game and return to "unjoined" state). No extra data
    CLIENT_CHAT = 6,                 // followed by string
    CLIENT_SET_READY = 7,            // followed by ubyte (ready-status)
    CLIENT_SET_HOUSE_COLOUR = 8,     // followed by ubyte (which house col to set).
    CLIENT_SET_MENU_SELECTION = 9,   // followed by 2 varints
    CLIENT_FINISHED_LOADING = 10,    // (I have loaded and am ready to start playing) no extra data 
    CLIENT_SEND_CONTROL = 11,        // followed by ubyte (bits 0-6: control num; bit 7: display num)
    CLIENT_READY_TO_END = 12,        // (I have clicked mouse on winner/loser screen and want to go to lobby) no extra data
    CLIENT_QUIT = 13,                // (I have pressed escape and want to go to lobby) no extra data
    CLIENT_SET_PAUSE_MODE = 14,      // followed by ubyte (paused-flag). only works for split screen mode currently.
    CLIENT_SEND_PASSWORD = 15,       // followed by string (the password).
    CLIENT_SET_OBS_FLAG = 17,        // followed by ubyte (0=I want to be a player, 1=I want to be an observer)
    CLIENT_REQUEST_SPEECH_BUBBLE = 18,   // followed by ubyte (0=don't show, 1=show)
    CLIENT_SET_APPROACH_BASED_CONTROLS = 19,     // followed by ubyte (1=true 0=false)
    CLIENT_SET_ACTION_BAR_CONTROLS = 20,         // followed by ubyte (1=true 0=false)
    CLIENT_RANDOM_QUEST = 21         // no extra data
};

// Messages sent by the server
enum ServerMessageCode {

    SERVER_ERROR = 1,                // followed by string

    SERVER_CONNECTION_ACCEPTED = 2,  // followed by varint (server version number)
    
    SERVER_JOIN_GAME_ACCEPTED = 3,   // complex
    SERVER_JOIN_GAME_DENIED = 4,     // followed by string (reason)
    SERVER_NOTUSED = 5,              // was SERVER_INITIAL_PLAYER_LIST in version 011 and below
    SERVER_PLAYER_CONNECTED = 6,     // followed by string (player name)
    SERVER_PLAYER_DISCONNECTED = 7,  // followed by string (player name)
    
    SERVER_LEAVE_GAME = 8,           // no extra data
    SERVER_SET_MENU_SELECTION = 9,   // complex
    SERVER_SET_QUEST_DESCRIPTION = 10,  // followed by string
    SERVER_START_GAME = 11,          // followed by ubyte (num_displays), ubyte (deathmatch flag). implicitly clears all ready-flags.
    SERVER_GOTO_MENU = 12,           // no extra data.
    SERVER_START_GAME_OBS = 13,      // followed by ubyte (num_displays), ubyte (deathmatch flag), NDisp strings (player names, for obs-mode)
                                     //   + ubyte (0 = at start of game, 1 = halfway through.)
    SERVER_GO_INTO_OBS_MODE = 14,    // followed by ubyte (num displays) + NDisp strings (player names, for obs-mode)

    SERVER_PLAYER_JOINED_THIS_GAME = 20,     // followed by string (name), ubyte (obs-flag), ubyte (house-col)
    SERVER_PLAYER_LEFT_THIS_GAME = 21,       // followed by string (name), ubyte (obs-flag)
    SERVER_SET_READY = 22,           // followed by string (name) and ubyte (ready status)
    SERVER_SET_HOUSE_COLOUR = 23,    // followed by string (name), ubyte (which house col to set).
    SERVER_SET_AVAILABLE_HOUSE_COLOURS = 24,    // followed by ubyte (num hse cols) and the cols as (r,g,b) ubyte triples.
    SERVER_SET_OBS_FLAG = 25,        // followed by string (player name), ubyte (1=obs 0=player).
    SERVER_DEACTIVATE_READY_FLAGS = 26,         // no additional data
    
    SERVER_CHAT = 30,                // followed by string (sender), ubyte (0=lobby 1=player 2=observer 3=team), string (msg)
    SERVER_ANNOUNCEMENT = 31,        // followed by string (msg)
    SERVER_POP_UP_WINDOW = 32,       // complex. only used in 1-player games.
    
    SERVER_REQUEST_PASSWORD = 35,    // followed by ubyte (first_attempt)

    SERVER_UPDATE_GAME = 36,         // followed by string (game name), varint (num_players), varint (num_observers), ubyte (status code)
    SERVER_DROP_GAME = 37,           // followed by string (game name)
    SERVER_UPDATE_PLAYER = 38,       // followed by string (player name), string (game name), ubyte (1=obs 0=player)

    SERVER_PLAYER_LIST = 39,         // complex
    SERVER_TIME_REMAINING = 40,      // followed by varint (time remaining in milliseconds)

    SERVER_READY_TO_END = 41,        // followed by string (name of the player who is ready to end)
    
    // knights callbacks
    SERVER_PLAY_SOUND = 50,          // followed by varint (soundnum) + varint (frequency)
    SERVER_WIN_GAME = 51,            // no extra data
    SERVER_LOSE_GAME = 52,           // no extra data
    SERVER_SET_AVAILABLE_CONTROLS = 53,  // followed by ubyte (num-ctrls) + ubytes (bit7=primary-flag, bit0-6=control-id)
    SERVER_SET_MENU_HIGHLIGHT = 54,  // followed by ubyte (control-id)
    SERVER_FLASH_SCREEN = 55,        // followed by varint (delay)

    // dungeonview
    SERVER_SET_CURRENT_ROOM = 100,   // followed by varint (room num) + room-coord (width, height)
    SERVER_ADD_ENTITY = 101,         // complex
    SERVER_RM_ENTITY = 102,          // followed by varint (id)
    SERVER_REPOSITION_ENTITY = 103,  // followed by varint (id), room-coord
    SERVER_MOVE_ENTITY = 104,        // complex
    SERVER_FLIP_ENTITY_MOTION = 105, // followed by varint (id), ushort (initial_delay), ushort (motion_duration)
    SERVER_SET_ANIM_DATA = 106,      // complex
    SERVER_SET_FACING = 107,         // followed by varint (id), ubyte (facing)
    SERVER_CLEAR_TILES = 108,        // followed by room-coord
    SERVER_SET_TILE = 109,           // complex
    SERVER_SET_ITEM = 110,           // followed by room-coord, varint (gfx-id)
    SERVER_PLACE_ICON = 111,         // followed by room-coord, varint (gfx-id), ushort (duration)
    SERVER_FLASH_MESSAGE = 112,      // followed by string, ubyte (ntimes)
    SERVER_CANCEL_CONTINUOUS_MESSAGES = 113,  // no data
    SERVER_ADD_CONTINUOUS_MESSAGE = 114,  // followed by string
    SERVER_SET_SPEECH_BUBBLE = 115,  // followed by varint (id), ubyte (show flag)

    // minimap
    SERVER_SET_MAP_SIZE = 150,       // followed by 2 ubytes (width, height)
    SERVER_SET_COLOUR = 151,         // complex
    SERVER_WIPE_MAP = 152,           // no data
    SERVER_MAP_KNIGHT_LOCATION = 153, // followed by ubyte (plyrnum), EITHER 2 ubytes (x,y) OR 1 ubyte (255)
    SERVER_MAP_ITEM_LOCATION = 154,  // followed by 3 ubytes (x, y, flag)

    // status display
    SERVER_SET_BACKPACK = 200,       // followed by ubyte (slot), 2 varints (gfx ids), 2 ubytes (no_carried, no_max)
    SERVER_ADD_SKULL = 201,          // no data
    SERVER_SET_HEALTH = 202,         // followed by ubyte (health)
    SERVER_SET_POTION_MAGIC = 203,   // followed by ubyte (bit7 = poison immun, bits0-6 = potion_magic)
    // NOTE: 204, 205 no longer used (were set quest message / set quest icons)

    // misc
    SERVER_SWITCH_PLAYER = 250,      // followed by ubyte (player number)

    // extended messages
    // (will be ignored if the client doesn't know the message.)
    SERVER_EXTENDED_MESSAGE = 255    // followed by extended code (varint), payload length (ushort) and payload.
};

enum ServerExtendedCode {
    SERVER_EXT_SET_QUEST_HINTS = 1,
    SERVER_EXT_NEXT_ANNOUNCEMENT_IS_ERROR = 2
};


#endif
