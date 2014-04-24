/*
 * password_screen.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2013.
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
#include "knights_client.hpp"
#include "password_screen.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"

#include "boost/scoped_ptr.hpp"

namespace {
    // Like TextField but draws all chars as '*'
    class PasswordField : public gcn::TextField {
    public:
        void draw(gcn::Graphics *graphics)
        {
            const std::string original_text = mText;
            mText.assign(mText.length(), '*');
            TextField::draw(graphics);
            mText = original_text;
        }
    };
}

class PasswordScreenImpl : public gcn::ActionListener {
public:
    PasswordScreenImpl(boost::shared_ptr<KnightsClient> cli, bool first)
        : client(cli), sent(false), first_attempt(first), knights_app(0), gui(0) { }

    void start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g);
    void action(const gcn::ActionEvent &event);

private:
    boost::shared_ptr<KnightsClient> client;
    
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Label> label1, label2;
    boost::scoped_ptr<PasswordField> password_field;
    boost::scoped_ptr<gcn::Button> ok_button, cancel_button;
    
    bool sent;
    bool first_attempt;

    KnightsApp *knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui *gui;
};


void PasswordScreenImpl::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
{
    knights_app = &ka;
    window = win;
    gui = &g;
    
    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int margin = 10;
    const int pad = 10;
    int y = 10;

    if (first_attempt) {
        label1.reset(new gcn::Label("This is a private server"));
        label2.reset(new gcn::Label("Please enter password:"));
    } else {
        label1.reset(new gcn::Label("Incorrect password entered"));
        label2.reset(new gcn::Label("Please try again:"));
    }

    const int total_width = 2*margin + label1->getWidth() + label2->getWidth();

    // centre the two labels
    container->add(label1.get(), total_width/2 - label1->getWidth()/2, y);
    y += label1->getHeight() + pad;
    container->add(label2.get(), total_width/2 - label2->getWidth()/2, y);
    y += label1->getHeight() + pad;

    password_field.reset(new PasswordField);
    password_field->adjustSize();
    password_field->setWidth(total_width - 2*margin);
    password_field->addActionListener(this);
    container->add(password_field.get(), margin, y);
    y += password_field->getHeight() + pad;

    ok_button.reset(new GuiButton("OK"));
    ok_button->addActionListener(this);
    container->add(ok_button.get(), pad, y);

    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    container->add(cancel_button.get(), total_width - pad - cancel_button->getWidth(), y);

    y += cancel_button->getHeight() + pad;

    container->setSize(total_width, y);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui->setTop(centre.get());

    password_field->requestFocus();
}

void PasswordScreenImpl::action(const gcn::ActionEvent &event)
{
    if (!sent && !password_field->getText().empty() &&
    (event.getSource() == ok_button.get() || event.getSource() == password_field.get())) {
        client->sendPassword(password_field->getText());
        sent = true;
        // clear the gui while we wait for response from server.
        container->clear();
        gui->logic();
        window->invalidateAll();
    } else if (event.getSource() == cancel_button.get() && knights_app) {
        std::auto_ptr<Screen> title_screen(new TitleScreen);
        knights_app->requestScreenChange(title_screen);
    }
}

PasswordScreen::PasswordScreen(boost::shared_ptr<KnightsClient> client, bool first_attempt)
    : pimpl(new PasswordScreenImpl(client, first_attempt))
{ }

bool PasswordScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl->start(ka, win, gui);
    return true;
}
