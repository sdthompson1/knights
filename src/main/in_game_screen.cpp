/*
 * in_game_screen.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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

#include "client_config.hpp"
#include "config_map.hpp"
#include "controller.hpp"
#include "error_screen.hpp"
#include "game_manager.hpp"
#include "gfx_manager.hpp"
#include "in_game_screen.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "local_display.hpp"
#include "menu.hpp"
#include "menu_selections.hpp"
#include "menu_screen.hpp"
#include "my_exceptions.hpp"
#include "options.hpp"

// coercri
#include "gfx/rectangle.hpp"

namespace {
    std::vector<int> GetObsPlayers(int curr_obs_plyr, int nplyrs)
    {
        std::vector<int> x(2);
        x[0] = curr_obs_plyr;
        x[1] = x[0] + 1;
        if (x[1] == nplyrs) x[1] = 0;
        return x;
    }
}

InGameScreen::InGameScreen(KnightsApp &ka, boost::shared_ptr<KnightsClient> kc, 
                           boost::shared_ptr<const ClientConfig> cfg, int nplayers_,
                           bool deathmatch_mode_,
                           const std::vector<std::string> &names,
                           bool sgl_plyr, bool tut)
    : single_player(sgl_plyr),
      tutorial_mode(tut),
      pause_mode(false),
      window_minimized(false),
      auto_mouse(false),
      speech_bubble_flag(false),
      init_nplayers(nplayers_),
      deathmatch_mode(deathmatch_mode_),
      prevent_drawing(false),
      knights_app(ka),
      knights_client(kc),
      client_config(cfg),
      init_player_names(names),
      curr_obs_player(0),
      mx(0), my(0), mleft(false), mright(false),
      focus_timer(0)
{
}

InGameScreen::~InGameScreen()
{
    // reset 'guiparams' of the chat list
    knights_app.getGameManager().getChatList().setGuiParams((Coercri::Font*)0, 999);

    // Delete LocalDisplay and remove it from the callback mechanism
    knights_client->setKnightsCallbacks(0);
    display.reset();
    
    if (window) {
        // Make sure the mouse pointer is shown after leaving in-game screen
        window->showMousePointer(true);
        // Remove myself from the window if needed
        window->rmWindowListener(this);
    }
}

unsigned int InGameScreen::getUpdateInterval()
{
    return 1000 / static_cast<unsigned int>(knights_app.getConfigMap().getInt("fps"));
}

void InGameScreen::setupDisplay()
{
    // Set up menu strings.
    boost::shared_ptr<vector<pair<string, string> > > menu_strings(new vector<pair<string, string> >);
    knights_app.getGameManager().getMenuStrings(*menu_strings);
    
    // Create displays / callbacks.
    const Controller * left_controller = 0;
    const Controller * right_controller = 0;
    if (init_player_names.empty()) {
        if (init_nplayers == 1) {
            left_controller = &knights_app.getNetGameController();
        } else {
            left_controller = &knights_app.getLeftController();
            right_controller = &knights_app.getRightController();
        }
    }
    
    const Options & options = knights_app.getOptions();
    
    display.reset(new LocalDisplay(knights_app.getConfigMap(),
                                   client_config->approach_offset,
                                   knights_app.getWinnerImage(),
                                   knights_app.getLoserImage(),
                                   knights_app.getSpeechBubble(),
                                   knights_app.getMenuGfxCentre(),
                                   knights_app.getMenuGfxEmpty(),
                                   knights_app.getMenuGfxHighlight(),
                                   knights_app.getPotionRenderer(),
                                   knights_app.getSkullRenderer(),
                                   menu_strings,
                                   client_config->standard_controls,
                                   left_controller,
                                   right_controller,
                                   init_nplayers,
                                   deathmatch_mode,
                                   init_player_names,
                                   knights_app.getTimer(),
                                   knights_app.getGameManager().getChatList(),
                                   knights_app.getGameManager().getIngamePlayerList(),
                                   *knights_client,
                                   *container,
                                   Coercri::RawKeyName(options.ctrls[2][0]), Coercri::RawKeyName(options.ctrls[2][1]),
                                   Coercri::RawKeyName(options.ctrls[2][2]), Coercri::RawKeyName(options.ctrls[2][3]),
                                   Coercri::RawKeyName(options.ctrls[2][4]), Coercri::RawKeyName(options.ctrls[2][5]),
                                   single_player,
                                   tutorial_mode,
                                   knights_app.getOptions().action_bar_tool_tips));
    knights_client->setKnightsCallbacks(display.get());

    init_nplayers = 0;
    init_player_names.clear();
}

void InGameScreen::draw(Coercri::GfxContext &gc)
{
    const std::vector<std::string> & player_names = display->getPlayerNamesForObsMode();
    const int nplayers = display->getNPlayers();
    
    GfxManager &gm = knights_app.getGfxManager();
    
    const bool is_split_screen = nplayers >= 2 && player_names.empty();
    const bool is_gameplay_paused = (pause_mode && (is_split_screen || single_player));

    display->recalculateTime(is_gameplay_paused,
                             knights_app.getGameManager().getTimeRemaining());

    std::vector<int> plyrs_to_draw;
    if (player_names.empty()) {
        for (int i = 0; i < 2; ++i) plyrs_to_draw.push_back(i);
    } else {
        plyrs_to_draw = GetObsPlayers(curr_obs_player, player_names.size());
    }

    if (nplayers >= 2) {
        if (player_names.empty()) {

            // Split screen mode
            const int w1 = gc.getWidth()/2;
            const int w2 = w1;
            const int h = gc.getHeight();
            if (pause_mode) {
                display->drawPauseDisplay(gc, gm, 0, 0, w1, h, true, false);
                display->drawPauseDisplay(gc, gm, w1, 0, w2, h, true, false);
            } else {
                display->draw(gc, gm, plyrs_to_draw[0], 0, 0, w1, h, -1);
                display->draw(gc, gm, plyrs_to_draw[1], w1, 0, w2, h, 1);
            }
            
        } else {

            // Observer mode
            const int h2 = std::max(200, gc.getHeight()/6);
            const int h1 = std::max(0, gc.getHeight() - h2);
            const int w1 = gc.getWidth()/2;
            const int w2 = w1;
            const int actual_height = display->draw(gc, gm, plyrs_to_draw[0], 0, 0, w1, h1, 0);
            display->draw(gc, gm, plyrs_to_draw[1], w1, 0, w2, h1, 0);
            display->updateGui(gm, 0, actual_height, gc.getWidth(), gc.getHeight() - actual_height, true);
            if (pause_mode) {
                display->drawPauseDisplay(gc, gm, 0, 0, gc.getWidth(), gc.getHeight(), false, true);
            }
        }
    } else {
        
        // Network game or single player mode.
        const int w1 = gc.getWidth()*11/20;  // allocate slightly more than half to the dungeon view.
        const int w2 = gc.getWidth() - w1;
        const int h = gc.getHeight();
        
        // Draw main screen area
        display->draw(gc, gm, plyrs_to_draw[0], 0, 0, w1, h, 0);

        // Draw gui area
        display->updateGui(gm, w1, 0, w2, h, false);

        // Draw pause display. (Single player mode => paused, Network game => not paused.)
        if (pause_mode) {
            display->drawPauseDisplay(gc, gm, 0, 0, gc.getWidth(), gc.getHeight(), single_player, true);
        }
    }
}

void InGameScreen::update()
{
    if (!container) return;  // just in case

    if (prevent_drawing) return;

    const std::vector<std::string> & player_names = display->getPlayerNamesForObsMode();
    const int nplayers = display->getNPlayers();    

    // Read controllers
    if (player_names.empty()) { // NOT in observer mode
        ASSERT(nplayers == 1 || nplayers == 2);
        knights_client->sendControl(0, display->readControl(0, mx, my, mleft, mright));
        if (nplayers == 2) knights_client->sendControl(1, display->readControl(1, -1, -1, false, false));
    }

    // Make sure sounds get played
    std::vector<int> players = GetObsPlayers(curr_obs_player, player_names.size());
    display->playSounds(knights_app.getSoundManager(), players);

    // Save chat field contents (Trac #12)
    // Also: speech bubble (Trac #11)
    const std::string chat_field_contents = display->getChatFieldContents();
    knights_app.getGameManager().setSavedChat(chat_field_contents);
    if (display->chatFieldSelected() && !speech_bubble_flag) {
        // Show speech bubble if it's not already visible
        speech_bubble_flag = true;
        knights_client->requestSpeechBubble(true);
    } else if (!display->chatFieldSelected() && speech_bubble_flag) {
        speech_bubble_flag = false;
        knights_client->requestSpeechBubble(false);
    }
    
    // force a repaint
    window->invalidateAll();

    if (knights_app.getGameManager().getChatList().isUpdated()) {
        display->setChatUpdated();
    }
}

void InGameScreen::onMouseMove(int new_x, int new_y)
{
    if (auto_mouse) window->showMousePointer(true);
    mx=new_x;my=new_y;
}

void InGameScreen::onMouseDown(int x, int y, Coercri::MouseButton b)
{
    if (focus_timer != 0) {
        const unsigned int time_now = knights_app.getTimer().getMsec();
        if (time_now <= focus_timer) {
            focus_timer = 0;
            return;  // ignore the first click after the window gains focus.
        }
    }
    
    if (b == Coercri::MB_LEFT) mleft = true;
    else if (b == Coercri::MB_RIGHT) mright = true;

    if (display->isGameOver() && !display->getReadyFlag() && 
    !prevent_drawing && b == Coercri::MB_LEFT) {
        knights_client->readyToEnd();
        display->setReadyFlag();
    }
}

void InGameScreen::onMouseUp(int x, int y, Coercri::MouseButton b)
{
    if (b == Coercri::MB_LEFT) mleft = false;
    else if (b == Coercri::MB_RIGHT) mright = false;
}

void InGameScreen::onRawKey(bool pressed, Coercri::RawKey rk)
{
    if (auto_mouse) window->showMousePointer(false);
    
    // Keys only work when in game
    if (prevent_drawing) return;

    const std::vector<std::string> & player_names = display->getPlayerNamesForObsMode();
    const int nplayers = display->getNPlayers();
    
    const bool is_game_over = display->isGameOver();
    const bool escape_pressed = pressed && rk == Coercri::RK_ESCAPE;
    const bool space_pressed = pressed && rk == Coercri::RK_SPACE;
    const bool q_pressed = pressed && rk == Coercri::RK_Q;
    const bool tab_pressed = pressed && rk == Coercri::RK_TAB;

    // TAB => toggle chat mode
    if (tab_pressed) {
        display->toggleChatMode();
    }
    
    // ESC if tutorial active => Cancel all tutorial windows, and unpause
    // SPACE if tutorial active => Cancel current tutorial window, and unpause if there are no more windows.
    if (escape_pressed && display->tutorialActive()) {
        display->clearAllTutorialWindows();
        knights_client->setPauseMode(pause_mode);
        return;
    }
    if (space_pressed && display->tutorialActive()) {
        display->clearTutorialWindow();
        if (!display->tutorialActive()) knights_client->setPauseMode(pause_mode);
    }
    
    // ESC in game => enter/exit "pause mode".
    // (Note this only actually pauses the gameplay in split screen mode...)
    if (escape_pressed) {
        pause_mode = !pause_mode;
        if (nplayers == 2 && player_names.empty()  // split screen mode
        || single_player) {                        // single player mode
            knights_client->setPauseMode(pause_mode || window_minimized); // tell server to pause gameplay
        }
        window->invalidateAll();   // make sure screen gets redrawn!
        return;
    }

    // Q in "pause mode" to request quit
    if (q_pressed && pause_mode) {
        if (!player_names.empty()) {
            // obs mode
            // leave the game
            knights_client->leaveGame();
        } else {
            // normal mode
            // request to quit the game
            knights_app.getGameManager().setSavedChat("");
            knights_client->requestQuit();
        }
        prevent_drawing = true;  // prevents further drawing etc after the game has finished (and gfx deleted!)
        return;
    }

    // Left/Right in Observe mode => shift currently observed player.
    if (player_names.size() > 2 && pressed) {
        if (rk == Coercri::RK_LEFT) {
            --curr_obs_player;
            if (curr_obs_player < 0) curr_obs_player = player_names.size() - 1;
        } else if (rk == Coercri::RK_RIGHT) {
            ++curr_obs_player;
            if (curr_obs_player >= player_names.size()) curr_obs_player = 0;
        }
    }
}

void InGameScreen::onActivate()
{
    const std::vector<std::string> & player_names = display->getPlayerNamesForObsMode();
    const int nplayers = display->getNPlayers();

    window_minimized = false;
    if (nplayers == 2 && player_names.empty()) knights_client->setPauseMode(pause_mode || window_minimized);
}

void InGameScreen::onDeactivate()
{
    const std::vector<std::string> & player_names = display->getPlayerNamesForObsMode();
    const int nplayers = display->getNPlayers();
    
    window_minimized = true;
    if (nplayers == 2 && player_names.empty()) knights_client->setPauseMode(pause_mode || window_minimized);
}

void InGameScreen::onGainFocus()
{
    focus_timer = knights_app.getTimer().getMsec() + 100;
}

void InGameScreen::onLoseFocus()
{
}

bool InGameScreen::start(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    const bool obs_mode = !init_player_names.empty();
    const bool two_player_splitscreen = !obs_mode && init_nplayers > 1;
    const bool fullscreen = app.getOptions().fullscreen;
    
    if (fullscreen && two_player_splitscreen) win->showMousePointer(false);
    auto_mouse = false;   // disabling auto mouse for now, as it behaves a bit strangely on SDL (mouse warps to centre of screen)
    
    win->addWindowListener(this);
    window = win;

    container.reset(new gcn::Container);
    container->setOpaque(false);
    gui.setTop(container.get());

    // we handle the TAB key ourselves, so don't let guichan handle it
    gui.setTabbingEnabled(false);
    
    setupDisplay();
    knights_client->finishedLoading();
    
    return true;
}
