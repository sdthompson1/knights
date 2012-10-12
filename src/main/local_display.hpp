/*
 * local_display.hpp
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
 * In-game display. Can handle both split screen and single screen
 * modes. Implements KnightsCallbacks.
 *
 */

#ifndef LOCAL_DISPLAY_HPP
#define LOCAL_DISPLAY_HPP

#include "action_bar.hpp"
#include "knights_callbacks.hpp"
#include "map_support.hpp"
#include "tutorial_window.hpp"

// coercri stuff
#include "gcn/cg_font.hpp"
#include "gfx/gfx_context.hpp"
#include "timer/timer.hpp"

#include "boost/scoped_ptr.hpp"

class ActionBar;
class ChatList;
class ConfigMap;
class Controller;
class GfxManager;
class Graphic;
class GuiPanel2;
class LocalDungeonView;
class LocalMiniMap;
class LocalStatusDisplay;
class PotionRenderer;
class SkullRenderer;
class SoundManager;
class TabFont;
class TitleBlock;

class LocalDisplay : public KnightsCallbacks,
                     public gcn::ActionListener,
                     public gcn::FocusListener,
                     public gcn::MouseListener
{
public:
    LocalDisplay(const ConfigMap &config_map_,
                 int approach_offset,
                 const Graphic *winner_image_,
                 const Graphic *loser_image_,
                 const Graphic *speech_bubble_,
                 const Graphic *menu_gfx_centre,
                 const Graphic *menu_gfx_empty,
                 const Graphic *menu_gfx_highlight,
                 const PotionRenderer *potion_renderer,
                 const SkullRenderer *skull_renderer,
                 boost::shared_ptr<std::vector<std::pair<std::string,std::string> > > menu_strings_,
                 const std::vector<const UserControl*> &standard_controls_,
                 const Controller *ctrlr1,
                 const Controller *ctrlr2,
                 int nplyrs,
                 bool deathmatch_mode,
                 const std::vector<std::string> &player_names_,
                 Coercri::Timer &timer,
                 ChatList &chat_list_,
                 ChatList &ingame_player_list_,
                 ChatList &quest_rqmts_list_,
                 KnightsClient &knights_client_,
                 gcn::Container &container_,
                 const std::string &up_key,
                 const std::string &down_key,
                 const std::string &left_key,
                 const std::string &right_key,
                 const std::string &action_key,
                 const std::string &suicide_key,
                 bool sgl_plyr,
                 bool tutorial,
                 bool tool_tips,
                 const std::string &chat_keys);
    ~LocalDisplay();

    //
    // functions specific to this class
    //

    // should be called before each call to drawNormal, drawSplitScreen or drawObs.
    // remaining = time left until end of game (milliseconds) or <0 for no time limit
    void recalculateTime(bool is_paused, int remaining);

    // routines to draw the in-game screen (dungeon view, status display etc.)
    // return value = actual height used.
    int drawNormal(Coercri::GfxContext &gc, GfxManager &gm,
                   int vp_x, int vp_y, int vp_width, int vp_height);
    int drawSplitScreen(Coercri::GfxContext &gc, GfxManager &gm,
                        int vp_x, int vp_y, int vp_width, int vp_height);
    int drawObs(Coercri::GfxContext &gc, GfxManager &gm,
                int vp_x, int vp_y, int vp_width, int vp_height);
    
    // cycle through the available players. used with drawObs.
    // (used to implement left/right arrow keys in observer mode.)
    void cycleObsPlayer(int delta);
    
    // draw the quest info / press q to quit screen.
    // is_paused => say "GAME PAUSED" instead of "KNIGHTS"
    // blend => draw a blended black rectangle behind the pause display.
    void drawPauseDisplay(Coercri::GfxContext &gc, GfxManager &gm, 
                          int vp_x, int vp_y, int vp_width, int vp_height,
                          bool is_paused, bool blend);
    
    // update the in-game gui to use the given viewport.
    // should be called when drawing each frame.
    void updateGui(GfxManager &gm, int vp_x, int vp_y, int vp_width, int vp_height, bool horiz_split);

    // hide the in game gui temporarily
    // call updateGui to show it again
    void hideGui();
    
    // play all queued sounds, then clear the sound queue.
    // Only sounds for currently-observed players are played.
    void playSounds(SoundManager &sm);

    // This routine reads the joystick/controller, and converts this
    // information to a UserControl. It also activates the on-screen
    // menu (when fire is held down) if necessary.
    const UserControl * readControl(int plyr, int mx, int my, bool mleft, bool mright);

    // Tell us if anyone has won yet. (ie has winGame() or loseGame() been called yet.)
    bool isGameOver() const {
        for (int p = 0; p < nplayers; ++p) {
            if (won[p] || lost[p]) return true;
        }
        return false;
    }

    void setReadyFlag() { ready_msg_sent = true; }
    bool getReadyFlag() const { return ready_msg_sent; }

    void setChatUpdated() { chat_updated = true; }


    void action(const gcn::ActionEvent &event);
    void focusGained(const gcn::Event &event);
    void focusLost(const gcn::Event &event);
    void mouseClicked(gcn::MouseEvent &event);
    
    bool tutorialActive() const { return !tutorial_popups.empty(); }
    void clearTutorialWindow();
    void clearAllTutorialWindows() { while (tutorialActive()) clearTutorialWindow(); }

    int getNPlayers() const { return nplayers; }
    const std::vector<std::string> & getPlayerNamesForObsMode() const { return names; }

    std::string getChatFieldContents() const;
    bool chatFieldSelected() const;
    void toggleChatMode(bool global);
    
    
    //
    // functions overridden from KnightsCallbacks
    //
    
    DungeonView & getDungeonView(int plyr);
    MiniMap & getMiniMap(int plyr);
    StatusDisplay & getStatusDisplay(int plyr);

    void playSound(int plyr, const Sound &sound, int frequency);
    
    void winGame(int plyr);
    void loseGame(int plyr);
    
    void setAvailableControls(int plyr, const std::vector<std::pair<const UserControl*, bool> > &available_controls);
    void setMenuHighlight(int plyr, const UserControl *highlight);
    
    void flashScreen(int plyr, int delay);

    void gameMsg(int plyr, const std::string &msg, bool is_err);

    void popUpWindow(const std::vector<TutorialWindow> &windows);
    void onElimination(int) { }
    void disableView(int player_num);
    void goIntoObserverMode(int nplayers, const std::vector<std::string> &names);
    
private:
    void initialize(int nplayers, const std::vector<std::string> &names, 
                    const Graphic *, const Graphic *, const Graphic *, const Graphic *);
    void setMenuControl(int plyr, MapDirection d, const UserControl *ctrl, const UserControl *prev);
    void setupGui(int chat_area_x, int chat_area_y, int chat_area_width, int chat_area_height,
                  int plyr_list_x, int plyr_list_y, int plyr_list_width, int plyr_list_height,
                  int quest_rqmts_x, int quest_rqmts_y, int quest_rqmts_width, int quest_rqmts_height,
                  GfxManager &gm);
    void setupFonts(GfxManager &gm);
    std::string replaceSpecialChars(std::string msg) const;
    void updateTutorialWidget();
    static void setWidgetEnabled(gcn::Widget &, bool);
    void setTimeLimitCaption();
    bool mouseInGuiControl(int mx, int my) const;
    void activateChatField();
    void deactivateChatField();

    // draw the in-game screen for one player within the given viewport.
    // bias = 0 to centre within viewport, -1 to draw left of centre,
    // +1 to draw right of centre (useful for split screen mode).
    // return value = actual height used.
    int draw(Coercri::GfxContext &gc, GfxManager &gm,
             int plyr_num,
             int vp_x, int vp_y, int vp_width, int vp_height,
             int bias);    
    
private:
    const ConfigMap &config_map;
    const int approach_offset;
    const PotionRenderer *potion_renderer;
    const SkullRenderer *skull_renderer;

    // cached config variables
    const int ref_vp_width, ref_vp_height, ref_gutter;
    const int ref_pixels_per_square, dungeon_tiles_x, dungeon_tiles_y;
    const int min_font_size;
    const float ref_font_size;
    const int ref_action_bar_left, ref_action_bar_top;
    const int game_over_t1, game_over_t2, game_over_t3;

    // controllers.
    const Controller *controller1, *controller2;
    const std::vector<const UserControl*> &standard_controls;
    
    // controller state
    bool attack_mode[2], allow_menu_open[2], approached_when_menu_was_opened[2];
    int fire_start_time[2];
    enum MenuNullEnum { M_OK, M_CTS, M_NULL };
    MenuNullEnum menu_null[2];
    MapDirection menu_null_dir[2];
    MapDirection my_facing[2];
    const UserControl * tap_control[2];
    const UserControl * menu_control[2][4];

    // state for new control system (WASD+mouse)

    // slot_controls: contains controls currently assigned to the action bar, or the "tap" slot.
    const UserControl * slot_controls[2][TOTAL_NUM_SLOTS];

    // mouse_over_slot: which slot is the mouse currently over.
    // could also be TAP_SLOT if the mouse is not over any action bar slot, or NO_SLOT if the mouse is
    // over a gui control and therefore LMB controls should be disabled.
    // NB: does not update while LMB held down, this means controls "stick" until you release the mouse button.
    int mouse_over_slot[2];

    // lmb_down: True if the LMB was down on the previous frame.
    bool lmb_down[2];

    // chat_flag: Set true when chat is active. Set false when LMB is released.
    // Used to prevent the first left-click (to get out of chat mode) from activating any controls.
    bool chat_flag[2];
    
    
    // display components
    std::vector<boost::shared_ptr<LocalDungeonView> > dungeon_view;
    std::vector<boost::shared_ptr<LocalMiniMap> > mini_map;
    std::vector<boost::shared_ptr<LocalStatusDisplay> > status_display;
    boost::shared_ptr<ActionBar> action_bar;

    // other display stuff.
    std::vector<int> flash_screen_start;
    boost::shared_ptr<std::vector<std::pair<std::string,std::string> > > menu_strings;
    const Graphic *winner_image;
    const Graphic *loser_image;

    // we need to keep track of time (for animations, making entities move at the correct speed, etc).
    // the time is stored here and updated by calls to draw() (caller must pass in an updated time).
    int time;

    // have we won or lost yet
    std::vector<bool> won;
    std::vector<bool> lost;
    std::vector<int> game_over_time;
    bool ready_msg_sent;
    
    // sounds waiting to be played
    struct MySound {
        const Sound *sound;
        int frequency;

        // This vector tells us which players can hear this sound. This is relevant if we are an observer
        // and there are more than two players in the game; in that case, we receive sounds for all players
        // from the server, but we only actually want to play sounds for the two players who are currently
        // on-screen.
        std::vector<bool> plyr;
    };
    std::vector<MySound> sounds;

    // timer
    Coercri::Timer & timer;
    int last_time;
    int game_end_time; // -1 if not set or else the value of 'time' when the game should end.

    // gui stuff
    ChatList &chat_list;
    ChatList &ingame_player_list;
    ChatList &quest_rqmts_list;
    KnightsClient &knights_client;
    gcn::Container &container;
    bool chat_updated;
    int prev_gui_x, prev_gui_y, prev_gui_width, prev_gui_height;
    boost::scoped_ptr<Coercri::CGFont> gui_font, gui_small_font;
    boost::scoped_ptr<gcn::ListBox> chat_listbox;
    boost::scoped_ptr<gcn::ScrollArea> chat_scrollarea;
    boost::scoped_ptr<TitleBlock> chat_titleblock;
    boost::scoped_ptr<gcn::TextField> chat_field;
    boost::scoped_ptr<gcn::Button> send_button, clear_button;
    boost::scoped_ptr<TitleBlock> plyr_list_titleblock;
    boost::scoped_ptr<TabFont> tab_font;
    boost::shared_ptr<gcn::Font> house_colour_font;
    boost::scoped_ptr<gcn::ListBox> plyr_list_listbox;
    boost::scoped_ptr<gcn::ScrollArea> plyr_list_scrollarea;
    boost::scoped_ptr<class TutorialWidget> tutorial_widget;
    boost::scoped_ptr<gcn::ScrollArea> tutorial_scrollarea;
    boost::scoped_ptr<gcn::Label> tutorial_label;
    boost::scoped_ptr<gcn::Button> tutorial_left, tutorial_right;
    boost::scoped_ptr<gcn::Label> time_limit_label;

    boost::scoped_ptr<TitleBlock> quest_titleblock;
    boost::scoped_ptr<gcn::ListBox> quest_listbox;
    boost::scoped_ptr<gcn::ScrollArea> quest_scrollarea;

    boost::shared_ptr<Coercri::Font> my_font, txt_font;

    std::string time_limit_string;
    
    // player names. used in "observer" mode.
    std::vector<std::string> names;
    int nplayers;
    int curr_obs_player;
    std::vector<bool> obs_visible;
    
    bool observer_mode;
    bool single_player;
    bool tutorial_mode;
    bool have_sent_end_msg;
    
    std::deque<TutorialWindow> tutorial_popups;
    std::vector<TutorialWindow> tutorial_windows;
    int tutorial_selected_window;
    std::string up_string, down_string, left_string, right_string, action_string, suicide_string;

    const Graphic *speech_bubble;

    bool action_bar_tool_tips;
    bool deathmatch_mode;
    std::string chat_msg;

    bool quest_rqmts_minimized;
    bool force_setup_gui;  // used to force gui update when the quest rqmts area is minimized.
};

#endif
