/*
 * error_screen.cpp
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

#include "knights_app.hpp"

#include "error_screen.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "boost/scoped_ptr.hpp"
using namespace boost;

class ErrorScreenImpl : public gcn::ActionListener {
public:
    ErrorScreenImpl(KnightsApp &app, gcn::Gui &gui, const std::string &msg)
        : knights_app(app)
    {
        // create container
        container.reset(new gcn::Container);
        container->setOpaque(false);

        int y = 6;
        const int hpad = 6, vpad = 6;
        
        // create label
        label.reset(new gcn::Label(msg));
        container->add(label.get(), hpad, y);
        y += label->getHeight() + vpad;
        
        // create exit button
        exit_button.reset(new GuiButton("Exit"));
        exit_button->addActionListener(this);
        container->add(exit_button.get(), hpad + label->getWidth()/2 - exit_button->getWidth()/2, y);
        y += exit_button->getHeight() + vpad;

        // resize the container
        container->setSize(label->getWidth() + 2*hpad, y);

        // add panel, centre.
        panel.reset(new GuiPanel(container.get()));
        centre.reset(new GuiCentre(panel.get()));
        gui.setTop(centre.get());
    }

    void action(const gcn::ActionEvent &event)
    {
        auto_ptr<Screen> title_screen(new TitleScreen);
        knights_app.requestScreenChange(title_screen);
    }

private:
    KnightsApp &knights_app;
    scoped_ptr<GuiCentre> centre;
    scoped_ptr<GuiPanel> panel;
    scoped_ptr<gcn::Container> container;
    scoped_ptr<gcn::Label> label;
    scoped_ptr<gcn::Button> exit_button;
};

bool ErrorScreen::start(KnightsApp &app, shared_ptr<Coercri::Window>, gcn::Gui &gui)
{
    app.resetAll();
    pimpl.reset(new ErrorScreenImpl(app, gui, msg));

    // Pop window to front when there is an error. This is useful in case you were waiting on the
    // lobby screen (with the window minimized) and you got disconnected.
    app.popWindowToFront();
    return true;
}
