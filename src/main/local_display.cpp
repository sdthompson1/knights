/*
 * local_display.cpp
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

#include "misc.hpp"

#include "action_bar.hpp"
#include "config_map.hpp"
#include "controller.hpp"
#include "game_manager.hpp"  // for ChatList
#include "gfx_manager.hpp"
#include "gfx_resizer.hpp"
#include "graphic.hpp"
#include "house_colour_font.hpp"
#include "knights_client.hpp"
#include "local_display.hpp"
#include "local_dungeon_view.hpp"
#include "local_mini_map.hpp"
#include "local_status_display.hpp"
#include "make_scroll_area.hpp"
#include "my_exceptions.hpp"
#include "round.hpp"
#include "sound_manager.hpp"
#include "tab_font.hpp"
#include "text_formatter.hpp"
#include "title_block.hpp"
#include "user_control.hpp"

#include "gfx/rectangle.hpp"  // coercri
#include "gcn/cg_graphics.hpp"

#include <iomanip>
#include <limits>
#include <sstream>

namespace {    

    int GetNextPlayer(int p1, int delta, const std::vector<bool> &obs_visible)
    {
        ASSERT(delta == 1 || delta == -1);
        ASSERT(p1 >= 0 && p1 < int(obs_visible.size()));
        
        int p2 = p1;
        while (1) {
            p2 += delta;
            if (p2 == int(obs_visible.size())) {
                p2 = 0;
            }
            if (p2 == -1) {
                p2 = int(obs_visible.size()) - 1;
            }

            if (p2 == p1) break; // got back to where we started
            if (obs_visible[p2]) break; // this screen is visible so display it
        }

        return p2;
    }

    // a version of ListBox that doesn't react to mouse clicks on it.
    class ListBoxNoMouse : public gcn::ListBox {
    public:
        virtual void mousePressed(gcn::MouseEvent &) { }
    };

    // draw "key" lines in the ESC menu.
    void DrawKeyLine(int xc, int y, const Coercri::Color &col1, const Coercri::Color &col2,
                     Coercri::GfxContext &gc, GfxManager &gm, const char *msg1_utf8, const char *msg2_utf8)
    {
        const UTF8String separator = UTF8String::fromUTF8(": ");
        const UTF8String msg1 = UTF8String::fromUTF8(msg1_utf8);
        const UTF8String msg2 = UTF8String::fromUTF8(msg2_utf8);

        const int w1 = gm.getFont()->getTextWidth(msg1 + separator);
        const int w = gm.getFont()->getTextWidth(msg1 + separator + msg2);
        gc.drawText(xc - w/2, y, *gm.getFont(), msg1 + separator, col1);
        gc.drawText(xc - w/2 + w1, y, *gm.getFont(), msg2, col2);
    }

    struct CountingPrinter : Printer {
        explicit CountingPrinter(const Coercri::Font &font_) : font(font_), count(0) { }
        virtual int getTextWidth(const std::string &t_latin1) { return font.getTextWidth(UTF8String::fromLatin1(t_latin1)); }
        virtual int getTextHeight() { return font.getTextHeight(); }
        virtual void printLine(const std::string &, int, bool) { ++count; }

        const Coercri::Font &font;
        int count;
    };

    struct CountingPrinter2 : Printer {
        explicit CountingPrinter2(const gcn::Font &font_) : font(font_), count(0) { }
        int getTextWidth(const std::string &t) { return font.getWidth(t); }
        int getTextHeight() { return font.getHeight(); }
        void printLine(const std::string &, int, bool) { ++count; }

        const gcn::Font &font;
        int count;
    };

    struct MyPrinter : Printer {
        MyPrinter(Coercri::GfxContext &gc_, const Coercri::Font &font_, const Coercri::Color &col_,
                  int x, int y) : gc(gc_), font(font_), col(col_), x0(x), y0(y) { }
        virtual int getTextWidth(const std::string &t_latin1) { return font.getTextWidth(UTF8String::fromLatin1(t_latin1)); }
        virtual int getTextHeight() { return font.getTextHeight(); }
        void printLine(const std::string &text_latin1, int y, bool do_centre) { gc.drawText(x0, y0 + y, font, UTF8String::fromLatin1(text_latin1), col); }

        Coercri::GfxContext &gc;
        const Coercri::Font &font;
        Coercri::Color col;
        int x0, y0;
    };

    struct GcnPrinter : Printer {
        GcnPrinter(gcn::Graphics &gfx, const gcn::Font &f, int x, int y) : graphics(gfx), font(f), x0(x), y0(y) { }
        int getTextWidth(const std::string &x) { return font.getWidth(x); }
        int getTextHeight() { return font.getHeight(); }
        void printLine(const std::string &text, int y, bool do_centre) { graphics.drawText(text, x0, y0 + y); }

        gcn::Graphics &graphics;
        const gcn::Font &font;
        int x0, y0;
    };
}

class TutorialWidget : public gcn::Widget { 
public:
    TutorialWidget(GfxManager &gm_)
        : gm(gm_), scale_factor(1.0f) { }
    
    void setTitle(const std::string &t) { title = t; }   // latin1
    void setText(const std::string &t) { text = t; }     // latin1
    void setGfx(const std::vector<const Graphic *> &gfx_, const std::vector<ColourChange> &cc_)
        { gfx = gfx_; cc = cc_; }
    void setScaleFactor(float sf) { scale_factor = sf; }
    float getScaleFactor() const { return scale_factor; }

    void wipe()
    {
        title = text = "";
        gfx.clear();
        cc.clear();
    }

    void adjustHeight()
    {
        int dummy, gfx_height = 0;
        if (!gfx.empty() && gfx[0]) {
            gm.getGraphicSize(*gfx[0], dummy, gfx_height);
            gfx_height = Round(gfx_height * scale_factor);
        }

        CountingPrinter2 pr(*getFont());
        TextFormatter tf(pr, getWidth(), false);
        tf.printString(text);
        int nlines = pr.count + 1;
        if (gfx_height == 0) ++nlines;
        const int text_height = getFont()->getHeight();
        setHeight(nlines * text_height + std::max(gfx_height, text_height) + 2);
    }
    
    virtual void draw(gcn::Graphics *graphics)
    {
        Coercri::CGGraphics * cg_graphics = dynamic_cast<Coercri::CGGraphics*>(graphics);  // should always succeed
        Coercri::GfxContext * gc = cg_graphics ? cg_graphics->getTarget() : 0;
        
        const int text_height = getFont()->getHeight();
        int y = 2;
        
        int first_gfx_width = 0, first_gfx_height = 0;
        if (!gfx.empty() && gfx[0]) {
            gm.getGraphicSize(*gfx[0], first_gfx_width, first_gfx_height);
            first_gfx_width = Round(first_gfx_width * scale_factor);
            first_gfx_height = Round(first_gfx_height * scale_factor);
        }

        const bool add_handle = (gfx.size() == 1); // needed to get items to draw properly

        // Work out position of the title
        const std::string title_string = first_gfx_width > 0 ? (" " + title) : title;
        const int title_offset = std::max(0, (first_gfx_height - text_height) / 2 - 1);
        
        // Draw gfx
        for (int i = 0; i < int(gfx.size()) && i < int(cc.size()); ++i) {
            if (gfx[i] && gc) {
                // Work out gfx size
                int gfx_width, gfx_height;
                gm.getGraphicSize(*gfx[i], gfx_width, gfx_height);
                gfx_width = Round(gfx_width * scale_factor);
                gfx_height = Round(gfx_height * scale_factor);
                
                // bypass guichan graphics object and just use gm to draw directly.
                const int xx = graphics->getCurrentClipArea().xOffset + (add_handle ? Round(gfx[i]->getHX() * scale_factor) : 0);
                const int yy = graphics->getCurrentClipArea().yOffset + (add_handle ? Round(gfx[i]->getHY() * scale_factor) : 0);
                gm.drawTransformedGraphic(*gc, xx, y + yy, *gfx[i], gfx_width, gfx_height, cc[i]);
            }
        }

        // Draw title
        graphics->setFont(getFont());
        graphics->setColor(gcn::Color(255,255,255));
        graphics->drawText(title_string, first_gfx_width, y + title_offset);
        y += std::max(text_height*2, first_gfx_height + text_height/2);

        // Draw main text
        GcnPrinter pr(*graphics, *getFont(), 0, y);
        TextFormatter tf(pr, getWidth(), false);
        tf.printString(text);
    }
    
private:
    GfxManager &gm;
    std::string title, text;
    std::vector<const Graphic *> gfx;
    std::vector<ColourChange> cc;
    float scale_factor;
};


LocalDisplay::LocalDisplay(const ConfigMap &cfg,
                           int aofs,
                           const Graphic *winner_image_,
                           const Graphic *loser_image_,
                           const Graphic *speech_bubble_,
                           const Graphic *menu_gfx_centre,
                           const Graphic *menu_gfx_empty,
                           const Graphic *menu_gfx_highlight,
                           const PotionRenderer *potion_rend,
                           const SkullRenderer *skull_rend,
                           boost::shared_ptr<std::vector<std::pair<std::string,std::string> > > menu_strings_,
                           const std::vector<const UserControl*> &std_ctrls,
                           const Controller *ctrlr1, 
                           const Controller *ctrlr2,
                           int nplyrs,
                           bool dm_mode,
                           const std::vector<UTF8String> &player_names,
                           Coercri::Timer & timer_,
                           ChatList &chat_list_,
                           ChatList &ingame_player_list_,
                           ChatList &quest_rqmts_list_,
                           KnightsClient &knights_client_,
                           gcn::Container &container_,
                           const std::string &u_key, const std::string &d_key, const std::string &l_key,
                           const std::string &r_key, const std::string &a_key, const std::string &s_key,
                           bool sgl_plyr,
                           bool tut,
                           bool tool_tips,
                           const std::string &chat_keys)
    : config_map(cfg),
      approach_offset(aofs),
      potion_renderer(potion_rend),
      skull_renderer(skull_rend),

      // cached config variables
      ref_vp_width(cfg.getInt("dpy_viewport_width")),
      ref_vp_height(cfg.getInt("dpy_viewport_height")),
      ref_gutter(cfg.getInt("dpy_gutter")),
      
      ref_pixels_per_square(cfg.getInt("dpy_pixels_per_square")),
      dungeon_tiles_x(cfg.getInt("dpy_dungeon_width") / ref_pixels_per_square),
      dungeon_tiles_y(cfg.getInt("dpy_dungeon_height") / ref_pixels_per_square),

      min_font_size(cfg.getInt("dpy_font_min_size")),
      ref_font_size(cfg.getInt("dpy_font_base_size") / 100.0f),

      ref_action_bar_left(cfg.getInt("dpy_action_bar_left")),
      ref_action_bar_top(cfg.getInt("dpy_action_bar_top")),
      
      game_over_t1(cfg.getInt("game_over_fade_time")),
      game_over_t2(game_over_t1 + cfg.getInt("game_over_black_time")),
      game_over_t3(game_over_t2 + game_over_t1),
      
      // other stuff
      controller1(ctrlr1),
      controller2(ctrlr2),
      standard_controls(std_ctrls),
      menu_strings(menu_strings_),
      winner_image(winner_image_),
      loser_image(loser_image_),
      time(0),
      ready_msg_sent(false),

      timer(timer_),
      last_time(-1),
      game_end_time(-1),
      fps(std::min(1000, cfg.getInt("fps"))),

      chat_list(chat_list_),
      ingame_player_list(ingame_player_list_),
      quest_rqmts_list(quest_rqmts_list_),
      knights_client(knights_client_),
      container(container_),
      chat_updated(false),

      prev_gui_x(-1), prev_gui_y(-1), prev_gui_width(0), prev_gui_height(0),

      single_player(sgl_plyr),
      tutorial_mode(tut),
      have_sent_end_msg(false),

      tutorial_selected_window(-1),
      
      up_string(u_key), down_string(d_key), left_string(l_key), right_string(r_key),
      action_string(a_key), suicide_string(s_key),
      
      speech_bubble(speech_bubble_),
      action_bar_tool_tips(tool_tips),
      deathmatch_mode(dm_mode),

      chat_msg("<Click here or press " + chat_keys + " to chat>"),

      quest_rqmts_minimized(false),
      force_setup_gui(false)
{
    for (int i = 0; i < 2; ++i) {
        attack_mode[i] = false;
        allow_menu_open[i] = true;
        approached_when_menu_was_opened[i] = false;
        fire_start_time[i] = 0;
        menu_null[i] = M_OK;
        menu_null_dir[i] = D_NORTH;
        my_facing[i] = D_NORTH;
        tap_control[i] = 0;
        for (int j = 0; j < 4; ++j) menu_control[i][j] = 0;
        for (int j = 0; j < TOTAL_NUM_SLOTS; ++j) slot_controls[i][j] = 0;
        mouse_over_slot[i] = NO_SLOT;
        lmb_down[i] = false;
        chat_flag[i] = false;
    }

    initialize(nplyrs, player_names, menu_gfx_centre, menu_gfx_empty, menu_gfx_highlight, speech_bubble);
}

LocalDisplay::~LocalDisplay()
{
    // empty dtor - needed because of unique_ptrs in the header file.
}

void LocalDisplay::disableView(int p)
{
    if (p < 0 || p >= nplayers) return;

    obs_visible[p] = false;
    for (int which = 0; which < 2; ++which) {
        if (p == curr_obs_player[which]) {
            curr_obs_player[which] = GetNextPlayer(curr_obs_player[which], 1, obs_visible);
        }
    }
}

void LocalDisplay::goIntoObserverMode(int nplyrs,
                                      const std::vector<UTF8String> &player_names)
{
    dungeon_view.clear();
    mini_map.clear();
    status_display.clear();
    action_bar.reset();
    won.clear();
    lost.clear();
    game_over_time.clear();
    flash_screen_start.clear();

    initialize(nplyrs, player_names, 0, 0, 0, speech_bubble);
}

void LocalDisplay::initialize(int nplyrs, const std::vector<UTF8String> &player_names,
                              const Graphic *menu_gfx_centre,
                              const Graphic *menu_gfx_empty,
                              const Graphic *menu_gfx_highlight,
                              const Graphic *speech_bubble)
{
    // common code between ctor and goIntoObserverMode.
    nplayers = nplyrs;
    curr_obs_player[0] = 0;
    curr_obs_player[1] = nplayers >= 2 ? 1 : 0;
    obs_visible.assign(nplayers, true);  // all screens visible initially
    names = player_names;

    dungeon_view.resize(nplyrs);
    mini_map.resize(nplyrs);
    status_display.resize(nplyrs);
    won.resize(nplyrs);
    lost.resize(nplyrs);
    game_over_time.resize(nplyrs);
    flash_screen_start.resize(nplyrs);
    
    // If names are set then we must be in "observer mode"
    observer_mode = !names.empty();

    if (!observer_mode && (controller1->usingActionBar() || tutorial_mode)) {
        action_bar.reset(new ActionBar(config_map, menu_gfx_empty, menu_gfx_highlight));
    }
    
    for (int i = 0; i < nplyrs; ++i) {
        dungeon_view[i].reset(new LocalDungeonView(config_map, approach_offset, speech_bubble));
        mini_map[i].reset(new LocalMiniMap(config_map));
        status_display[i].reset(new LocalStatusDisplay(config_map, potion_renderer, skull_renderer,
                                                       menu_gfx_centre, menu_gfx_empty, menu_gfx_highlight));

        won[i] = lost[i] = false;
        game_over_time[i] = -9999;
        flash_screen_start[i] = -9999;
    }

    if (tutorial_mode && controller1 && !controller1->usingActionBar()) {
        TutorialWindow win;
        win.title_latin1 = "NOTE";
        win.msg_latin1 = "You have selected old-style controls (keyboard only).\n\n"
            "This tutorial is written for people who use the new-style controls (mouse "
            "and keyboard). However, it is perfectly playable using either set of controls.\n\n"
            "During the tutorial, both sets of controls will be available and you can use "
            "whichever you prefer. Controls will be reset back to normal when the tutorial ends.";
        tutorial_popups.push_back(win);
        tutorial_windows.push_back(win);
    }
}

unsigned int LocalDisplay::getAdjustedTime()
{
    // Get time now, but adjusted to the start of a "frame boundary"
    // The aim is to try to get smoother animation by making sure that all drawn frames
    // are roughly equally spaced in time
    const unsigned int time_now = timer.getMsec();
    const unsigned int frame_count = MsecToFrameCount(time_now, fps);
    return FrameCountToMsec(frame_count, fps);
}

void LocalDisplay::recalculateTime(bool is_paused, int remaining)
{
    const int time_now = getAdjustedTime();
    if (last_time == -1) last_time = time_now;  // initialization.
    const int time_delta = time_now - last_time;
    last_time = time_now;

    if (!is_paused) {
        time += time_delta;
        for (int plyr_num = 0; plyr_num < nplayers; ++plyr_num) {
            dungeon_view[plyr_num]->addToTime(time_delta);
        }

        // recalculate game_end_time
        if (remaining >= 0) {
            const int new_end_time = time + remaining;
            if (game_end_time < 0) prev_gui_x = -9999; // force a re-layout of the gui
            if (new_end_time < game_end_time || game_end_time < 0) game_end_time = new_end_time;
            setTimeLimitCaption();
        }
    }
}

void LocalDisplay::setTimeLimitCaption()
{
    if (game_end_time >= 0) {

        int t = 0;
        for (std::vector<int>::const_iterator it = game_over_time.begin(); it != game_over_time.end(); ++it) {
            if (*it < 0) {
                // The game is not finished yet. Use current time
                t = time;
                break;
            } else {
                // Use the game_over_time of the last player to finish
                t = std::max(t, *it);
            }
        }
        
        const int time_left = std::max(0, game_end_time - t + 999) / 1000;
        const int mins = time_left / 60;
        const int secs = time_left % 60;
        std::stringstream str;
        str.fill('0');
        str << mins << ":" << std::setw(2) << secs;
        time_limit_string = str.str();
    } else {
        time_limit_string = std::string();
    }

    if (time_limit_label) {
        time_limit_label->setCaption("Time remaining: " + time_limit_string);
        time_limit_label->adjustSize();
        time_limit_string = std::string(); // don't draw both under the potion AND in the time limit label.
    }
}

void LocalDisplay::setupGui(int chat_area_x, int chat_area_y, int chat_area_width, int chat_area_height,
                            int plyr_list_x, int plyr_list_y, int plyr_list_width, int plyr_list_height,
                            int quest_rqmts_x, int quest_rqmts_y, int quest_rqmts_width, int quest_rqmts_height,
                            GfxManager &gm)
{
    const bool chat_should_be_focused = chat_field && chat_field->isFocused();
    
    const gcn::Color bg_col(0x66, 0x66, 0x44);

    // Clear out the container as we are going to re-populate it.
    // (Not strictly necessary because DeathListener will remove the widgets when
    // they get reset()... but it makes things neater.)
    container.clear();
    
    // Resize and reposition the container
    container.setSize(std::max(chat_area_x+chat_area_width, plyr_list_x+plyr_list_width),
                      std::max(chat_area_y+chat_area_height, plyr_list_y+plyr_list_height));
    container.setPosition(0, 0);
    container.setFocusable(true); // allow clicking away from the chat area.
    

    //
    // CHAT AREA
    //
    
    int y2 = chat_area_y + chat_area_height - 2;

    if (!single_player) {

        // Add text field and labels, and the two buttons.

        send_button.reset(new gcn::Button("Send"));
        send_button->addActionListener(this);
        send_button->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        send_button->setForegroundColor(gcn::Color(255,255,255));
        send_button->setFont(gui_small_font.get());
        send_button->adjustSize();
        send_button->setFocusable(false);
        const int chat_y = chat_area_y + chat_area_height - send_button->getHeight();
        container.add(send_button.get(), chat_area_x + chat_area_width - send_button->getWidth(), chat_y);

        clear_button.reset(new gcn::Button("Clear"));
        clear_button->addActionListener(this);
        clear_button->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        clear_button->setForegroundColor(gcn::Color(255,255,255));
        clear_button->setFont(gui_small_font.get());
        clear_button->adjustSize();
        clear_button->setFocusable(false);
        container.add(clear_button.get(), send_button->getX() - clear_button->getWidth() - 2, chat_y);
        
        chat_field.reset(new gcn::TextField);
        chat_field->setForegroundColor(gcn::Color(255, 255, 255));
        chat_field->setBackgroundColor(bg_col);
        chat_field->setFont(gui_font.get());
        chat_field->adjustSize();
        chat_field->setWidth(clear_button->getX() - chat_area_x - 2);
        chat_field->addActionListener(this);
        chat_field->addFocusListener(this);
        const int chat_y2 = chat_y + std::max(0, (send_button->getHeight() - chat_field->getHeight())/2);
        container.add(chat_field.get(), chat_area_x, chat_y2);
        if (chat_should_be_focused) {
            activateChatField();
            chat_field->requestFocus();
        } else {
            container.requestFocus();
            deactivateChatField();
        }

        y2 = chat_y - 8;
    }

    // "Messages" titleblock
    std::vector<std::string> titles(1, "Messages");
    std::vector<int> widths(1, chat_area_width);
    chat_titleblock.reset(new TitleBlock(titles, widths));
    chat_titleblock->setFont(gui_font.get());
    chat_titleblock->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
    chat_titleblock->setForegroundColor(gcn::Color(0xff, 0xff, 0xff));
    chat_titleblock->setFocusable(true);
    container.add(chat_titleblock.get(), chat_area_x, chat_area_y);
        
    // Add the Messages listbox
    chat_listbox.reset(new ListBoxNoMouse);
    chat_listbox->setBackgroundColor(gcn::Color(0,0,0));
    chat_listbox->setSelectionColor(gcn::Color(0,0,0));
    chat_listbox->setFont(gui_font.get());
    chat_listbox->setForegroundColor(gcn::Color(255,255,255));
    chat_list.setGuiParams(chat_listbox->getFont(), chat_area_width - DEFAULT_SCROLLBAR_WIDTH);
    chat_listbox->setListModel(&chat_list);
    chat_listbox->setWidth(chat_area_width - DEFAULT_SCROLLBAR_WIDTH);
    
    chat_scrollarea.reset(new gcn::ScrollArea);
    chat_scrollarea->setContent(chat_listbox.get());
    chat_scrollarea->setSize(chat_area_width, y2 - chat_area_y - chat_titleblock->getHeight() - 2);
    chat_scrollarea->setScrollbarWidth(DEFAULT_SCROLLBAR_WIDTH);
    chat_scrollarea->setFrameSize(0);
    chat_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
    chat_scrollarea->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_ALWAYS);
    chat_scrollarea->setBaseColor(bg_col);
    chat_scrollarea->setBackgroundColor(gcn::Color(0,0,0));
    chat_scrollarea->setOpaque(false);
    container.add(chat_scrollarea.get(), chat_area_x, chat_area_y + chat_titleblock->getHeight() + 2);

    //
    // PLAYER LIST
    //

    if (!tutorial_mode) {
        // Titles ("Player", "Kills", "Deaths" etc)
        titles.clear();
        titles.reserve(deathmatch_mode ? 3 : 4);
        titles.push_back("Player");
        if (deathmatch_mode) {
            titles.push_back("Score");
        } else {        
            titles.push_back("Kills");
            titles.push_back("Deaths");
        }
        titles.push_back("Ping");
        widths.clear();
        widths.reserve(titles.size());
        
        int num_width = gui_font->getWidth("9999");
        for (int i = 1; i < titles.size(); ++i) {
            num_width = std::max(num_width, gui_font->getWidth(titles[i]));
        }
        num_width += gui_font->getWidth("  ") + 4;
        
        widths.push_back(plyr_list_width - (titles.size()-1)*num_width);
        for (int i = 0; i < titles.size()-1; ++i) widths.push_back(num_width);

        plyr_list_titleblock.reset(new TitleBlock(titles, widths));
        plyr_list_titleblock->setFont(gui_font.get());
        plyr_list_titleblock->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        plyr_list_titleblock->setForegroundColor(gcn::Color(0xff, 0xff, 0xff));
        container.add(plyr_list_titleblock.get(), plyr_list_x, plyr_list_y);

        // Time remaining
        int time_limit_height = 0;
        if (game_end_time >= 0) {
            time_limit_label.reset(new gcn::Label);
            time_limit_label->setFont(gui_font.get());
            time_limit_label->setForegroundColor(gcn::Color(255,255,255));
            setTimeLimitCaption();
            time_limit_height = time_limit_label->getHeight();
        }
        
        // make a tabfont on top of a house colour font
        // (doing it the other way around doesn't work properly!)
        const int sz = gui_font->getHeight() - 3;
        house_colour_font.reset(new HouseColourFont(*gui_font, sz, sz));
        tab_font.reset(new TabFont(house_colour_font, widths));
        
        plyr_list_listbox.reset(new ListBoxNoMouse);
        plyr_list_listbox->setBackgroundColor(gcn::Color(0,0,0));
        plyr_list_listbox->setSelectionColor(gcn::Color(0,0,0));
        plyr_list_listbox->setFont(tab_font.get());
        plyr_list_listbox->setForegroundColor(gcn::Color(255,255,255));
        plyr_list_listbox->setListModel(&ingame_player_list);
        plyr_list_listbox->setWidth(plyr_list_width);
        
        plyr_list_scrollarea.reset(new gcn::ScrollArea);
        plyr_list_scrollarea->setContent(plyr_list_listbox.get());
        plyr_list_scrollarea->setSize(plyr_list_width, plyr_list_height - plyr_list_titleblock->getHeight() - time_limit_height - 2);
        plyr_list_scrollarea->setScrollbarWidth(DEFAULT_SCROLLBAR_WIDTH);
        plyr_list_scrollarea->setFrameSize(0);
        plyr_list_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
        plyr_list_scrollarea->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_AUTO);
        plyr_list_scrollarea->setBaseColor(bg_col);
        plyr_list_scrollarea->setBackgroundColor(gcn::Color(0,0,0));
        plyr_list_scrollarea->setOpaque(false);
        container.add(plyr_list_scrollarea.get(), plyr_list_x, plyr_list_y + plyr_list_titleblock->getHeight() + 2);

        if (time_limit_label.get()) {
            container.add(time_limit_label.get(), plyr_list_x, plyr_list_scrollarea->getY() + plyr_list_scrollarea->getHeight());
        }
    }

    //
    // TUTORIAL WINDOW
    //

    if (tutorial_mode) {
        // Titleblock -- note can reuse 'player_list_titleblock' since there is no player list in tutorial mode
        std::vector<std::string> titles(1, "Tutorial");
        std::vector<int> widths(1, chat_area_width);
        plyr_list_titleblock.reset(new TitleBlock(titles, widths));
        plyr_list_titleblock->setFont(gui_font.get());
        plyr_list_titleblock->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        plyr_list_titleblock->setForegroundColor(gcn::Color(0xff, 0xff, 0xff));
        plyr_list_titleblock->setFocusable(true);
        container.add(plyr_list_titleblock.get(), plyr_list_x, plyr_list_y);

        // Left/Right buttons
        tutorial_right.reset(new gcn::Button("  >>  "));
        tutorial_right->addActionListener(this);
        tutorial_right->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        tutorial_right->setForegroundColor(gcn::Color(255,255,255));
        tutorial_right->setFont(gui_small_font.get());
        tutorial_right->adjustSize();
        container.add(tutorial_right.get(), plyr_list_x + plyr_list_width - tutorial_right->getWidth(),
                      plyr_list_y + plyr_list_height - tutorial_right->getHeight());
        
        tutorial_left.reset(new gcn::Button("  <<  "));
        tutorial_left->addActionListener(this);
        tutorial_left->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        tutorial_left->setForegroundColor(gcn::Color(255,255,255));
        tutorial_left->setFont(gui_small_font.get());
        tutorial_left->adjustSize();
        container.add(tutorial_left.get(), plyr_list_x + plyr_list_width - tutorial_left->getWidth() - tutorial_right->getWidth(),
                      plyr_list_y + plyr_list_height - tutorial_left->getHeight());

        tutorial_label.reset(new gcn::Label);
        tutorial_label->setFont(gui_small_font.get());
        tutorial_label->setAlignment(gcn::Graphics::RIGHT);
        tutorial_label->setWidth(std::max(0, plyr_list_width - tutorial_left->getWidth() - tutorial_right->getWidth() - 6));
        tutorial_label->setHeight(tutorial_label->getFont()->getHeight());
        tutorial_label->setForegroundColor(gcn::Color(255,255,255));
        const int y_adjust = std::max(0, tutorial_left->getHeight() - tutorial_label->getHeight()) / 2;
        container.add(tutorial_label.get(), plyr_list_x, tutorial_left->getY() + y_adjust);
        
        // Tutorial Widget itself
        const float old_scale_factor = tutorial_widget ? tutorial_widget->getScaleFactor() : 1.0f;
        tutorial_widget.reset(new TutorialWidget(gm));
        tutorial_widget->setScaleFactor(old_scale_factor);
        tutorial_widget->setFont(gui_font.get());
        tutorial_widget->setWidth(plyr_list_width - DEFAULT_SCROLLBAR_WIDTH);

        tutorial_scrollarea.reset(new gcn::ScrollArea);
        tutorial_scrollarea->setContent(tutorial_widget.get());
        tutorial_scrollarea->setSize(plyr_list_width, plyr_list_height - plyr_list_titleblock->getHeight() - 2 - tutorial_left->getHeight());
        tutorial_scrollarea->setScrollbarWidth(DEFAULT_SCROLLBAR_WIDTH);
        tutorial_scrollarea->setFrameSize(0);
        tutorial_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
        tutorial_scrollarea->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_AUTO);
        tutorial_scrollarea->setBaseColor(bg_col);
        tutorial_scrollarea->setBackgroundColor(gcn::Color(0,0,0));
        tutorial_scrollarea->setOpaque(false);
        container.add(tutorial_scrollarea.get(), plyr_list_x, plyr_list_y + plyr_list_titleblock->getHeight() + 2);
        
        updateTutorialWidget();
    }

    //
    // QUEST REQUIREMENTS
    //

    if (quest_rqmts_height > 0) {

        std::vector<std::string> titles(1, "Quest Requirements");
        if (quest_rqmts_minimized) titles.front().append(" (click to show)");
        std::vector<int> widths(1, quest_rqmts_width);
        quest_titleblock.reset(new TitleBlock(titles, widths));
        quest_titleblock->setFont(gui_font.get());
        quest_titleblock->setBaseColor(gcn::Color(0x66, 0x66, 0x44));
        quest_titleblock->setForegroundColor(gcn::Color(0xff, 0xff, 0xff));
        quest_titleblock->setFocusable(true);
        quest_titleblock->addMouseListener(this);
        container.add(quest_titleblock.get(), quest_rqmts_x, quest_rqmts_y);

        if (quest_rqmts_minimized) {
            quest_listbox.reset();
            quest_scrollarea.reset();
        } else {

            quest_listbox.reset(new ListBoxNoMouse);
            quest_listbox->setBackgroundColor(gcn::Color(0,0,0));
            quest_listbox->setSelectionColor(gcn::Color(0,0,0));
            quest_listbox->setFont(gui_font.get());
            quest_listbox->setForegroundColor(gcn::Color(255,255,255));
            quest_listbox->setListModel(&quest_rqmts_list);
            quest_listbox->setWidth(quest_rqmts_width - DEFAULT_SCROLLBAR_WIDTH);
            
            quest_scrollarea.reset(new gcn::ScrollArea);
            quest_scrollarea->setContent(quest_listbox.get());
            quest_scrollarea->setSize(quest_rqmts_width, quest_rqmts_height - quest_titleblock->getHeight() - 2);
            quest_scrollarea->setScrollbarWidth(DEFAULT_SCROLLBAR_WIDTH);
            quest_scrollarea->setFrameSize(0);
            quest_scrollarea->setHorizontalScrollPolicy(gcn::ScrollArea::SHOW_NEVER);
            quest_scrollarea->setVerticalScrollPolicy(gcn::ScrollArea::SHOW_AUTO);
            quest_scrollarea->setBaseColor(bg_col);
            quest_scrollarea->setBackgroundColor(gcn::Color(0,0,0));
            container.add(quest_scrollarea.get(), quest_rqmts_x, quest_rqmts_y + quest_titleblock->getHeight() + 2);
        }
    }
}

void LocalDisplay::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == chat_field.get() || event.getSource() == send_button.get()) {
        const std::string msg = getChatFieldContents();

        bool empty = true;
        for (std::string::const_iterator it = msg.begin(); it != msg.end(); ++it) {
            if (*it != ' ') {
                empty = false;
                break;
            }
        }
        
        if (!empty) {
            knights_client.sendChatMessage(msg);
        }
        
        chat_field->setText("");
        container.requestFocus();
        deactivateChatField();
        
    } else if (event.getSource() == clear_button.get()) {
        chat_field->setText("");
        if (!chat_field->isFocused()) deactivateChatField();
    } else if (event.getSource() == tutorial_left.get()) {
        if (tutorial_selected_window > tutorial_windows.size()) tutorial_selected_window = tutorial_windows.size();
        --tutorial_selected_window;
        if (tutorial_selected_window < 0) tutorial_selected_window = 0;
        updateTutorialWidget();
    } else if (event.getSource() == tutorial_right.get()) {
        if (tutorial_selected_window < 0) tutorial_selected_window = -1;
        ++tutorial_selected_window;
        if (tutorial_selected_window >= tutorial_windows.size()) tutorial_selected_window = tutorial_windows.size()-1;
        updateTutorialWidget();
    }
}

void LocalDisplay::mouseClicked(gcn::MouseEvent &event)
{
    if (event.getSource() == quest_titleblock.get() && event.getButton() == gcn::MouseEvent::LEFT) {
        quest_rqmts_minimized = !quest_rqmts_minimized;
        force_setup_gui = true;
    }
}

// Checks whether the mouse is in a gui control e.g. the chat box, the Clear/Send buttons, etc.
// The idea is to prevent LMB from controlling the knight when you are clicking on important GUI buttons.
bool LocalDisplay::mouseInGuiControl(int mx, int my) const
{
    gcn::Widget * widget = container.getWidgetAt(mx, my);

    if (widget) {
        if (widget == chat_field.get() ||
            widget == send_button.get() ||
            widget == clear_button.get() ||
            widget == tutorial_left.get() ||
            widget == tutorial_right.get() ||
            widget == quest_titleblock.get()) {
                return true;
        } else if (widget == chat_scrollarea.get() || widget == tutorial_scrollarea.get()) {
            const int relx = mx - widget->getX();
            const int rely = my - widget->getY();
            return (! widget->getChildrenArea().isPointInRect(relx, rely));  // This means the mouse is
                // in the scroll bar, as opposed to the "children area" of the scroll area (which would be the 
                // message window itself).
        }
    }

    return false;
}

void LocalDisplay::clearTutorialWindow()
{
    // clears any popup present.
    if (!tutorial_popups.empty()) {
        tutorial_popups.pop_front();
        if (tutorial_popups.empty()) {
            tutorial_selected_window = tutorial_windows.size() - 1;
            updateTutorialWidget();
        }
        if (tutorial_popups.empty() && won[0] && !ready_msg_sent) {
            knights_client.readyToEnd();
            ready_msg_sent = true;
        }
    }
}

std::string LocalDisplay::replaceSpecialChars(std::string msg) const
{
    for (int i = 0; i < msg.size(); ++i) {
        if (msg[i] == '^') {
            msg.replace(i, 1, "\n\n");
        } else if (msg[i] == '%' && i+1 < msg.size()) {
            switch (msg[i+1]) {
            case 'A':
                msg.replace(i, 2, action_string);
                break;
            case 'L':
                msg.replace(i, 2, left_string);
                break;
            case 'R':
                msg.replace(i, 2, right_string);
                break;
            case 'U':
                msg.replace(i, 2, up_string);
                break;
            case 'D':
                msg.replace(i, 2, down_string);
                break;
            case 'S':
                msg.replace(i, 2, suicide_string);
                break;
            case 'M':
                if (left_string == "LEFT" && right_string == "RIGHT" && up_string == "UP" && down_string == "DOWN") { // a bit nasty, I know...
                    msg.replace(i, 2, "ARROW KEYS");
                } else if (left_string == "A" && right_string == "D" && up_string == "W" && down_string == "S") {
                    msg.replace(i, 2, "WASD keys");
                } else {
                    msg.replace(i, 2, up_string + ", " + left_string + ", " + down_string + " and " + right_string + " keys");
                }
            }
        }
    }
    return msg;
}

void LocalDisplay::setupFonts(GfxManager &gm)
{
    // setup "my_font" (used for chat, and player-name captions in observer mode)
    // TODO better font loading method?
    if (!my_font) {
        gm.setFontSize(14);
        my_font = gm.getFont();
    }
    if (!txt_font) {
        txt_font = my_font; // TODO resize dynamically based on screen size?
    }
    if (!gui_font) {
        gui_font.reset(new Coercri::CGFont(my_font));
        gm.setFontSize(13);
        gui_small_font.reset(new Coercri::CGFont(gm.getFont()));
    }
}

void LocalDisplay::focusGained(const gcn::Event &event)
{
    if (event.getSource() == chat_field.get()) {
        activateChatField();
    }
}

void LocalDisplay::focusLost(const gcn::Event &event)
{
    if (event.getSource() == chat_field.get()) {
        deactivateChatField();
    }
}

void LocalDisplay::cycleObsPlayer(int which, int delta)
{
    if (which == 0 || which == 1) {
        curr_obs_player[which] = GetNextPlayer(curr_obs_player[which], delta, obs_visible);
    }
}

int LocalDisplay::drawNormal(Coercri::GfxContext &gc, GfxManager &gm,
                             int vp_x, int vp_y, int vp_width, int vp_height)
{
    return draw(gc, gm, curr_obs_player[0], vp_x, vp_y, vp_width, vp_height, 0);
}

int LocalDisplay::drawSplitScreen(Coercri::GfxContext &gc, GfxManager &gm,
                                  int vp_x, int vp_y, int vp_width, int vp_height)
{
    draw(gc, gm, 0, vp_x, vp_y, vp_width/2, vp_height, -1);
    return draw(gc, gm, 1, vp_x + vp_width/2, vp_y, vp_width/2, vp_height, 1);
}

int LocalDisplay::drawObs(Coercri::GfxContext &gc, GfxManager &gm,
                          int vp_x, int vp_y, int vp_width, int vp_height)
{
    draw(gc, gm, curr_obs_player[0], vp_x, vp_y, vp_width/2, vp_height, 0);
    return draw(gc, gm, curr_obs_player[1],
                vp_x + vp_width/2, vp_y, vp_width/2, vp_height, 0);
}

int LocalDisplay::draw(Coercri::GfxContext &gc, GfxManager &gm, 
                       int player_num,
                       int vp_x, int vp_y, int vp_width, int vp_height,
                       int bias)
{
    if (vp_width <= 0 || vp_height <= 0) return 0;

    if (player_num < 0 || player_num >= dungeon_view.size()) {
        throw UnexpectedError("bad player num in LocalDisplay");
    }

    setupFonts(gm);

    // work out scale factors
    const int obs_margin = 4 + vp_height/300;
    const int obs_caption_height = my_font->getTextHeight() + 2*obs_margin;    

    const int height_minus_obs_caption = observer_mode ? vp_height - obs_caption_height : vp_height;

    if (height_minus_obs_caption <= 0) return 0;
    
    const float ideal_x_scale = float(vp_width) / float(ref_vp_width);
    const float ideal_y_scale = float(height_minus_obs_caption) / float(ref_vp_height);

    float x_scale, y_scale, dummy;
    gm.getGfxResizer()->roundScaleFactor(ideal_x_scale, x_scale, dummy);
    gm.getGfxResizer()->roundScaleFactor(ideal_y_scale, y_scale, dummy);

    const float scale = std::min(x_scale, y_scale);


    // work out dungeon and status area sizes
    const int pixels_per_square = int(ref_pixels_per_square * scale);
    const int dungeon_width = pixels_per_square * dungeon_tiles_x;
    const int dungeon_height = pixels_per_square * dungeon_tiles_y;
    const float dungeon_scale_factor = float(pixels_per_square) / float(ref_pixels_per_square);

    if (tutorial_widget) tutorial_widget->setScaleFactor(dungeon_scale_factor);

    int status_area_width, status_area_height;
    status_display[player_num]->getSize(scale, status_area_width, status_area_height);

    const int excess_height = std::max(0, vp_height - dungeon_height - status_area_height);
    const int dungeon_area_height = dungeon_height + (observer_mode ? 0 : (excess_height / 2));

    const int dungeon_excess_width = std::max(0, vp_width - dungeon_width);
    const int status_area_excess_width = std::max(0, vp_width - status_area_width);
    
    // work out dungeon position
    int dungeon_x;
    const int dungeon_margin = bias == 0 ? dungeon_excess_width/2 : dungeon_excess_width/5;
    dungeon_x = vp_x + (bias <= 0 ? dungeon_margin : vp_width - dungeon_width - dungeon_margin);
    const int dungeon_y = vp_y + (observer_mode ? obs_caption_height : (excess_height / 4));

    // work out status area position
    int status_area_x;
    const int status_area_margin = bias == 0 ? status_area_excess_width/2 : status_area_excess_width/5;
    status_area_x = vp_x + (bias <= 0 ? status_area_margin : vp_width - status_area_width - status_area_margin);
    const int status_area_y = vp_y + dungeon_area_height + (observer_mode ? obs_caption_height : (excess_height/4));

    // work out action bar position
    const int action_bar_x = dungeon_x + Round(ref_action_bar_left * scale);
    const int action_bar_y = status_area_y + Round(ref_action_bar_top * scale);    
    
    // Set clip rectangle
    // (this is to stop anything accidentally being drawn on the opponent's side of the screen).
    if (tutorial_popups.empty()) {
        Coercri::Rectangle clip_rect(vp_x, vp_y, vp_width, vp_height);
        gc.setClipRectangle(clip_rect);
    }

    // work out what we are drawing
    bool draw_in_game_screen = true;
    bool draw_winner_loser_screen = false;
    if (won[player_num] || lost[player_num]) {
        if (game_over_time[player_num] < 0) {
            game_over_time[player_num] = time;
        }
        if (time >= game_over_time[player_num] + game_over_t1) {
            draw_in_game_screen = false;
        }
        if (time > game_over_time[player_num] + game_over_t2) {
            draw_winner_loser_screen = true;
        }
        if (tutorial_mode && time > game_over_time[player_num] + game_over_t3 && !have_sent_end_msg) {
            have_sent_end_msg = true;
            TutorialWindow win;
            win.popup = true;
            win.title_latin1 = "Quest Complete";
            win.msg_latin1 = "Congratulations! You have completed your quest."
                "^If you now want to play a proper game of Knights, you have the following options:"
                "^* Multiplayer mode. Knights is designed first and foremost as a multiplayer game, and you can "
                "play online, on a LAN, or in two-player split screen mode."
                "^* Single Player mode. This is a good way to learn about the different types of quests and so on. Alternatively, if you "
                "want a more challenging game, you can add a time limit and try to complete quests against the clock."
                "^That concludes the tutorial. We will now return to the main menu.";
            popUpWindow(std::vector<TutorialWindow>(1,win));
            tutorial_selected_window = -1;
            updateTutorialWidget();
        }
    }

    // set an appropriate font size, based on dungeon_scale_factor
    int font_size = Round(ref_font_size * dungeon_scale_factor);
    gm.setFontSize(std::max(min_font_size, font_size));


    // draw the player name if necessary.
    if (observer_mode && my_font) {
        const int w = my_font->getTextWidth(names[player_num]);
        int x;
        if (draw_winner_loser_screen) {
            x = vp_x + vp_width/2;  // centre of window
        } else {
            x = dungeon_x + dungeon_width/2;  // centre of dungeon area
        }
        x -= w/2;
        gc.drawText(x, vp_y + obs_margin, *my_font, names[player_num], Coercri::Color(255,255,255));
    }

    // Draw Winner/Loser screen
    if (draw_winner_loser_screen) {
        boost::shared_ptr<ColourChange> my_colour_change = dungeon_view[player_num]->getMyColourChange();

        const Graphic * image = 0;
        
        if (won[player_num]) {
            if (winner_image) {
                image = winner_image;
            } else {
                gc.drawText(vp_x + 10, 50, *gm.getFont(), UTF8String::fromUTF8("WINNER"), Coercri::Color(255,255,255));
            }
        } else {
            if (loser_image) {
                image = loser_image;
            } else {
                gc.drawText(vp_x + 10, 50, *gm.getFont(), UTF8String::fromUTF8("LOSER"), Coercri::Color(255,255,255));
            }
        }

        if (image) {
            int width, height;
            gm.loadGraphic(*image);
            gm.getGraphicSize(*image, width, height);
            const int new_width = int(scale*width);
            const int new_height = int(scale*height);
            const int th = gm.getFont()->getTextHeight();
            const int image_y = vp_y + (vp_height - new_height)/2 - th;
            gm.drawTransformedGraphic(gc, vp_x + (vp_width - new_width)/2, image_y,
                                      *image, new_width, new_height,
                                      my_colour_change ? *my_colour_change : ColourChange());

            if (!ready_msg_sent && !observer_mode && !tutorial_mode) {
                const UTF8String msg = UTF8String::fromLatin1(config_map.getString("game_over_msg"));
                const int x = vp_x + vp_width/2 - gm.getFont()->getTextWidth(msg)/2;
                gc.drawText(x,
                            image_y + new_height + 2*th,
                            *gm.getFont(),
                            msg,
                            Coercri::Color(config_map.getInt("game_over_r"), 
                                           config_map.getInt("game_over_g"), 
                                           config_map.getInt("game_over_b")));
            }
        }
    }

    // Draw In-Game Screen
    if (draw_in_game_screen) {

        // Check whether screen should flash
        const bool screen_flash = (time >= flash_screen_start[player_num] 
                                  && time <= flash_screen_start[player_num] + config_map.getInt("screen_flash_time"));

        // Draw the coloured background if screen is flashing
        if (screen_flash) {
            Coercri::Color col(config_map.getInt("screen_flash_r"), 
                               config_map.getInt("screen_flash_g"), 
                               config_map.getInt("screen_flash_b"));
            Coercri::Rectangle rect(dungeon_x, dungeon_y, dungeon_width, dungeon_height);
            gc.fillRectangle(rect, col);
        }
        
        // Draw dungeon view. Pass in screen_flash as this only draws
        // part of the dungeon when screen is flashing.
        int room_tl_x, room_tl_y;
        dungeon_view[player_num]->draw(gc, gm, screen_flash, dungeon_x, dungeon_y, dungeon_width,
                                       dungeon_height, pixels_per_square, dungeon_scale_factor, *txt_font,
                                       observer_mode,  // Show my knight name in obs mode. In normal mode, show only names of opponents, not myself.
                                       room_tl_x, room_tl_y);

        // Work out highlighting for action bar
        const int highlight_slot = mouse_over_slot[player_num];
        const bool strong_highlight = lmb_down[player_num];
        
        // Draw action bar
        if (action_bar) {
            action_bar->draw(gc, gm, time, scale, action_bar_x, action_bar_y, highlight_slot, strong_highlight);

            if (action_bar_tool_tips || tutorial_mode) {
                // Draw action bar tool tips
                // NOTE: We currently assume that the action bar has the height of one dungeon tile, i.e. that
                // the action bar height is pixels_per_square
                const int mos = mouse_over_slot[player_num];
                if (mos >= 0 && mos < NUM_ACTION_BAR_SLOTS) {
                    const UserControl *ctrl = slot_controls[player_num][mos];
                    const UTF8String name = UTF8String::fromLatin1(ctrl ? ctrl->getName() : "");
                    gc.drawText(action_bar_x, action_bar_y + pixels_per_square + 2, *txt_font, name, Coercri::Color(255,255,255));
                }
            }
        }

        // Draw status area
        status_display[player_num]->draw(gc, gm, time, scale, status_area_x, status_area_y,
                                         dungeon_view[player_num]->aliveRecently(),
                                         *mini_map[player_num], time_limit_string);

    }

    // Draw Tutorial Popup Window
    if (!tutorial_popups.empty()) {
        const TutorialWindow& win = tutorial_popups.front();

        // replace 'control characters' in the message
        std::string msg_latin1 = replaceSpecialChars(win.msg_latin1);
        msg_latin1 += "\n\nPress SPACE to continue.";

        // figure out size/position of the window
        const int text_height = gm.getFont()->getTextHeight();
        const int title_height = std::max(text_height, pixels_per_square);
        const int title_yofs = pixels_per_square / 4;
        const int msg_margin = gm.getFont()->getTextWidth(UTF8String::fromUTF8("n"));
        const int bottom_margin = text_height;

        int t_x, t_y, t_w, t_h;

        for (int i = 0; i < 2; ++i) {
            t_w = std::max(int(gc.getWidth() * (i==0 ? 0.45f : 0.9f)),
                           gm.getFont()->getTextWidth(UTF8String::fromUTF8("nCongratulations! You have completed your quest.n")));
            if (t_w > gc.getWidth()-6) t_w = gc.getWidth()-6;
            t_x = (gc.getWidth() - t_w)/2;

            CountingPrinter counter(*gm.getFont());
            TextFormatter tf2(counter, t_w - msg_margin*2, false);
            tf2.printString(msg_latin1);

            t_h = title_yofs + title_height + text_height + text_height * counter.count + bottom_margin;
            t_y = (gc.getHeight() - t_h)/2;

            if (t_y >= 0) break;
        }
            
        Coercri::Rectangle t_rect(t_x, t_y, t_w, t_h);
        
        // draw the window itself
        const Coercri::Color col3(config_map.getInt("pause3r"), config_map.getInt("pause3g"), config_map.getInt("pause3b"));
        gc.fillRectangle(t_rect, Coercri::Color(0, 0, 0, config_map.getInt("pausealpha")));
        gc.drawRectangle(t_rect, col3);

        // draw the title block
        const UTF8String title = UTF8String::fromLatin1(win.title_latin1);
        const int title_width = gm.getFont()->getTextWidth(title + UTF8String::fromUTF8("9/9"));
        const int x_margin = 20;
        const int title_xofs = std::max(0, (t_w - title_width)/2);
        const int title_txt_yofs = title_yofs + std::max(0, title_height - text_height)/2;
        gc.drawText(t_x + title_xofs, t_y + title_txt_yofs, *gm.getFont(), title, col3);

        // draw the message
        MyPrinter pr(gc, *gm.getFont(), col3, t_x + msg_margin, t_y + title_yofs + title_height + text_height);
        TextFormatter tf(pr, t_w - msg_margin*2, false);
        tf.printString(msg_latin1);
    }

    // Handle fade to black if necessary.
    // This is done in a quick-n-dirty way, by drawing a huge
    // semi-transparent black rectangle over the whole screen...
    if (won[player_num] || lost[player_num]) {
        float alpha = 0.0f;
        const int delta = time - game_over_time[player_num];
        if (delta <= 0) {
            alpha = 0.0f;
        } else if (delta < game_over_t1) {
            alpha = float(delta) / float(game_over_t1);
        } else if (delta < game_over_t2) {
            alpha = 1.0f;
        } else if (delta < game_over_t3) {
            alpha = 1.0f - float(delta - game_over_t2) / float(game_over_t3 - game_over_t2);
        } else {
            alpha = 0.0f;
        }
        if (alpha != 0.0f) {
            Coercri::Rectangle rect(vp_x, vp_y, vp_width, vp_height);
            gc.fillRectangle(rect, Coercri::Color(0, 0, 0, int(alpha*255)));
        }
    }

    gc.clearClipRectangle();

    return status_area_y + status_area_height - vp_y;
}

void LocalDisplay::drawPauseDisplay(Coercri::GfxContext &gc, GfxManager &gm, 
                                    int vp_x, int vp_y, int vp_width, int vp_height,
                                    bool is_paused, bool blend)
{
    if (vp_width <= 0 || vp_height <= 0) return;

    const Coercri::Color col1(config_map.getInt("pause1r"), config_map.getInt("pause1g"), config_map.getInt("pause1b"));
    const Coercri::Color col2(config_map.getInt("pause2r"), config_map.getInt("pause2g"), config_map.getInt("pause2b"));
    const Coercri::Color col3(config_map.getInt("pause3r"), config_map.getInt("pause3g"), config_map.getInt("pause3b"));
    const Coercri::Color col4(config_map.getInt("pause4r"), config_map.getInt("pause4g"), config_map.getInt("pause4b"));

    const int th = gm.getFont()->getTextHeight();

    const int border = 16;

    // if split screen mode then we have to resize the font here, because draw() won't have been called
    if (!blend) {
        const float x_scale = float(vp_width) / float(ref_vp_width);
        const float y_scale = float(vp_height) / float(ref_vp_height);
        const float scale = std::min(x_scale, y_scale);
        gm.setFontSize(std::max(min_font_size, Round(ref_font_size * scale)));
    }
    
    // work out height
    int pause_height = (9*th/2) + (menu_strings ? menu_strings->size() : 0)*th;

    // work out width
    int maxw1 = gm.getFont()->getTextWidth(UTF8String::fromUTF8("ESC:"));
    int maxw2 = gm.getFont()->getTextWidth(UTF8String::fromUTF8("Return to game"));
    if (menu_strings) {
        for (int i = 0; i < menu_strings->size(); ++i) {
            const std::pair<std::string, std::string> & p = (*menu_strings)[i];
            maxw1 = std::max(maxw1, gm.getFont()->getTextWidth(UTF8String::fromLatin1(p.first + ":")));
            maxw2 = std::max(maxw2, gm.getFont()->getTextWidth(UTF8String::fromLatin1(p.second)));
        }
    }

    const int horiz_gap = 5;
    const int pause_width = maxw1 + maxw2 + horiz_gap*2;

    // work out position
    const int x0 = vp_x + std::max(0, vp_width - pause_width) / 2;
    const int y0 = vp_y + std::max(0, vp_height - pause_height) / 3;
    int y = y0;

    const int bx = std::max(vp_x, x0 - border);
    const int by = std::max(vp_y, y0 - border);
    const int bw = std::min(vp_width - (bx - vp_x), pause_width + border*2);
    const int bh = std::min(vp_height - (by - vp_y), pause_height + border*2);

    // set a clip rectangle, just in case
    Coercri::Rectangle clip_rect(bx, by, bw, bh);
    gc.setClipRectangle(clip_rect);
    
    // darken visible area (if requested), also draw border.
    if (blend) {
        gc.fillRectangle(clip_rect, Coercri::Color(0, 0, 0, config_map.getInt("pausealpha")));
        gc.drawRectangle(clip_rect, col4);
    }

    // draw "header" lines
    const UTF8String pstr = UTF8String::fromUTF8(is_paused ? "GAME PAUSED" : "KNIGHTS");
    const int pwidth = gm.getFont()->getTextWidth(pstr);
    
    gc.drawText(vp_x + vp_width/2 - pwidth/2, y, *gm.getFont(), pstr, col1);
    y += th;
    y += th/2;
    
    DrawKeyLine(vp_x + vp_width/2, y, col1, col2, gc, gm, "ESC", "Return to game");
    y += th;
    DrawKeyLine(vp_x + vp_width/2, y, col1, col2, gc, gm, "Q", "Quit");
    y += th;
    y += th;

    if (menu_strings) {

        for (int i = 0; i < menu_strings->size(); ++i) {
            std::pair<std::string, std::string> p = (*menu_strings)[i];
            if (!p.first.empty()) p.first += ":";

            const int w1 = gm.getFont()->getTextWidth(UTF8String::fromLatin1(p.first));
            const int w2 = gm.getFont()->getTextWidth(UTF8String::fromLatin1(p.second));

            const int left_bound = w1 + horiz_gap;
            const int right_bound = pause_width - w2;
            int x = maxw1 + horiz_gap*2;
            // Move left if necessary, so as not to run off right side of screen.
            if (x > right_bound) x = right_bound;
            // Move right if necessary, so as not to overlap left-hand text.
            if (x < left_bound) x = left_bound;
                
            gc.drawText(x0,     y, *gm.getFont(), UTF8String::fromLatin1(p.first), col3);
            gc.drawText(x0 + x, y, *gm.getFont(), UTF8String::fromLatin1(p.second), col4);
            y += th;
        }
    }

    gc.clearClipRectangle();
}

void LocalDisplay::updateGui(GfxManager &gm, int vp_x, int vp_y, int vp_width, int vp_height, bool horiz_split)
{
    setupFonts(gm);
    
    if (chat_updated) {
        chat_updated = false;
        if (chat_scrollarea.get()) {
            // auto scroll chat window to bottom
            chat_listbox->adjustSize();
            gcn::Rectangle rect(0, chat_listbox->getHeight() - chat_listbox->getFont()->getHeight(),
                                1, chat_listbox->getFont()->getHeight());
            chat_scrollarea->showWidgetPart(chat_listbox.get(), rect);
        }
    }

    if (!status_display.empty() && status_display[0]->needQuestHintUpdate()) {
        int old_size = quest_rqmts_list.getNumberOfElements();

        quest_rqmts_list.clear();
        int prev_group = -999;

        const std::vector<std::string> & hints = status_display[0]->getQuestHints();
        for (std::vector<std::string>::const_iterator it = hints.begin(); it != hints.end(); ++it) {
            quest_rqmts_list.add(*it);
        }

        if (quest_rqmts_list.getNumberOfElements() != old_size) force_setup_gui = true;
    }

    // Reset the gui positions/sizes if the viewport has changed.
    if (vp_x != prev_gui_x || vp_y != prev_gui_y 
    || vp_width != prev_gui_width || vp_height != prev_gui_height
    || force_setup_gui) {

        force_setup_gui = false;
        
        prev_gui_x = vp_x;
        prev_gui_y = vp_y;
        prev_gui_width = vp_width;
        prev_gui_height = vp_height;

        int player_list_x, player_list_y, player_list_width, player_list_height;
        int chat_area_x, chat_area_y, chat_area_width, chat_area_height;
        int quest_rqmts_x, quest_rqmts_y, quest_rqmts_width, quest_rqmts_height;
        
        if (horiz_split) {
            // Want horizontal split (i.e. "left" and "right" panels)

            // Work out where to draw the player list (left)
            const int margin = 6;
            player_list_x = vp_x + margin;
            player_list_y = vp_y + margin;
            player_list_width = vp_width/2 - 2*margin;
            player_list_height = vp_height - 2*margin;

            // Work out where to draw the chat area (right)
            const int chat_margin = 6;
            chat_area_x = player_list_x + player_list_width + 2*margin;
            chat_area_y = player_list_y;
            chat_area_width = player_list_width;
            chat_area_height = player_list_height;

            // Quest requirements not shown in this mode
            quest_rqmts_x = quest_rqmts_y = quest_rqmts_width = quest_rqmts_height = 0;
            
        } else {
            // Want vertical split (i.e. stack player list, quest requirements, chat area vertically.)
            
            // Work out where to draw the player list / tutorial window (top)
            const int player_list_margin = 6;
            player_list_x = vp_x + player_list_margin;
            player_list_y = vp_y + player_list_margin;
            player_list_width = vp_width - 2*player_list_margin;

            player_list_height = gui_font->getHeight() * (tutorial_mode ? config_map.getInt("tutorial_lines")
                                                                        : config_map.getInt("player_list_lines"));

            const int third_height = vp_height/3 - 2*player_list_margin;
            if (player_list_height > third_height) player_list_height = third_height;

            const int player_list_bottom = player_list_y + player_list_height + player_list_margin;

            // Work out where to draw the quest requirements (middle)
            quest_rqmts_x = player_list_x;
            quest_rqmts_y = player_list_bottom + player_list_margin;
            quest_rqmts_width = player_list_width;

            if (quest_rqmts_minimized) {
                quest_rqmts_height = gui_font->getHeight() + 8;
            } else {
                quest_rqmts_height = gui_font->getHeight() * (quest_rqmts_list.getNumberOfElements() + 
                    config_map.getInt("quest_rqmts_extra_lines"));
                // the +quest_rqmts_extra_lines just gives it a bit of extra black space underneath.
            }
            
            if (quest_rqmts_height > third_height) quest_rqmts_height = third_height;

            const int quest_rqmts_bottom = quest_rqmts_y + quest_rqmts_height + player_list_margin;
            
            // Work out where to draw the chat area (bottom)
            const int chat_area_margin = player_list_margin;
            chat_area_x = vp_x + chat_area_margin;
            chat_area_y = quest_rqmts_bottom + chat_area_margin;
            chat_area_width = vp_width - 2*chat_area_margin;
            chat_area_height = vp_height - quest_rqmts_bottom - 2*chat_area_margin;
        }
        
        setupGui(chat_area_x, chat_area_y, chat_area_width, chat_area_height,
                 player_list_x, player_list_y, player_list_width, player_list_height,
                 quest_rqmts_x, quest_rqmts_y, quest_rqmts_width, quest_rqmts_height,
                 gm);
    }

    if (!container.isVisible()) container.setVisible(true);
}

void LocalDisplay::hideGui()
{
    if (container.isVisible()) container.setVisible(false);
}

void LocalDisplay::playSounds(SoundManager &sm)
{
    const int p1 = curr_obs_player[0];
    const int p2 = curr_obs_player[1];
    
    for (std::vector<MySound>::iterator it = sounds.begin(); it != sounds.end(); ++it) {

        bool found = false;
        for (int p = 0; p < nplayers; ++p) {
            if (it->plyr[p] && (p == p1 || p == p2)) {
                found = true;
                break;
            }
        }

        if (found) {
            sm.playSound(*it->sound, it->frequency);
        }
    }
    sounds.clear();
}

const UserControl * LocalDisplay::readControl(int plyr, int mx, int my, bool mleft, bool mright)
{
    if (observer_mode) return 0;
    
    if (plyr != 0 && plyr != 1) {
        throw UnexpectedError("bad player number in LocalDisplay");
    }
    
    // set up some constants
    const int menu_delay = config_map.getInt("menu_delay"); // cutoff btwn 'tapping' and 'holding' fire.
    const bool approached = dungeon_view[plyr]->isApproached();
    
    // get current controller state
    ControllerState ctrlr;
    if (plyr == 0 && controller1) {
        controller1->get(ctrlr);
        if (!action_bar) {
            // Not using action bar, which means we ARE using suicide keys.
            // Translate suicide key to a fake click on slot zero.
            mleft = mright = ctrlr.suicide;
            mx = my = -1;
        }
    } else if (plyr == 1 && controller2) {
        controller2->get(ctrlr);
        // Controller 2 always uses suicide key (rather than action bar).
        mleft = ctrlr.suicide;
        mright = false;
        mx = my = -1;
    }

    const bool old_mleft = lmb_down[plyr];

    // Left mouse button: Action bar and pickup/drop/open/close.
    
    // Update mouse_over_slot
    // (We don't update if the mouse is held down, this makes the selected control "stick" until the 
    // player releases the mouse.)
    if (!(mleft && old_mleft)) {  // mouse has not been down for >1 frame yet
        mouse_over_slot[plyr] = action_bar ? action_bar->mouseHit(mx, my) : NO_SLOT; // see if mouse is over any action bar icon

        // If mouse is over no slot, and is not over a gui control, then a mouse click should be treated as a "tap"
        if (mouse_over_slot[plyr] == NO_SLOT && !mouseInGuiControl(mx,my)) {
            mouse_over_slot[plyr] = TAP_SLOT;
        }

        // If mouse is over a slot w/ no control, then set mouse_over_slot to null.
        // This prevents empty slots being highlighted on mouse-over.
        if (mouse_over_slot[plyr] != NO_SLOT && slot_controls[plyr][mouse_over_slot[plyr]] == 0) {
            mouse_over_slot[plyr] = NO_SLOT;
        }

        // Suicide key is hard wired to slot zero
        if (ctrlr.suicide) mouse_over_slot[plyr] = 0;
    }

    // If mouse is over suicide slot then 'cancel' a left-only click (need left-AND-right click to suicide)
    if (mouse_over_slot[plyr] == 0 && !mright) mleft = false;
    
    // Update lmb_down
    lmb_down[plyr] = mleft;    
    
    // Update chat_flag
    if (!mleft) chat_flag[plyr] = false;
    if (chat_field && chat_field->isFocused() && mouse_over_slot[plyr] == TAP_SLOT) chat_flag[plyr] = true;

    // Right mouse button: Attack.
    if (mright && !mleft) {
        if (chat_field) deactivateChatField();
        return standard_controls[SC_ATTACK_NO_DIR];
    }
    
    // If left-mouse held then decide which control to return (action bar or 'tap' control).
    if (mleft && !chat_flag[plyr]) {
        // find out what is in the mouse over slot (or tap_control if mouse is not over anything)
        const UserControl * ctrl = 0;
        const int slot = mouse_over_slot[plyr];
        if (slot != NO_SLOT) {
            ctrl = slot_controls[plyr][slot];
        }
        
        // Don't repeat a control for more than one frame, unless it is continuous.
        if (ctrl && (!old_mleft || ctrl->isContinuous())) {
            // Return this control
            return ctrl;
        }
    }

    // Now the 'old' controller, which includes anything using the old style 'fire' or 'menu' buttons,
    // as well as plain ordinary movement (WASD keys).

    // NOTE: These are ignored if the chat box is active.
    if (chat_field && chat_field->isFocused()) return 0;

    // work out 'long' fire state
    enum FireState { NONE, TAPPED, HELD } long_fire = NONE;
    if (ctrlr.fire) {
        if (fire_start_time[plyr] == 0) {
            // fire_start_time: set to time when fire was pressed down (or 0 if fire is
            // currently released)
            // attack_mode: if fire+direction is used to attack, then attack_mode is set true,
            // which disables menus and 'tapping'.
            fire_start_time[plyr] = time;
            attack_mode[plyr] = false;
        }
        if (time >= fire_start_time[plyr] + menu_delay && !attack_mode[plyr]) {
            long_fire = HELD;
        }
    } else {
        // do a 'tap' if allowed
        if (fire_start_time[plyr] > 0 && fire_start_time[plyr] < time + menu_delay && !attack_mode[plyr]) {
            long_fire = TAPPED;
        }

        // reset fire state when fire is released
        fire_start_time[plyr] = 0;
        attack_mode[plyr] = false;

        // reset allow_menu_open when they let go of fire when the menu is closed.
        if (!status_display[plyr]->isMenuOpen()) allow_menu_open[plyr] = true;
    }
    
    if (status_display[plyr]->isMenuOpen()) {
        // Menu open

        if (long_fire != HELD) {
            // close the menu
            status_display[plyr]->setMenuOpen(false);
            menu_null[plyr] = M_OK;
        } else if (approached != approached_when_menu_was_opened[plyr]) {
            // This traps the situation where we have picked a lock and now start moving towards the door.
            // We want to automatically close the menu in this case, so that the knight is ready to attack
            // whatever is beyond the door. Also turn off allow_menu_open until fire is released.
            status_display[plyr]->setMenuOpen(false);
            allow_menu_open[plyr] = false;
        } else {
            if (menu_null[plyr]==M_NULL && (ctrlr.centred || ctrlr.dir != menu_null_dir[plyr])) {
                // Cancel null dir.
                menu_null[plyr] = M_OK;
            }
            if (!ctrlr.centred && (menu_null[plyr]!=M_NULL || ctrlr.dir != menu_null_dir[plyr])) {
                // A menu option has been selected
                const UserControl *c = menu_control[plyr][ctrlr.dir];
                if (c) {
                    // stop the action being executed more than once in a row.
                    // (continuous actions will be stopped only if the control disappears,
                    // which is what M_CTS is for.)
                    menu_null[plyr] = (c->isContinuous()? M_CTS: M_NULL);
                    menu_null_dir[plyr] = ctrlr.dir;
                }
                return c;
            }
        }

    } else if (approached) {
        // Approached, and menu closed
        if ((ctrlr.centred || ctrlr.dir != my_facing[plyr]) && !ctrlr.fire) {
            // Withdraw if controller is no longer pointing in the approach direction
            // and fire is released.
            return standard_controls[SC_WITHDRAW];
        } else {
            if (long_fire == HELD && allow_menu_open) {
                status_display[plyr]->setMenuOpen(true);
                approached_when_menu_was_opened[plyr] = true;
                menu_null[plyr] = M_NULL;
                menu_null_dir[plyr] = my_facing[plyr];
            } else if (long_fire == TAPPED) { 
                return tap_control[plyr];
            }
        }
    } else {
        // Not approached, and menu closed
        if (ctrlr.centred) {
            if (long_fire == HELD && allow_menu_open[plyr]) {
                status_display[plyr]->setMenuOpen(true);
                approached_when_menu_was_opened[plyr] = false;
                menu_null[plyr] = M_OK;
            } else if (long_fire == TAPPED) {
                return tap_control[plyr];
            }
        } else {
            if (ctrlr.fire) {
                attack_mode[plyr] = true;
                return standard_controls[SC_ATTACK + ctrlr.dir];
            } else {
                my_facing[plyr] = ctrlr.dir;
                return standard_controls[SC_MOVE + ctrlr.dir];
            }
        }
    }

    // No commands coming from the controller at present.
    return 0;
}

DungeonView & LocalDisplay::getDungeonView(int plyr)
{
    if (plyr >= 0 && plyr < dungeon_view.size()) {
        return *dungeon_view[plyr];
    } else {
        throw UnexpectedError("bad player number in LocalDisplay");
    }
}

MiniMap & LocalDisplay::getMiniMap(int plyr)
{
    if (plyr >= 0 && plyr < mini_map.size()) {
        return *mini_map[plyr];
    } else {
        throw UnexpectedError("bad player number in LocalDisplay");
    }
}

StatusDisplay & LocalDisplay::getStatusDisplay(int plyr)
{
    if (plyr >= 0 && plyr < status_display.size()) {
        return *status_display[plyr];
    } else {
        throw UnexpectedError("bad player number in LocalDisplay");
    }
}

void LocalDisplay::playSound(int plyr, const Sound &sound, int frequency)
{
    if (plyr < 0 || plyr >= nplayers) {
        throw UnexpectedError("bad player number in LocalDisplay");
    } else {
        // see if we have this sound already. this prevents the same sound
        // being played twice (and therefore at double volume!) in the
        // split screen mode when both knights are in the same room.
        for (std::vector<MySound>::iterator it = sounds.begin(); it != sounds.end(); ++it) {
            if (it->sound == &sound && it->frequency == frequency) {
                it->plyr[plyr] = true;
                return;
            }
        }
        
        // otherwise, add it to the list
        MySound s;
        s.sound = &sound;
        s.frequency = frequency;
        s.plyr.resize(nplayers);  // initializes all to false
        s.plyr[plyr] = true;
        sounds.push_back(s);
    }
}

void LocalDisplay::winGame(int plyr)
{
    if (plyr < 0 || plyr >= nplayers) {
        throw UnexpectedError("bad player number in LocalDisplay");
    } else {
        won[plyr] = true;
    }
}

void LocalDisplay::loseGame(int plyr)
{
    if (plyr < 0 || plyr >= nplayers) {
        throw UnexpectedError("bad player number in LocalDisplay");
    } else {
        lost[plyr] = true;
    }
}

void LocalDisplay::setAvailableControls(int plyr, const std::vector<std::pair<const UserControl*, bool> > &controls)
{
    if (observer_mode) return;  // observers can't control the game, so nothing to do for them...

    if (plyr < 0 || plyr >= nplayers || plyr >= 2) {
        throw UnexpectedError("bad player number in LocalDisplay::setAvailableControls");
    }
    
    int tap_pri = 0;
    const UserControl * prev_menu_control[4];

    tap_control[plyr] = 0;
    for (int i = 0; i < 4; ++i) {
        prev_menu_control[i] = menu_control[plyr][i];
        menu_control[plyr][i] = 0;
    }

    // All controls in 'controls' are available to our
    // knight. So put them into menu_control or tap_control as
    // appropriate.

    status_display[plyr]->clearMenuGraphics();

    for (std::vector<std::pair<const UserControl*, bool> >::const_iterator ctrl = controls.begin();
    ctrl != controls.end(); ++ctrl) {
        // Find the (primary) control with the highest tap priority:
        // (NB control is primary iff ctrl->second == true)
        if (ctrl->first->getTapPriority() > tap_pri && ctrl->second) {
            tap_pri = ctrl->first->getTapPriority();
            tap_control[plyr] = ctrl->first;
        }

        // Menu controls
        if (ctrl->first->getMenuGraphic() != 0
        && ((ctrl->first->getMenuSpecial() & UserControl::MS_NO_MENU)==0)) {
            MapDirection d = ctrl->first->getMenuDirection();
            // If no control in direction d, or if the control in direction d has
            // property MS_WEAK, then save ctrl->first into menu position d.
            if (menu_control[plyr][d] == 0 ||
            (menu_control[plyr][d]->getMenuSpecial() & UserControl::MS_WEAK)) {
                setMenuControl(plyr, d, ctrl->first, prev_menu_control[d]);
            }
        }
    }

    // Now try to place controls that could not be put in their preferred position.
    for (std::vector<std::pair<const UserControl *, bool> >::const_iterator ctrl = controls.begin(); 
    ctrl != controls.end(); ++ctrl) {
        if (ctrl->first->getMenuGraphic() != 0
        && ((ctrl->first->getMenuSpecial() & UserControl::MS_NO_MENU)==0)) {
            MapDirection d = ctrl->first->getMenuDirection();
            if (menu_control[plyr][d] != ctrl->first &&
            (ctrl->first->getMenuSpecial() & UserControl::MS_WEAK) == 0) {
                for (int i=0; i<4; ++i) {
                    if (menu_control[plyr][i] == 0) {
                        setMenuControl(plyr, MapDirection(i), ctrl->first, prev_menu_control[i]);
                        break;
                    }
                }
            }
        }
    }

    // Action bar.
    for (int i = 0; i < TOTAL_NUM_SLOTS; ++i) slot_controls[plyr][i] = 0;

    // Put the action bar controls into slot_controls
    for (std::vector<std::pair<const UserControl*, bool> >::const_iterator it = controls.begin(); it != controls.end(); ++it) {
        const int slot = it->first->getActionBarSlot();
        if (slot >= 0 && slot < NUM_ACTION_BAR_SLOTS) {  // It can go in the action bar
            const UserControl * & ctrl = slot_controls[plyr][slot];
            if (!ctrl || ctrl->getActionBarPriority() < it->first->getActionBarPriority()) {
                ctrl = it->first;
            }
        }
    }

    // Put the tap control into slot_controls
    slot_controls[plyr][TAP_SLOT] = tap_control[plyr];

    // Set the action bar graphics.
    const Graphic * gfx[NUM_ACTION_BAR_SLOTS];
    for (int i = 0; i < NUM_ACTION_BAR_SLOTS; ++i) {
        const UserControl *ctrl = slot_controls[plyr][i];
        gfx[i] = ctrl ? ctrl->getMenuGraphic() : 0;
    }
    
    if (action_bar) action_bar->setGraphics(gfx);
}

void LocalDisplay::setMenuControl(int plyr, MapDirection d, const UserControl *ctrl, const UserControl *prev)
{
    if (menu_null[plyr]==M_CTS && menu_null_dir[plyr]==d && ctrl != prev) {
        menu_null[plyr] = M_NULL;
    }
    menu_control[plyr][d] = ctrl;
    status_display[plyr]->setMenuGraphic(d, ctrl ? ctrl->getMenuGraphic() : 0);
}

void LocalDisplay::setMenuHighlight(int plyr, const UserControl *highlight)
{
    if (observer_mode) return; // The menu is not shown for observers.
    
    if (plyr < 0 || plyr >= nplayers || plyr >= 2) {
        throw UnexpectedError("bad player number in LocalDisplay::setMenuHighlight");
    }

    if (highlight) {
        for (int i = 0; i < 4; ++i) {
            if (menu_control[plyr][i] == highlight) {
                status_display[plyr]->setMenuHighlight(MapDirection(i));
                return;
            }
        }
    }
    status_display[plyr]->clearMenuHighlight();
}

void LocalDisplay::flashScreen(int plyr, int delay)
{
    if (plyr < 0 || plyr >= nplayers) {
        throw UnexpectedError("bad player number in LocalDisplay");
    } else {
        flash_screen_start[plyr] = time + delay;
    }
}

void LocalDisplay::gameMsg(int plyr, const std::string &msg, bool is_err)
{
    chat_list.add(msg);
}

void LocalDisplay::popUpWindow(const std::vector<TutorialWindow> &windows)
{
    bool popup = false;
    for (std::vector<TutorialWindow>::const_iterator it = windows.begin(); it != windows.end(); ++it) {
        if (it->popup) {
            tutorial_popups.push_back(*it);
            knights_client.setPauseMode(true);
            popup = true;
        }
        tutorial_windows.push_back(*it);
        if (tutorial_popups.empty() && !popup) tutorial_selected_window = tutorial_windows.size() - 1;
    }
    updateTutorialWidget();
}

void LocalDisplay::updateTutorialWidget()
{
    if (!tutorial_widget) return;

    if (tutorial_selected_window < 0 || tutorial_selected_window >= tutorial_windows.size()) {
        tutorial_widget->wipe();
        tutorial_label->setCaption("");
        setWidgetEnabled(*tutorial_left, false);
        setWidgetEnabled(*tutorial_right, false);
    } else {
        const TutorialWindow &win = tutorial_windows[tutorial_selected_window];
        tutorial_widget->setTitle(win.title_latin1);
        tutorial_widget->setText(replaceSpecialChars(win.msg_latin1));
        tutorial_widget->setGfx(win.gfx, win.cc);

        std::ostringstream str;
        str << (tutorial_selected_window+1) << "/" << tutorial_windows.size();
        tutorial_label->setCaption(str.str());

        setWidgetEnabled(*tutorial_left, tutorial_selected_window > 0);
        setWidgetEnabled(*tutorial_right, tutorial_selected_window < tutorial_windows.size() - 1);
    }

    tutorial_widget->adjustHeight();
}

void LocalDisplay::setWidgetEnabled(gcn::Widget &widget, bool enabled)
{
    widget.setEnabled(enabled);
    widget.setForegroundColor(enabled ? gcn::Color(255,255,255)
        : gcn::Color(0x77, 0x77, 0x55));
}

std::string LocalDisplay::getChatFieldContents() const
{
    if (chat_field && chat_field->getText() != chat_msg) return chat_field->getText();
    else return std::string();
}

bool LocalDisplay::chatFieldSelected() const
{
    return chat_field && chat_field->isFocused();
}

void LocalDisplay::toggleChatMode(bool global)
{
    if (chat_field) {
        if (chat_field->isFocused()) {
            container.requestFocus();
        } else {
            chat_field->requestFocus();

            // if chat was entered by pressing BACKQUOTE, and the chat
            // field is empty, then insert "/t " at the beginning.
            // Otherwise, leave it alone (let the player sort it out).

            if (!global && chat_field->getText().empty()) {
                chat_field->setText("/t ");
                chat_field->setCaretPosition(3);
            }
        }
    }
}

void LocalDisplay::activateChatField()
{
    if (chat_field->getText() == chat_msg) {
        chat_field->setText("");
    }
}

void LocalDisplay::deactivateChatField()
{
    // if they just pressed TAB or BACKQUOTE
    // (i.e. if chat field is "" or "/t ")
    // then clear the chat field, otherwise leave it be
    if (chat_field->getText().empty() || chat_field->getText() == "/t ") {
        chat_field->setText(chat_msg);
    }
}
