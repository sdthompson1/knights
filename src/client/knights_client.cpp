/*
 * knights_client.cpp
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

#include "misc.hpp"

#include "anim.hpp"
#include "client_callbacks.hpp"
#include "client_config.hpp"
#include "colour_change.hpp"
#include "dungeon_view.hpp"
#include "graphic.hpp"
#include "mini_map.hpp"
#include "overlay.hpp"
#include "sound.hpp"
#include "status_display.hpp"
#include "knights_callbacks.hpp"
#include "knights_client.hpp"
#include "protocol.hpp"
#include "user_control.hpp"
#include "version.hpp"

#include "network/byte_buf.hpp"  // coercri

namespace {
    void ReadRoomCoord(Coercri::InputByteBuf &buf, int &x, int &y)
    {
        buf.readNibbles(x, y);
        x--;
        y--;
    }

    void ReadTileInfo(Coercri::InputByteBuf &buf, int &depth, bool &cc)
    {
        int x = buf.readUbyte();
        cc = (x & 128) != 0;
        depth = (x & 127) - 64;
    }
}

class KnightsClientImpl {
public:
    // ctor
    KnightsClientImpl() : ndisplays(0), player(0), 
                          knights_callbacks(0), client_callbacks(0)
    { 
        for (int i = 0; i < 2; ++i) last_cts_ctrl[i] = 0;
    }

    // data
    int ndisplays;
    int player;         // currently 'selected' display
    std::vector<unsigned char> out;
    boost::shared_ptr<ClientConfig> client_config;
    KnightsCallbacks *knights_callbacks;
    ClientCallbacks *client_callbacks;
    const UserControl *last_cts_ctrl[2];
    
    // helper functions
    void receiveConfiguration(Coercri::InputByteBuf &buf);
    const Graphic * readGraphic(Coercri::InputByteBuf &buf) const;
    const Anim * readAnim(Coercri::InputByteBuf &buf) const;
    const Overlay * readOverlay(Coercri::InputByteBuf &buf) const;
    const Sound * readSound(Coercri::InputByteBuf &buf) const;
    const UserControl * getControl(int id) const;
};

KnightsClient::KnightsClient()
    : pimpl(new KnightsClientImpl)
{
    // Write the initial version string.
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeString("Knights/" KNIGHTS_VERSION);
}

KnightsClient::~KnightsClient()
{
}

void KnightsClient::setClientCallbacks(ClientCallbacks *c)
{
    pimpl->client_callbacks = c;
}

ClientCallbacks* KnightsClient::getClientCallbacks() const
{
    return pimpl->client_callbacks;
}

void KnightsClient::setKnightsCallbacks(KnightsCallbacks *c)
{
    pimpl->knights_callbacks = c;
}

KnightsCallbacks* KnightsClient::getKnightsCallbacks() const
{
    return pimpl->knights_callbacks;
}

void KnightsClient::receiveInputData(const std::vector<ubyte> &data)
{
    // This is where we decode the messages coming from the server,
    // and turn them into calls to ClientCallbacks.

    Coercri::InputByteBuf buf(data);

    // some aliases (to save typing):
    ClientCallbacks * client_cb = pimpl->client_callbacks;
    KnightsCallbacks * knights_cb = pimpl->knights_callbacks;
    DungeonView * dungeon_view = knights_cb ? &knights_cb->getDungeonView(pimpl->player) : 0;
    MiniMap * mini_map = knights_cb ? &knights_cb->getMiniMap(pimpl->player) : 0;
    StatusDisplay * status_display = knights_cb ? &knights_cb->getStatusDisplay(pimpl->player) : 0;
    
    while (!buf.eof()) {
        const unsigned char msg_code = buf.readUbyte();
        switch (msg_code) {
           
        case SERVER_ERROR:
            {
                const std::string err = buf.readString();
                if (client_cb) client_cb->serverError(err);
            }
            break;

        case SERVER_CONNECTION_ACCEPTED:
            {
                const int server_version = buf.readVarInt();
                if (client_cb) client_cb->connectionAccepted(server_version);
            }
            break;
            
        case SERVER_JOIN_GAME_ACCEPTED:
            {
                pimpl->receiveConfiguration(buf);
                const int my_house_colour = buf.readUbyte();

                const int n_plyrs = buf.readVarInt();
                if (n_plyrs < 0) throw ProtocolError("n_players incorrect");
                std::vector<std::string> players;
                std::vector<bool> ready_flags;
                std::vector<int> hse_cols;
                for (int i = 0; i < n_plyrs; ++i) {
                    players.push_back(buf.readString());
                    ready_flags.push_back(buf.readUbyte() != 0);
                    hse_cols.push_back(buf.readUbyte());
                }
                
                const int n_obs = buf.readVarInt();
                std::vector<std::string> observers;
                if (n_obs < 0) throw ProtocolError("n_observers incorrect");
                observers.reserve(n_obs);
                for (int i = 0; i < n_obs; ++i) {
                    observers.push_back(buf.readString());
                }
                if (client_cb) client_cb->joinGameAccepted(pimpl->client_config,
                                                           my_house_colour,
                                                           players, ready_flags, hse_cols,
                                                           observers);
            }
            break;

        case SERVER_JOIN_GAME_DENIED:
            {
                const std::string reason = buf.readString();
                if (client_cb) client_cb->joinGameDenied(reason);
            }
            break;

        case SERVER_NOTUSED:
            throw ProtocolError("Cannot connect: This server is running an old version of Knights.");
            break;
            
        case SERVER_PLAYER_CONNECTED:
            {
                const std::string name = buf.readString();
                if (client_cb) client_cb->playerConnected(name);
            }
            break;

        case SERVER_PLAYER_DISCONNECTED:
            {
                const std::string name = buf.readString();
                if (client_cb) client_cb->playerDisconnected(name);
            }
            break;

        case SERVER_LEAVE_GAME:
            if (client_cb) client_cb->leaveGame();
            break;
            
        case SERVER_SET_MENU_SELECTION:
            {
                const std::string key = buf.readString();
                const int val = buf.readVarInt();
                std::vector<int> allowed_vals;
                const int num_allowed_vals = buf.readVarInt();
                allowed_vals.resize(num_allowed_vals);
                for (int i = 0; i < num_allowed_vals; ++i) {
                    allowed_vals[i] = buf.readVarInt();
                }
                if (client_cb) client_cb->setMenuSelection(key, val, allowed_vals);
            }
            break;

        case SERVER_SET_QUEST_DESCRIPTION:
            {
                const std::string quest_descr = buf.readString();
                if (client_cb) client_cb->setQuestDescription(quest_descr);
            }
            break;
            
        case SERVER_START_GAME:
            {
                pimpl->ndisplays = buf.readUbyte();
                const bool deathmatch_mode = buf.readUbyte() != 0;
                pimpl->player = 0;
                if (client_cb) {
                    client_cb->startGame(pimpl->ndisplays, deathmatch_mode,
                                         std::vector<std::string>(), false);
                }
            }
            break;

        case SERVER_START_GAME_OBS:
            {
                pimpl->ndisplays = buf.readUbyte();
                const bool deathmatch_mode = buf.readUbyte() != 0;
                pimpl->player = 0;
                std::vector<std::string> player_names;
                player_names.reserve(pimpl->ndisplays);
                for (int i = 0; i < pimpl->ndisplays; ++i) {
                    player_names.push_back(buf.readString());
                }
                const bool already_started = buf.readUbyte() != 0;
                if (client_cb) {
                    client_cb->startGame(pimpl->ndisplays, deathmatch_mode,
                                         player_names, already_started);
                }
            }
            break;

        case SERVER_GO_INTO_OBS_MODE:
            {
                pimpl->ndisplays = buf.readUbyte();
                pimpl->player = 0;
                std::vector<std::string> player_names;
                player_names.reserve(pimpl->ndisplays);
                for (int i = 0; i < pimpl->ndisplays; ++i) {
                    player_names.push_back(buf.readString());
                }
                if (knights_cb) knights_cb->goIntoObserverMode(pimpl->ndisplays, player_names);
                if (client_cb) client_cb->announcement("** You have been eliminated from this game. You are now in observer mode. Use left/right arrow keys to switch between players.");
            }
            break;

        case SERVER_GOTO_MENU:
            if (client_cb) client_cb->gotoMenu();
            break;

        case SERVER_PLAYER_JOINED_THIS_GAME:
            {
                const std::string name = buf.readString();
                const bool obs_flag = buf.readUbyte() != 0;
                const int house_col = buf.readUbyte();
                if (client_cb) client_cb->playerJoinedThisGame(name, obs_flag, house_col);
            }
            break;

        case SERVER_PLAYER_LEFT_THIS_GAME:
            {
                const std::string name = buf.readString();
                const bool obs_flag = buf.readUbyte() != 0;
                if (client_cb) client_cb->playerLeftThisGame(name, obs_flag);
            }
            break;

        case SERVER_SET_READY:
            {
                const std::string name = buf.readString();
                const int ready = buf.readUbyte();
                if (client_cb) client_cb->setReady(name, ready != 0);
            }
            break;

        case SERVER_SET_HOUSE_COLOUR:
            {
                const std::string name = buf.readString();
                const int x = buf.readUbyte();
                if (client_cb) client_cb->setPlayerHouseColour(name, x);
            }
            break;

        case SERVER_SET_AVAILABLE_HOUSE_COLOURS:
            {
                const int n = buf.readUbyte();
                std::vector<Coercri::Color> cols;
                cols.reserve(n);
                for (int i = 0; i < n; ++i) {
                    const unsigned char r = buf.readUbyte();
                    const unsigned char g = buf.readUbyte();
                    const unsigned char b = buf.readUbyte();
                    cols.push_back(Coercri::Color(r,g,b));
                }
                if (client_cb) client_cb->setAvailableHouseColours(cols);
            }
            break;

        case SERVER_SET_OBS_FLAG:
            {
                const std::string player = buf.readString();
                const bool new_obs_flag = (buf.readUbyte() != 0);
                if (client_cb) client_cb->setObsFlag(player, new_obs_flag);
            }
            break;

        case SERVER_CHAT:
            {
                const std::string whofrom = buf.readString();

                // we currently don't parse all the chat codes, we only look for 2 (Observer)
                // and use this to print "(Observer)" after the player's name...
                // Also: 3 = (Team)
                const int chat_code = buf.readUbyte();
                const bool is_observer = (chat_code == 2);
                const bool is_team = (chat_code == 3);
                const std::string msg = buf.readString();
                if (client_cb) client_cb->chat(whofrom, is_observer, is_team, msg);
            }
            break;

        case SERVER_ANNOUNCEMENT:
            {
                const std::string msg = buf.readString();
                if (client_cb) client_cb->announcement(msg);
            }
            break;

        case SERVER_POP_UP_WINDOW:
            {
                const int n = buf.readVarInt();
                std::vector<TutorialWindow> windows;
                windows.reserve(n);
                for (int i = 0; i < n; ++i) {
                    TutorialWindow win;
                    win.title = buf.readString();
                    win.msg = buf.readString();
                    win.popup = buf.readVarInt() != 0;
                    const int ngfx = buf.readVarInt();
                    win.gfx.reserve(ngfx);
                    win.cc.reserve(ngfx);
                    for (int i = 0; i < ngfx; ++i) {
                        win.gfx.push_back(pimpl->readGraphic(buf));
                        win.cc.push_back(ColourChange(buf));
                    }
                    windows.push_back(win);
                }
                if (knights_cb) knights_cb->popUpWindow(windows);
            }
            break;

        case SERVER_REQUEST_PASSWORD:
            {
                const bool first_attempt = buf.readUbyte() != 0;
                if (client_cb) client_cb->passwordRequested(first_attempt);
            }
            break;

        case SERVER_UPDATE_GAME:
            {
                const std::string game_name = buf.readString();
                const int num_players = buf.readVarInt();
                const int num_observers = buf.readVarInt();
                const GameStatus status_code = GameStatus(buf.readUbyte());
                if (client_cb) client_cb->updateGame(game_name, num_players, num_observers, status_code);
            }
            break;

        case SERVER_DROP_GAME:
            {
                const std::string game_name = buf.readString();
                if (client_cb) client_cb->dropGame(game_name);
            }
            break;

        case SERVER_UPDATE_PLAYER:
            {
                const std::string player_name = buf.readString();
                const std::string game_name = buf.readString();
                const bool obs_flag = buf.readUbyte() != 0;
                if (client_cb) client_cb->updatePlayer(player_name, game_name, obs_flag);
            }
            break;

        case SERVER_PLAYER_LIST:
            {
                const int nplayers = buf.readVarInt();
                std::vector<ClientPlayerInfo> player_list;
                player_list.reserve(nplayers);
                for (int i = 0; i < nplayers; ++i) {
                    ClientPlayerInfo inf;
                    inf.name = buf.readString();
                    inf.house_colour.r = buf.readUbyte();
                    inf.house_colour.g = buf.readUbyte();
                    inf.house_colour.b = buf.readUbyte();
                    inf.kills = buf.readVarInt();
                    inf.deaths = buf.readVarInt();
                    inf.frags = buf.readVarInt();
                    inf.ping = buf.readVarInt();
                    player_list.push_back(inf);
                }
                if (client_cb) client_cb->playerList(player_list);
            }
            break;

        case SERVER_PLAY_SOUND:
            {
                const Sound *sound = pimpl->readSound(buf);
                const int freq = buf.readVarInt();
                if (knights_cb && sound) knights_cb->playSound(pimpl->player, *sound, freq);
            }
            break;

        case SERVER_WIN_GAME:
            if (knights_cb) knights_cb->winGame(pimpl->player);
            break;

        case SERVER_LOSE_GAME:
            if (knights_cb) knights_cb->loseGame(pimpl->player);
            break;

        case SERVER_SET_AVAILABLE_CONTROLS:
            {
                const int n = buf.readUbyte();
                std::vector<std::pair<const UserControl*, bool> > avail;
                avail.reserve(n);
                for (int i = 0; i < n; ++i) {
                    const int x = buf.readUbyte();
                    const UserControl * ctrl = pimpl->getControl(x & 0x7f);
                    const bool primary = (x & 0x80) != 0;
                    avail.push_back(std::make_pair(ctrl, primary));
                }
                if (knights_cb) knights_cb->setAvailableControls(pimpl->player, avail);
            }
            break;

        case SERVER_SET_MENU_HIGHLIGHT:
            {
                const UserControl * ctrl = pimpl->getControl(buf.readUbyte());
                if (knights_cb) knights_cb->setMenuHighlight(pimpl->player, ctrl);
            }
            break;

        case SERVER_FLASH_SCREEN:
            {
                const int delay = buf.readVarInt();
                if (knights_cb) knights_cb->flashScreen(pimpl->player, delay);
            }
            break;

        case SERVER_SET_CURRENT_ROOM:
            {
                const int room = buf.readVarInt();
                int width, height;
                ReadRoomCoord(buf, width, height);
                if (dungeon_view) dungeon_view->setCurrentRoom(room, width, height);
            }
            break;

        case SERVER_ADD_ENTITY:
            {
                const int id = buf.readVarInt();
                int x, y;
                ReadRoomCoord(buf, x, y);
                int ht, facing;
                buf.readNibbles(ht, facing);
                const Anim *anim = pimpl->readAnim(buf);
                const Overlay *overlay = pimpl->readOverlay(buf);
                int af, z;
                buf.readNibbles(af, z);
                const MotionType motion_type = MotionType(z >> 1);
                const bool ainvuln = ((z & 1) != 0);
                const int atz_diff = af == 0 ? 0 : buf.readShort();
                const int cur_ofs = buf.readUshort();
                const int motion_time_remaining = motion_type == MT_NOT_MOVING ? 0 : buf.readUshort();
                const std::string name = buf.readString();
                if (dungeon_view) dungeon_view->addEntity(id, x, y, MapHeight(ht), MapDirection(facing),
                                                          anim, overlay, af, atz_diff, ainvuln,
                                                          cur_ofs, motion_type, motion_time_remaining, name);
            }
            break;

        case SERVER_RM_ENTITY:
            {
                const int id = buf.readVarInt();
                if (dungeon_view) dungeon_view->rmEntity(id);
            }
            break;

        case SERVER_REPOSITION_ENTITY:
            {
                const int id = buf.readVarInt();
                int x, y;
                ReadRoomCoord(buf, x, y);
                if (dungeon_view) dungeon_view->repositionEntity(id, x, y);
            }
            break;

        case SERVER_MOVE_ENTITY:
            {
                const int id = buf.readVarInt();
                int motion_type, missile_mode;
                buf.readNibbles(motion_type, missile_mode);
                const int motion_duration = buf.readUshort();
                if (dungeon_view) dungeon_view->moveEntity(id, MotionType(motion_type), motion_duration, missile_mode!=0);
            }
            break;

        case SERVER_FLIP_ENTITY_MOTION:
            {
                const int id = buf.readVarInt();
                const int initial_delay = buf.readUshort();
                const int motion_duration = buf.readUshort();
                if (dungeon_view) dungeon_view->flipEntityMotion(id, initial_delay, motion_duration);
            }
            break;

        case SERVER_SET_ANIM_DATA:
            {
                const int id = buf.readVarInt();
                const Anim *anim = pimpl->readAnim(buf);
                const Overlay *overlay = pimpl->readOverlay(buf);
                int af, z;
                buf.readNibbles(af, z);
                const bool ainvuln = ((z & 2) != 0);
                const bool currently_moving = ((z & 1) != 0);
                const int atz_diff = buf.readShort();
                if (dungeon_view) dungeon_view->setAnimData(id, anim, overlay, af, atz_diff, ainvuln, currently_moving);
            }
            break;

        case SERVER_SET_FACING:
            {
                const int id = buf.readVarInt();
                const MapDirection facing = MapDirection(buf.readUbyte());
                if (dungeon_view) dungeon_view->setFacing(id, facing);
            }
            break;

        case SERVER_SET_SPEECH_BUBBLE:
            {
                const int id = buf.readVarInt();
                const bool show = (buf.readUbyte() != 0);
                if (dungeon_view) dungeon_view->setSpeechBubble(id, show);
            }
            break;

        case SERVER_CLEAR_TILES:
            {
                int x, y;
                ReadRoomCoord(buf, x, y);
                if (dungeon_view) dungeon_view->clearTiles(x, y, true);
            }
            break;

        case SERVER_SET_TILE:
            {
                int x, y;
                ReadRoomCoord(buf, x, y);
                int depth;
                bool cc_set;
                ReadTileInfo(buf, depth, cc_set);
                const Graphic *graphic = pimpl->readGraphic(buf);
                boost::shared_ptr<ColourChange> cc;
                if (cc_set) cc.reset(new ColourChange(buf));
                if (dungeon_view) dungeon_view->setTile(x, y, depth, graphic, cc, true);
            }
            break;

        case SERVER_SET_ITEM:
            {
                int x, y;
                ReadRoomCoord(buf, x, y);
                const Graphic * graphic = pimpl->readGraphic(buf);
                if (dungeon_view) dungeon_view->setItem(x, y, graphic, true);
            }
            break;

        case SERVER_PLACE_ICON:
            {
                int x, y;
                ReadRoomCoord(buf, x, y);
                const Graphic * graphic = pimpl->readGraphic(buf);
                const int duration = buf.readUshort();
                if (dungeon_view) dungeon_view->placeIcon(x, y, graphic, duration);
            }
            break;

        case SERVER_FLASH_MESSAGE:
            {
                const std::string msg = buf.readString();
                const int ntimes = buf.readUbyte();
                if (dungeon_view) dungeon_view->flashMessage(msg, ntimes);
            }
            break;

        case SERVER_CANCEL_CONTINUOUS_MESSAGES:
            if (dungeon_view) dungeon_view->cancelContinuousMessages();
            break;

        case SERVER_ADD_CONTINUOUS_MESSAGE:
            {
                const std::string msg = buf.readString();
                if (dungeon_view) dungeon_view->addContinuousMessage(msg);
            }
            break;

        case SERVER_SET_MAP_SIZE:
            {
                const int width = buf.readUbyte();
                const int height = buf.readUbyte();
                if (mini_map) mini_map->setSize(width, height);
            }
            break;

        case SERVER_SET_COLOUR:
            {
                const int nruns = buf.readVarInt();
                for (int i = 0; i < nruns; ++i) {
                    const int start_x = buf.readUbyte();
                    const int y = buf.readUbyte();
                    const int nx = buf.readUbyte();
                    for (int x = start_x; x < start_x + nx; ++x) {
                        const MiniMapColour col = MiniMapColour(buf.readUbyte());
                        if (mini_map) mini_map->setColour(x, y, col);
                    }
                }
            }
            break;

        case SERVER_WIPE_MAP:
            if (mini_map) mini_map->wipeMap();
            break;

        case SERVER_MAP_KNIGHT_LOCATION:
            {
                const int knight_id = buf.readUbyte();
                const int x = buf.readUbyte();
                if (x == 255) {
                    if (mini_map) mini_map->mapKnightLocation(knight_id, -1, -1);
                } else {
                    const int y = buf.readUbyte();
                    if (mini_map) mini_map->mapKnightLocation(knight_id, x, y);
                }
            }
            break;

        case SERVER_MAP_ITEM_LOCATION:
            {
                const int x = buf.readUbyte();
                const int y = buf.readUbyte();
                const bool flag = buf.readUbyte() != 0;
                if (mini_map) mini_map->mapItemLocation(x, y, flag);
            }
            break;

        case SERVER_SET_BACKPACK:
            {
                const int slot = buf.readUbyte();
                const Graphic * gfx = pimpl->readGraphic(buf);
                const Graphic * ovr = pimpl->readGraphic(buf);
                const int no_carried = buf.readUbyte();
                const int no_max = buf.readUbyte();
                if (status_display) status_display->setBackpack(slot, gfx, ovr, no_carried, no_max);
            }
            break;

        case SERVER_ADD_SKULL:
            if (status_display) status_display->addSkull();
            break;

        case SERVER_SET_HEALTH:
            {
                const int h = buf.readVarInt();
                if (status_display) status_display->setHealth(h);
            }
            break;

        case SERVER_SET_POTION_MAGIC:
            {
                const int x = buf.readUbyte();
                if (status_display) status_display->setPotionMagic(PotionMagic(x & 0x7f), (x & 0x80) != 0);
            }
            break;

        case SERVER_SWITCH_PLAYER:
            {
                const int new_player = buf.readUbyte();
                if (new_player >= pimpl->ndisplays) throw ProtocolError("display number out of range");
                pimpl->player = new_player;
                if (knights_cb) {
                    dungeon_view = &knights_cb->getDungeonView(new_player);
                    mini_map = &knights_cb->getMiniMap(new_player);
                    status_display = &knights_cb->getStatusDisplay(new_player);
                }
            }
            break;

        case SERVER_TIME_REMAINING:
            {
                const int milliseconds = buf.readVarInt();
                if (client_cb) client_cb->setTimeRemaining(milliseconds);
            }
            break;

        case SERVER_READY_TO_END:
            {
                const std::string player = buf.readString();
                if (client_cb) client_cb->playerIsReadyToEnd(player);
            }
            break;

        case SERVER_EXTENDED_MESSAGE:
            {
                int ext_code = buf.readVarInt();
                int payload_length = buf.readUshort();

                size_t pos_before_payload = buf.getPos();

                // See if we want to parse any of the payload contents.
                switch (ext_code) {
                case SERVER_EXT_SET_QUEST_ICONS:
                    {
                        int num_icons = buf.readUbyte();
                        std::vector<StatusDisplay::QuestIcon> icons;
                        icons.reserve(num_icons);
                        for (int i = 0; i < num_icons; ++i) {
                            StatusDisplay::QuestIcon qi;
                            qi.sort = buf.readUshort();
                            qi.msg = buf.readString();
                            qi.complete = buf.readUbyte() != 0;
                            icons.push_back(qi);
                        }
                        if (status_display) status_display->setQuestIcons(icons);
                    }
                    break;
                }

                // check that we read the entire payload
                size_t pos_now = buf.getPos();
                size_t should_be = pos_before_payload + size_t(payload_length);
                
                // If we did not read all of the payload, then skip over the unread payload 
                // bytes. (This is for forwards compatibility.)
                while (pos_now < should_be) {
                    buf.readUbyte();
                    ++pos_now;
                }

                // If we read MORE than the payload then something has gone wrong.
                if (pos_now != should_be) {
                    throw ProtocolError("Bad EXT code from server");
                }
            }
            break;
            
        default:
            throw ProtocolError("Unknown message code from server");
        }
    }
}

void KnightsClient::getOutputData(std::vector<ubyte> &data)
{
    data.swap(pimpl->out);
    pimpl->out.clear();
}

void KnightsClient::connectionClosed()
{
    if (pimpl->client_callbacks) pimpl->client_callbacks->connectionLost();
}

void KnightsClient::connectionFailed()
{
    if (pimpl->client_callbacks) pimpl->client_callbacks->connectionFailed();
}

void KnightsClient::setPlayerNameAndControls(const std::string &name, bool action_bar_ctrls)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SET_PLAYER_NAME);
    buf.writeString(name);
    buf.writeUbyte(CLIENT_SET_ACTION_BAR_CONTROLS);
    buf.writeUbyte(action_bar_ctrls ? 1 : 0);
}

void KnightsClient::joinGame(const std::string &game_name)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_JOIN_GAME);
    buf.writeString(game_name);
}

void KnightsClient::joinGameSplitScreen(const std::string &game_name)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_JOIN_GAME_SPLIT_SCREEN);
    buf.writeString(game_name);
}

void KnightsClient::leaveGame()
{
    pimpl->out.push_back(CLIENT_LEAVE_GAME);
}

void KnightsClient::sendChatMessage(const std::string &msg)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_CHAT);
    buf.writeString(msg);
}

void KnightsClient::setReady(bool ready)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SET_READY);
    buf.writeUbyte(ready);
}

void KnightsClient::setHouseColour(int x)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SET_HOUSE_COLOUR);
    buf.writeUbyte(x);
}

void KnightsClient::setObsFlag(bool x)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SET_OBS_FLAG);
    buf.writeUbyte(x ? 1 : 0);
}

void KnightsClient::setMenuSelection(const std::string &key, int value)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SET_MENU_SELECTION);
    buf.writeString(key);
    buf.writeVarInt(value);
}

void KnightsClient::randomQuest()
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_RANDOM_QUEST);
}

void KnightsClient::finishedLoading()
{
    pimpl->out.push_back(CLIENT_FINISHED_LOADING);
}

void KnightsClient::sendPassword(const std::string &password)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SEND_PASSWORD);
    buf.writeString(password);
}

void KnightsClient::sendControl(int plyr, const UserControl *ctrl)
{
    int id = 0;
    if (ctrl) {
        id = ctrl->getID();
        if (id < 1 || id > 127) throw ProtocolError("control ID out of range (sendControl)");
    }
    if (plyr < 0 || plyr > 1) throw ProtocolError("player number out of range (sendControl)");

    // optimization: do not send "repeats" of continuous controls.
    if (ctrl == 0 || ctrl->isContinuous()) {
        if (ctrl == pimpl->last_cts_ctrl[plyr]) return;
        pimpl->last_cts_ctrl[plyr] = ctrl;
    } else {
        // this is a "discrete" control so make sure we re-send the next continuous control after it...
        pimpl->last_cts_ctrl[plyr] = 0;
    }
    
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_SEND_CONTROL);

    if (plyr == 1) id += 128;
    buf.writeUbyte(id);
}

void KnightsClient::requestSpeechBubble(bool show)
{
    Coercri::OutputByteBuf buf(pimpl->out);
    buf.writeUbyte(CLIENT_REQUEST_SPEECH_BUBBLE);
    buf.writeUbyte(show ? 1 : 0);
}

void KnightsClient::readyToEnd()
{
    pimpl->out.push_back(CLIENT_READY_TO_END);
}

void KnightsClient::requestQuit()
{
    pimpl->out.push_back(CLIENT_QUIT);
}

void KnightsClient::setPauseMode(bool p)
{
    pimpl->out.push_back(CLIENT_SET_PAUSE_MODE);
    pimpl->out.push_back(p ? 1 : 0);
}

void KnightsClientImpl::receiveConfiguration(Coercri::InputByteBuf &buf)
{
    client_config.reset(new ClientConfig);  // wipe out the old client config if there is one.
    
    const int n_graphics = buf.readVarInt();
    client_config->graphics.reserve(n_graphics);
    for (int i = 0; i < n_graphics; ++i) {
        client_config->graphics.push_back(new Graphic(i+1, buf));
    }

    const int n_anims = buf.readVarInt();
    client_config->anims.reserve(n_anims);
    for (int i = 0; i < n_anims; ++i) {
        client_config->anims.push_back(new Anim(i+1, buf, client_config->graphics));
    }

    const int n_overlays = buf.readVarInt();
    client_config->overlays.reserve(n_overlays);
    for (int i = 0; i < n_overlays; ++i) {
        client_config->overlays.push_back(new Overlay(i+1, buf, client_config->graphics));
    }

    const int n_sounds = buf.readVarInt();
    client_config->sounds.reserve(n_sounds);
    for (int i = 0; i < n_sounds; ++i) {
        client_config->sounds.push_back(new Sound(i+1, buf));
    }

    const int n_standard_controls = buf.readVarInt();
    client_config->standard_controls.reserve(n_standard_controls);
    for (int i = 0; i < n_standard_controls; ++i) {
        client_config->standard_controls.push_back(new UserControl(i+1, buf, client_config->graphics));
    }
    
    const int n_other_controls = buf.readVarInt();
    client_config->other_controls.reserve(n_other_controls);
    for (int i = 0; i < n_other_controls; ++i) {
        client_config->other_controls.push_back(new UserControl(i+1+n_standard_controls, buf, client_config->graphics));
    }

    client_config->menu.reset(new Menu(buf));

    client_config->approach_offset = buf.readVarInt();
}

const Graphic * KnightsClientImpl::readGraphic(Coercri::InputByteBuf &buf) const
{
    const int id = buf.readVarInt();
    if (id < 0 || id > int(client_config->graphics.size())) throw ProtocolError("graphic id out of range");
    if (id == 0) return 0;
    return client_config->graphics.at(id-1);
}

const Anim * KnightsClientImpl::readAnim(Coercri::InputByteBuf &buf) const
{
    const int id = buf.readVarInt();
    if (id < 0 || id > int(client_config->anims.size())) throw ProtocolError("anim id out of range");
    if (id == 0) return 0;
    return client_config->anims.at(id-1);
}

const Overlay * KnightsClientImpl::readOverlay(Coercri::InputByteBuf &buf) const
{
    const int id = buf.readVarInt();
    if (id < 0 || id > int(client_config->overlays.size())) throw ProtocolError("overlay id out of range");
    if (id == 0) return 0;
    return client_config->overlays.at(id-1);
}

const Sound * KnightsClientImpl::readSound(Coercri::InputByteBuf &buf) const
{
    const int id = buf.readVarInt();
    if (id < 0 || id > int(client_config->sounds.size())) throw ProtocolError("sound id out of range");
    if (id == 0) return 0;
    return client_config->sounds.at(id-1);
}

const UserControl * KnightsClientImpl::getControl(int id) const
{
    if (id == 0) return 0;
    if (id <= int(client_config->standard_controls.size())) {
        return client_config->standard_controls.at(id-1);
    } else if (id <= int(client_config->standard_controls.size() + client_config->other_controls.size())) {
        return client_config->other_controls.at(id - 1 - client_config->standard_controls.size()); 
    } else {
        throw ProtocolError("control id out of range");
    }
}
