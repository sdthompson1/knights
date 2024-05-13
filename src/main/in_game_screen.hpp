/*
 * in_game_screen.hpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2024.
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

/*
 * InGameScreen
 * --- runs the game
 * Goes back to MenuScreen when done
 *
 */

#ifndef IN_GAME_SCREEN_HPP
#define IN_GAME_SCREEN_HPP

#include "screen.hpp"

#include "gfx/window_listener.hpp"  // coercri

#include "boost/scoped_ptr.hpp"

#include <string>
#include <vector>

class ClientConfig;
class Graphic;
class KnightsClient;
class LocalDisplay;
class UserControl;

class GuiPanel2;

class InGameScreen : public Screen, public Coercri::WindowListener {
public:
    InGameScreen(KnightsApp &ka, boost::shared_ptr<KnightsClient> knights_client, 
                 boost::shared_ptr<const ClientConfig> config, int nplayers_,
                 bool deathmatch_mode,
                 const std::vector<UTF8String> &player_names,
                 bool single_player_, bool tutorial);
    virtual bool start(KnightsApp &, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
    virtual ~InGameScreen();
    virtual unsigned int getUpdateInterval();
    virtual void update();
    virtual void draw(Coercri::GfxContext &gc);

    virtual void onMouseMove(int new_x, int new_y);
    virtual void onMouseDown(int x, int y, Coercri::MouseButton m);
    virtual void onMouseUp(int x, int y, Coercri::MouseButton m);
    virtual void onKey(Coercri::KeyEventType type, Coercri::KeyCode kc, Coercri::KeyModifier);
    virtual void onMinimize();
    virtual void onUnminimize();
    virtual void onGainFocus();
    virtual void onLoseFocus();

private:
    void setupDisplay();
    void checkChatFocus();
    
private:
    bool single_player;
    bool tutorial_mode;
    bool pause_mode;
    bool window_minimized;
    bool auto_mouse;
    bool speech_bubble_flag;

    boost::shared_ptr<LocalDisplay> display;

    bool prevent_drawing;

    boost::shared_ptr<Coercri::Window> window;

    KnightsApp &knights_app;
    boost::shared_ptr<KnightsClient> knights_client;
    boost::shared_ptr<const ClientConfig> client_config;

    boost::scoped_ptr<gcn::Container> container;

    // held across initialization only:
    std::vector<UTF8String> init_player_names;
    int init_nplayers;
    bool deathmatch_mode;

    // mouse handling. (for action bar)
    int mx, my;
    bool mleft, mright;

    unsigned int focus_timer;

    // chat keys
    Coercri::KeyCode global_chat_key, team_chat_key;
    bool waiting_to_focus_chat;
    bool waiting_to_chat_all;
    std::string stored_chat_field_contents;
};

#endif
