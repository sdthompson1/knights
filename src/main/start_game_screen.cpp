/*
 * start_game_screen.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2026.
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

#include "lan_game_screen.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "online_multiplayer_screen.hpp"
#include "start_game_screen.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"
#include "utf8string.hpp"

#include "guichan.hpp"

#include <algorithm>
#include <memory>
using namespace boost;

class StartGameScreenImpl : public gcn::ActionListener, public gcn::MouseListener {
public:
    StartGameScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);
    void mouseEntered(gcn::MouseEvent &e) override;
    void mouseExited(gcn::MouseEvent &e) override;

private:
    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<gcn::Container> outer_container;
    std::unique_ptr<GuiPanel> panel1, panel2;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Label> title;
    std::unique_ptr<GuiTextWrap> help_text;
#ifdef ONLINE_PLATFORM
    std::unique_ptr<gcn::Button> online_multiplayer;
#endif
    std::unique_ptr<gcn::Button> lan_games;
    std::unique_ptr<gcn::Button> split_screen_mode;
    std::unique_ptr<gcn::Button> single_player_mode;
    std::unique_ptr<gcn::Button> exit;
};

StartGameScreenImpl::StartGameScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
    : knights_app(app), window(win)
{
    const Localization &loc = app.getLocalization();
    const int w = 300, h = 40, vspace = 15, pad = 15, vspace2 = 40;
    
    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int x = pad;
    int y = pad;
    const int yinc = h + vspace;

#ifdef ONLINE_PLATFORM
    online_multiplayer.reset(new GuiButton(loc.get(LocalKey("online_multiplayer")).asUTF8()));
    online_multiplayer->setSize(w,h);
    online_multiplayer->addActionListener(this);
    container->add(online_multiplayer.get(), x, y);
    y += yinc;
#endif

    lan_games.reset(new GuiButton(loc.get(LocalKey("lan_games")).asUTF8()));
    lan_games->setSize(w,h);
    lan_games->addActionListener(this);
    container->add(lan_games.get(), x, y);
    y += yinc;

    split_screen_mode.reset(new GuiButton(loc.get(LocalKey("split_screen_mode")).asUTF8()));
    split_screen_mode->setSize(w,h);
    split_screen_mode->addActionListener(this);
    container->add(split_screen_mode.get(), x, y);
    y += yinc;

    single_player_mode.reset(new GuiButton(loc.get(LocalKey("single_player_mode")).asUTF8()));
    single_player_mode->setSize(w,h);
    single_player_mode->addActionListener(this);
    container->add(single_player_mode.get(), x, y);
    y += yinc;
    
    exit.reset(new GuiButton(loc.get(LocalKey("cancel")).asUTF8()));
    exit->setSize(w,h);
    exit->addActionListener(this);
    container->add(exit.get(), x, y);
    y += yinc;
    
    container->setSize(w + 2*pad, y - vspace + pad);
    panel2.reset(new GuiPanel(container.get()));

    title.reset(new gcn::Label(loc.get(LocalKey("start_game_caps")).asUTF8()));
    title->setWidth(w+2*pad);
    title->setHeight(title->getHeight() + 2*pad);
    title->setAlignment(gcn::Graphics::CENTER);
    panel1.reset(new GuiPanel(title.get()));

    // Size the help text based on font metrics and window width
    const int m_width = title->getFont()->getWidth("M");
    int win_w, win_h;
    window->getSize(win_w, win_h);
    const int text_wrap_width = std::min(40 * m_width, win_w);
    const int text_height = 4 * title->getFont()->getHeight();

    help_text.reset(new GuiTextWrap);
    help_text->setWidth(text_wrap_width);
    help_text->setHeight(text_height);
    help_text->setCentred(true);
    help_text->setOpaque(false);
    help_text->setBackgroundColor(gcn::Color(0, 0, 0));
    help_text->setForegroundColor(gcn::Color(235, 235, 235));

    // Layout: add (text_height + vspace) of padding above the panels to match
    // the help text below them. This keeps GuiCentre placing the panels at
    // exactly the same vertical position as before.
    const int panel_w = panel2->getWidth();
    const int outer_w = std::max(panel_w, text_wrap_width);
    const int panel_x = (outer_w - panel_w) / 2;
    const int text_x = (outer_w - text_wrap_width) / 2;
    const int top_pad = text_height + vspace2;
    const int panel1_y = top_pad;
    const int buttons_y = panel1_y + panel1->getHeight() + vspace2;
    const int text_y = buttons_y + panel2->getHeight() + vspace2;

    outer_container.reset(new gcn::Container);
    outer_container->setOpaque(false);
    outer_container->add(panel1.get(), panel_x, panel1_y);
    outer_container->add(panel2.get(), panel_x, buttons_y);
    outer_container->add(help_text.get(), text_x, text_y);
    outer_container->setSize(outer_w, text_y + text_height);

#ifdef ONLINE_PLATFORM
    online_multiplayer->addMouseListener(this);
#endif
    lan_games->addMouseListener(this);
    split_screen_mode->addMouseListener(this);
    single_player_mode->addMouseListener(this);
    exit->addMouseListener(this);

    centre.reset(new GuiCentre(outer_container.get()));
    gui.setTop(centre.get());
}

void StartGameScreenImpl::mouseEntered(gcn::MouseEvent &e)
{
    const Localization &loc = knights_app.getLocalization();
    UTF8String desc;
#ifdef ONLINE_PLATFORM
    if (e.getSource() == online_multiplayer.get()) {
        desc = loc.get(LocalKey("online_multiplayer_desc"));
    } else
#endif
    if (e.getSource() == lan_games.get()) {
        desc = loc.get(LocalKey("lan_games_desc"));
    } else if (e.getSource() == split_screen_mode.get()) {
        desc = loc.get(LocalKey("split_screen_desc"));
    } else if (e.getSource() == single_player_mode.get()) {
        desc = loc.get(LocalKey("single_player_desc"));
    } else if (e.getSource() == exit.get()) {
        desc = loc.get(LocalKey("cancel_desc"));
    }
    help_text->setText(desc);
    window->invalidateAll();
}

void StartGameScreenImpl::mouseExited(gcn::MouseEvent &e)
{
    help_text->setText(UTF8String());
    window->invalidateAll();
}

void StartGameScreenImpl::action(const gcn::ActionEvent &event)
{
    std::unique_ptr<Screen> new_screen;

    if (event.getSource() == split_screen_mode.get()) {
        knights_app.startSplitScreenGame();

#ifdef ONLINE_PLATFORM
    } else if (event.getSource() == online_multiplayer.get()) {
        // Go to OnlineMultiplayerScreen
        new_screen.reset(new OnlineMultiplayerScreen);
#endif

    } else if (event.getSource() == lan_games.get()) {
        // Go to LanGameScreen
        new_screen.reset(new LanGameScreen);

    } else if (event.getSource() == single_player_mode.get()) {
        knights_app.startSinglePlayerGame();
        
    } else if (event.getSource() == exit.get()) {
        // Go back to title screen
        new_screen.reset(new TitleScreen);
    }
    
    if (new_screen.get()) {
        knights_app.requestScreenChange(std::move(new_screen));
    }
}        

bool StartGameScreen::start(KnightsApp &knights_app, shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    knights_app.resetAll();
    pimpl.reset(new StartGameScreenImpl(knights_app, w, gui));
    return true;
}
