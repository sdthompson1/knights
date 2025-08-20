/*
 * server_callbacks.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2025.
 * Copyright (C) Kalle Marjola, 1994.
 *
 * Knights is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
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

#include "announcement_loc.hpp"
#include "graphic.hpp"
#include "protocol.hpp"
#include "server_callbacks.hpp"
#include "server_dungeon_view.hpp"
#include "server_mini_map.hpp"
#include "server_status_display.hpp"
#include "sound.hpp"
#include "user_control.hpp"

#include <iterator>
#include <limits>

ServerCallbacks::ServerCallbacks(int nplayers)
    : game_over(false), next_observer_num(1), no_err_msgs(0)
{
    pub.resize(nplayers);
    prv.resize(nplayers);
    prev_menu_highlight.resize(nplayers);
    dungeon_view.reserve(nplayers);
    mini_map.reserve(nplayers);
    status_display.reserve(nplayers);
    loser.resize(nplayers);
    
    for (int i = 0; i < nplayers; ++i) {
        boost::shared_ptr<ServerDungeonView> dview(new ServerDungeonView(pub[i]));
        dungeon_view.push_back(dview);

        boost::shared_ptr<ServerMiniMap> mm(new ServerMiniMap(pub[i]));
        mini_map.push_back(mm);

        boost::shared_ptr<ServerStatusDisplay> sdisp(new ServerStatusDisplay(pub[i]));
        status_display.push_back(sdisp);
    }
}

ServerCallbacks::~ServerCallbacks()
{
}

void ServerCallbacks::appendPlayerCmds(int plyr, std::vector<ubyte> &out) const
{
    doAppendPlayerCmds(plyr, out, 0, true);
}

void ServerCallbacks::doAppendPlayerCmds(int plyr, std::vector<ubyte> &out, int observer_num, bool include_private) const
{
    std::copy(pub[plyr].begin(), pub[plyr].end(), std::back_inserter(out));
    if (include_private) std::copy(prv[plyr].begin(), prv[plyr].end(), std::back_inserter(out));
    mini_map[plyr]->appendMiniMapCmds(out);
    dungeon_view[plyr]->appendDungeonViewCmds(observer_num*1000+plyr, out);  // note 'transformed' observer_num
}

void ServerCallbacks::appendObserverCmds(int observer_num, std::vector<ubyte> &out) const
{
    int num_to_observe = pub.size();

    for (int i = 0; i < num_to_observe; ++i) {
        out.push_back(SERVER_SWITCH_PLAYER);
        out.push_back(ubyte(i));
        const size_t prev_size = out.size();
        doAppendPlayerCmds(i, out, observer_num, false);
        if (out.size() == prev_size) {
            // remove the SWITCH_PLAYER cmd, it isn't needed if there
            // was no output for that player
            out.pop_back();
            out.pop_back();
        }
    }
}

void ServerCallbacks::clearCmds()
{
    for (int i = 0; i < int(pub.size()); ++i) {
        pub[i].clear();
        prv[i].clear();
        mini_map[i]->clearMiniMapCmds();
        dungeon_view[i]->clearDungeonViewCmds();
    }
}

int ServerCallbacks::allocObserverNum()
{
    int result = next_observer_num;
    ++next_observer_num;
    if (result >= std::numeric_limits<int>::max() / 1000 - 1) throw UnexpectedError("observer_num overflow!"); // will probably never happen
    return result;
}

void ServerCallbacks::rmObserverNum(int o)
{
    for (int i = 0; i < int(pub.size()); ++i) {
        dungeon_view[i]->rmObserverNum(o*1000+i);
    }
}

DungeonView & ServerCallbacks::getDungeonView(int i)
{
    return *dungeon_view[i];
}

MiniMap & ServerCallbacks::getMiniMap(int i)
{
    return *mini_map[i];
}

StatusDisplay & ServerCallbacks::getStatusDisplay(int i)
{
    return *status_display[i];
}

void ServerCallbacks::playSound(int plyr, const Sound &sound, int frequency)
{
    Coercri::OutputByteBuf buf(pub[plyr]);
    buf.writeUbyte(SERVER_PLAY_SOUND);
    buf.writeVarInt(sound.getID());
    buf.writeVarInt(frequency);
}

void ServerCallbacks::winGame(int plyr)
{
    Coercri::OutputByteBuf buf(pub[plyr]);
    buf.writeUbyte(SERVER_WIN_GAME);
    game_over = true;
    winner_num = plyr;
}

void ServerCallbacks::loseGame(int plyr)
{
    Coercri::OutputByteBuf buf(pub[plyr]);
    buf.writeUbyte(SERVER_LOSE_GAME);

    loser[plyr] = true;
    bool all_lost = true;
    for (int i = 0; i < int(loser.size()); ++i) {
        if (!loser[i]) {
            all_lost = false;
            break;
        }
    }
    if (all_lost) {
        game_over = true;
        winner_num = -1;
    }
}

void ServerCallbacks::setAvailableControls(int plyr, const std::vector<std::pair<const UserControl*, bool> > &available_controls)
{
    Coercri::OutputByteBuf buf(prv[plyr]);
    buf.writeUbyte(SERVER_SET_AVAILABLE_CONTROLS);
    buf.writeUbyte(available_controls.size());
    for (std::vector<std::pair<const UserControl*, bool> >::const_iterator it = available_controls.begin();
    it != available_controls.end(); ++it) {
        int x = it->first->getID();
        if (x <= 0 || x >= 128) throw UnexpectedError("Control ID out of range");
        if (it->second) x += 128;
        buf.writeUbyte(x);
    }
}

void ServerCallbacks::setMenuHighlight(int plyr, const UserControl *highlight)
{
    if (prev_menu_highlight[plyr] == highlight) return;
    prev_menu_highlight[plyr] = highlight;

    Coercri::OutputByteBuf buf(prv[plyr]);
    buf.writeUbyte(SERVER_SET_MENU_HIGHLIGHT);
    buf.writeUbyte(highlight ? highlight->getID() : 0);
}

void ServerCallbacks::flashScreen(int plyr, int delay)
{
    Coercri::OutputByteBuf buf(pub[plyr]);
    buf.writeUbyte(SERVER_FLASH_SCREEN);
    buf.writeVarInt(delay);
}

void ServerCallbacks::gameMsgRaw(int plyr, const Coercri::UTF8String &msg, bool is_err)
{
    gameMsgImpl(plyr, msg, LocalKey(), std::vector<LocalParam>(), is_err);
}

void ServerCallbacks::gameMsgLoc(int plyr, const LocalKey &key, const std::vector<LocalParam> &params, bool is_err)
{
    gameMsgImpl(plyr, UTF8String(), key, params, is_err);
}

void ServerCallbacks::gameMsgImpl(int plyr, const Coercri::UTF8String &msg, const LocalKey &key, const std::vector<LocalParam> &params, bool is_err)
{
    const int MAX_ERRORS = 50;

    if (is_err) {
        ++no_err_msgs;
        if (no_err_msgs > MAX_ERRORS) return;
    }
    
    // convert to an ANNOUNCEMENT
    for (int p = 0; p < int(pub.size()); ++p) {
        if (p == plyr || plyr < 0) {   // plyr < 0 means "send to all players"

            // First msg goes to 'pub', rest go to 'prv', this prevents duplicate messages
            // for observers (Trac #36).
            // NOTE: This can screw up the order of messages, because private messages always come 
            // after public. No good solution for that at the moment.
            Coercri::OutputByteBuf buf(
                (plyr < 0 && p > 0) ? prv[p] : pub[p]
            );

            if (is_err) {
                buf.writeUbyte(SERVER_EXTENDED_MESSAGE);
                buf.writeVarInt(SERVER_EXT_NEXT_ANNOUNCEMENT_IS_ERROR);
                buf.writeUshort(0);
            }

            if (key == LocalKey()) {
                buf.writeUbyte(SERVER_ANNOUNCEMENT_RAW);
                buf.writeString(msg.asUTF8());
            } else {
                WriteAnnouncementLoc(buf, key, params);
            }
        }
    }

    if (is_err && no_err_msgs == MAX_ERRORS) {
        gameMsgLoc(-1, LocalKey("too_many_errors"), std::vector<LocalParam>(), false);
    }
}

void ServerCallbacks::popUpWindow(const std::vector<TutorialWindow> &windows)
{
    // Tutorial only applies to one-player games
    Coercri::OutputByteBuf buf(pub[0]);
    buf.writeUbyte(SERVER_POP_UP_WINDOW);
    buf.writeVarInt(windows.size());
    for (std::vector<TutorialWindow>::const_iterator it = windows.begin(); it != windows.end(); ++it) {
        buf.writeString(it->title_latin1);
        buf.writeString(it->msg_latin1);
        buf.writeVarInt(it->popup ? 1 : 0);
        buf.writeVarInt(it->gfx.size());
        ASSERT(it->cc.size() == it->gfx.size());
        for (int i = 0; i < int(it->gfx.size()); ++i) {
            buf.writeVarInt(it->gfx[i] ? it->gfx[i]->getID() : 0);
            it->cc[i].serialize(buf);
        }
    }
}

void ServerCallbacks::onElimination(int player_num)
{
    players_to_put_into_obs_mode.push_back(player_num);
}

void ServerCallbacks::disableView(int player_num)
{
    Coercri::OutputByteBuf buf(pub[player_num]);
    buf.writeUbyte(SERVER_EXTENDED_MESSAGE);
    buf.writeVarInt(SERVER_EXT_DISABLE_VIEW);
    buf.writeUshort(0);
}

std::vector<int> ServerCallbacks::getPlayersToPutIntoObsMode()
{
    std::vector<int> result = players_to_put_into_obs_mode;
    players_to_put_into_obs_mode.clear();
    return result;
}

void ServerCallbacks::prepareForCatchUp(int i)
{
    dungeon_view[i]->rmObserverNum(i);
    mini_map[i]->prepareForCatchUp();
}
