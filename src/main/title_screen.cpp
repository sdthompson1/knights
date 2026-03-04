/*
 * title_screen.cpp
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

#include "credits_screen.hpp"
#include "knights_app.hpp"
#include "loading_screen.hpp"
#include "options_screen.hpp"
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

class TitleScreenImpl : public gcn::ActionListener, public gcn::MouseListener {
public:
    TitleScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui);
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
    std::unique_ptr<gcn::Button> start_game;
    std::unique_ptr<gcn::Button> tutorial;
    std::unique_ptr<gcn::Button> options;
    std::unique_ptr<gcn::Button> credits;
    std::unique_ptr<gcn::Button> exit;
};

TitleScreenImpl::TitleScreenImpl(KnightsApp &app, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
    : knights_app(app), window(win)
{
    const Localization &loc = app.getLocalization();
    const int w = 280, h = 40, vspace = 15, pad = 15, vspace2 = 40;
    
    container.reset(new gcn::Container);
    container->setOpaque(false);
    
    const int x = pad;
    int y = pad;
    const int yinc = h + vspace;
    
    start_game.reset(new GuiButton(loc.get(LocalKey("start_game")).asUTF8()));
    start_game->setSize(w,h);
    start_game->addActionListener(this);
    container->add(start_game.get(), x, y);
    y += yinc;
    
    tutorial.reset(new GuiButton(loc.get(LocalKey("tutorial")).asUTF8()));
    tutorial->setSize(w,h);
    tutorial->addActionListener(this);
    container->add(tutorial.get(), x, y);
    y += yinc;

    options.reset(new GuiButton(loc.get(LocalKey("options")).asUTF8()));
    options->setSize(w, h);
    options->addActionListener(this);
    container->add(options.get(), x, y);
    y += yinc;

    credits.reset(new GuiButton(loc.get(LocalKey("credits")).asUTF8()));
    credits->setSize(w, h);
    credits->addActionListener(this);
    container->add(credits.get(), x, y);
    y += yinc;
    
    exit.reset(new GuiButton(loc.get(LocalKey("quit")).asUTF8()));
    exit->setSize(w,h);
    exit->addActionListener(this);
    container->add(exit.get(), x, y);
    y += yinc;
    
    container->setSize(w + 2*pad, y - vspace + pad);
    panel2.reset(new GuiPanel(container.get()));

    title.reset(new gcn::Label(loc.get(LocalKey("knights_caps")).asUTF8()));
    title->setWidth(w+2*pad);
    title->setHeight(title->getHeight() + 2*pad);
    title->setAlignment(gcn::Graphics::CENTER);
    panel1.reset(new GuiPanel(title.get()));

    const int m_width = title->getFont()->getWidth("M");
    int win_w, win_h;
    window->getSize(win_w, win_h);
    const int text_wrap_width = std::min(40 * m_width, win_w);
    const int text_height = 4 * title->getFont()->getHeight();

    help_text.reset(new GuiTextWrap);
    help_text->setWidth(text_wrap_width);
    help_text->setHeight(text_height);
    help_text->setCentred(true);
    help_text->setOpaque(true);
    help_text->setBackgroundColor(gcn::Color(0, 0, 0));
    help_text->setForegroundColor(gcn::Color(255, 255, 255));

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

    start_game->addMouseListener(this);
    tutorial->addMouseListener(this);
    options->addMouseListener(this);
    credits->addMouseListener(this);
    exit->addMouseListener(this);

    centre.reset(new GuiCentre(outer_container.get()));
    gui.setTop(centre.get());
}

void TitleScreenImpl::mouseEntered(gcn::MouseEvent &e)
{
    const Localization &loc = knights_app.getLocalization();
    UTF8String desc;
    if (e.getSource() == start_game.get()) {
        desc = loc.get(LocalKey("start_game_desc"));
    } else if (e.getSource() == tutorial.get()) {
        desc = loc.get(LocalKey("tutorial_desc"));
    } else if (e.getSource() == options.get()) {
        desc = loc.get(LocalKey("options_desc"));
    } else if (e.getSource() == credits.get()) {
        desc = loc.get(LocalKey("credits_desc"));
    } else if (e.getSource() == exit.get()) {
        desc = loc.get(LocalKey("quit_desc"));
    }
    help_text->setText(desc);
    window->invalidateAll();
}

void TitleScreenImpl::mouseExited(gcn::MouseEvent &e)
{
    help_text->setText(UTF8String());
    window->invalidateAll();
}

void TitleScreenImpl::action(const gcn::ActionEvent &event)
{
    std::unique_ptr<Screen> new_screen;

    if (event.getSource() == start_game.get()) {
        // Go to Start Game sub-menu
        new_screen.reset(new StartGameScreen);
        
    } else if (event.getSource() == tutorial.get()) {
        knights_app.startTutorial();

    } else if (event.getSource() == options.get()) {
        new_screen.reset(new OptionsScreen);

    } else if (event.getSource() == credits.get()) {
        new_screen.reset(new CreditsScreen("credits_", ".txt"));

    } else if (event.getSource() == exit.get()) {
        knights_app.requestQuit();
    }
    
    if (new_screen.get()) {
        knights_app.requestScreenChange(std::move(new_screen));
    }
}        

bool TitleScreen::start(KnightsApp &knights_app, shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    knights_app.resetAll();
    pimpl.reset(new TitleScreenImpl(knights_app, w, gui));
    return true;
}
