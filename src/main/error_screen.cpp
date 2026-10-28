/*
 * error_screen.cpp
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

#include "error_screen.hpp"
#include "knights_app.hpp"
#include "localization.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"

// See note in utf8_text_field.cpp about accessing SDL clipboard directly
#include <SDL2/SDL.h>

#include <memory>
using namespace boost;

class ErrorScreenImpl : public gcn::ActionListener, public gcn::KeyListener {
public:
    ErrorScreenImpl(KnightsApp &app, gcn::Gui &gui, const UTF8String &msg)
        : knights_app(app), msg(msg)
    {
        // create container
        container.reset(new gcn::Container);
        container->setOpaque(false);

        int y = 6;
        const int hpad = 6, vpad = 6;
        
        // create label, or text wrap if the message is too wide or multi-line
        const int max_width = 900;
        std::unique_ptr<gcn::Label> label(new gcn::Label(msg.asUTF8()));
        if (label->getWidth() > max_width || msg.asUTF8().find('\n') != std::string::npos) {
            std::unique_ptr<GuiTextWrap> wrap(new GuiTextWrap);
            wrap->setWidth(max_width);
            wrap->setText(msg);
            wrap->adjustHeight();
            message = std::move(wrap);
        } else {
            message = std::move(label);
        }
        container->add(message.get(), hpad, y);
        y += message->getHeight() + vpad;

        // create exit button
        exit_button.reset(new GuiButton(app.getLocalization().get(LocalKey("exit")).asUTF8()));
        exit_button->addActionListener(this);
        container->add(exit_button.get(), hpad + message->getWidth()/2 - exit_button->getWidth()/2, y);
        y += exit_button->getHeight() + vpad;

        // resize the container
        container->setSize(message->getWidth() + 2*hpad, y);

        // add panel, centre.
        panel.reset(new GuiPanel(container.get()));
        centre.reset(new GuiCentre(panel.get()));
        gui.setTop(centre.get());
        gui.addGlobalKeyListener(this);
    }

    void action(const gcn::ActionEvent &event)
    {
        std::unique_ptr<Screen> title_screen(new TitleScreen);
        knights_app.requestScreenChange(std::move(title_screen));
    }

    void keyPressed(gcn::KeyEvent &event) override
    {
        // Ctrl+C: copy error message to clipboard
        if (event.getKey().getValue() == 'c' && event.isControlPressed()) {
            SDL_SetClipboardText(msg.asUTF8().c_str());
            event.consume();
        }
    }

private:
    KnightsApp &knights_app;
    UTF8String msg;
    std::unique_ptr<GuiCentre> centre;
    std::unique_ptr<GuiPanel> panel;
    std::unique_ptr<gcn::Container> container;
    std::unique_ptr<gcn::Widget> message;
    std::unique_ptr<gcn::Button> exit_button;
};

bool ErrorScreen::start(KnightsApp &app, shared_ptr<Coercri::Window>, gcn::Gui &gui)
{
    app.resetAll();
    pimpl.reset(new ErrorScreenImpl(app, gui, msg));

    // Flash window when there is an error. This is useful in case you were waiting on the
    // lobby screen (with the window minimized) and you got disconnected.
    app.flashWindow();
    return true;
}
