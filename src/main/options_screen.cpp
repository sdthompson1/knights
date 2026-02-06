/*
 * options_screen.cpp
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

#include "options.hpp"

#include "knights_app.hpp"
#include "options_screen.hpp"
#include "title_screen.hpp"

#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "gui_text_wrap.hpp"

// coercri
#include "gcn/cg_font.hpp"
#include "gfx/window_listener.hpp"

#include "boost/scoped_ptr.hpp"

#include <sstream>

namespace {
    const char * control_names[] = { "UP", "DOWN", "LEFT", "RIGHT", "ACTION", "SUICIDE" };
    const char * chat_names[] = { "GLOBAL CHAT", "TEAM CHAT" };

    class ScalingListModel : public gcn::ListModel {
    public:
        std::string getElementAt(int i) {
            if (i==0) return "Scale2x";
            else return "Pixellated";
        }

        int getNumberOfElements() {
            return 2;
        }
    };

    class DisplayListModel : public gcn::ListModel {
    public:
        std::string getElementAt(int i) {
            if (i == 0) {
                return "Windowed";
            } else if (i == 1) {
                return "Full Screen";
            } else {
                return "";
            }
        }

        int getNumberOfElements() {
            return 2;
        }
    };

    class ShowControlsListModel : public gcn::ListModel {
    public:
        std::string getElementAt(int i) {
            if (i==0) return "LAN / Internet / Single Player";
            else return "Two Player (Split Screen)";
        }

        int getNumberOfElements() {
            return 2;
        }
    };

    class ControlSystemListModel : public gcn::ListModel {
    public:
        ControlSystemListModel() { }

        std::string getElementAt(int i) {
            if (i==0) return "New control system (Mouse + Keyboard)";
            else return "Original Amiga controls (Keyboard only)";
        }

        int getNumberOfElements() {
            return 2;
        }
    };
}

class OptionsScreenImpl : public gcn::ActionListener, public gcn::KeyListener, public Coercri::WindowListener {
public:
    OptionsScreenImpl(KnightsApp &app, gcn::Gui &gui);
    void action(const gcn::ActionEvent &event);
    void keyPressed(gcn::KeyEvent &ke);
    void keyReleased(gcn::KeyEvent &ke);
    void onKey(Coercri::KeyEventType type, Coercri::KeyCode kc, Coercri::KeyModifier);

    void transferToGui();
    void setupChangeControls(int);
    
    boost::shared_ptr<Coercri::Window> window;

private:
    KnightsApp &knights_app;
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Button> ok_button, cancel_button;
    boost::scoped_ptr<gcn::Label> controls_title;
    boost::scoped_ptr<gcn::Label> show_controls_label;
    boost::scoped_ptr<gcn::DropDown> show_controls_dropdown;
    boost::scoped_ptr<gcn::ListModel> show_controls_listmodel;
    boost::scoped_ptr<gcn::Label> control_system_label;
    boost::scoped_ptr<gcn::DropDown> control_system_dropdown;
    boost::scoped_ptr<ControlSystemListModel> control_system_listmodel;
    boost::scoped_ptr<gcn::Label> control_label[2][7];
    boost::scoped_ptr<gcn::Label> control_setting[2][6];
    boost::scoped_ptr<gcn::Button> change_button[2];
    boost::scoped_ptr<gcn::Label> change_label;
    boost::scoped_ptr<gcn::Label> options_label, scaling_label, display_label;
    boost::scoped_ptr<gcn::ListModel> scaling_listmodel;
    boost::scoped_ptr<DisplayListModel> display_listmodel;
    boost::scoped_ptr<gcn::DropDown> scaling_dropdown, display_dropdown;
    boost::scoped_ptr<gcn::CheckBox> non_integer_checkbox;
    boost::scoped_ptr<gcn::Button> restore_button;
    boost::scoped_ptr<GuiTextWrap> bad_key_area;
    boost::scoped_ptr<gcn::CheckBox> tooltip_checkbox;

    Options current_opts;

    bool change_active;
    int change_key;
    int change_player;

    bool previous_fullscreen;

    UTF8String bad_key_msg;
};

OptionsScreenImpl::OptionsScreenImpl(KnightsApp &app, gcn::Gui &gui)
    : knights_app(app), current_opts(app.getOptions()), change_active(false)
{
    previous_fullscreen = current_opts.fullscreen;
    
    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 15, pad_small = 5, extra_pad = 100, indent = 20;
    const int min_width = 500;
    int y = pad;

    // Controls

    controls_title.reset(new gcn::Label("Controls"));
    controls_title->setForegroundColor(gcn::Color(0,0,128));
    container->add(controls_title.get(), pad, y);
    y += controls_title->getHeight() + pad_small;

    show_controls_label.reset(new gcn::Label("Show controls for: "));
    container->add(show_controls_label.get(), pad, y+1);
    show_controls_listmodel.reset(new ShowControlsListModel);
    show_controls_dropdown.reset(new gcn::DropDown(show_controls_listmodel.get()));
    const int scdx = pad + show_controls_label->getWidth();
    show_controls_dropdown->addActionListener(this);
    container->add(show_controls_dropdown.get(), scdx, y);
    y += show_controls_dropdown->getHeight() + pad;

    control_system_label.reset(new gcn::Label("Control system: "));
    container->add(control_system_label.get(), pad, y+1);
    control_system_listmodel.reset(new ControlSystemListModel);
    control_system_dropdown.reset(new gcn::DropDown(control_system_listmodel.get()));
    const int csx = pad + control_system_label->getWidth();
    control_system_dropdown->addActionListener(this);
    container->add(control_system_dropdown.get(), csx, y);
    y += control_system_dropdown->getHeight() + pad;
    
    control_label[0][0].reset(new gcn::Label("Player 1"));
    control_label[0][0]->setForegroundColor(gcn::Color(0,0,128));
    control_label[1][0].reset(new gcn::Label("Player 2"));
    control_label[1][0]->setForegroundColor(gcn::Color(0,0,128));
    for (int i = 0; i < 2; ++i) {
        control_label[i][1].reset(new gcn::Label("Up:"));
        control_label[i][2].reset(new gcn::Label("Down:"));
        control_label[i][3].reset(new gcn::Label("Left:"));
        control_label[i][4].reset(new gcn::Label("Right:"));
        control_label[i][5].reset(new gcn::Label("Action:"));
        control_label[i][6].reset(new gcn::Label("Suicide:"));
        for (int j = 0; j < 6; ++j) {
            control_setting[i][j].reset(new gcn::Label);
        }
    }

    change_label.reset(new gcn::Label);
    change_label->setVisible(false);
    change_label->setForegroundColor(gcn::Color(128,0,0));

    const int cw0 = control_label[0][6]->getWidth() + 10;
    const int cw1 = control_label[0][0]->getFont()->getWidth("RIGHT WINDOWS");  // longest possible key name
    const int intercol = 50;
    const int overall_width = std::max(min_width, indent + (pad + cw0 + cw1)*2 + intercol);

    show_controls_dropdown->setWidth(overall_width - 2*pad - show_controls_label->getWidth());
    control_system_dropdown->setWidth(overall_width - 2*pad - control_system_label->getWidth());

    container->add(control_label[0][0].get(), indent + pad, y);
    container->add(control_label[1][0].get(), indent + pad + cw0 + cw1 + intercol, y);
    y += control_label[0][0]->getHeight();
    y += 7;

    for (int i = 0; i < 6; ++i) {
        container->add(control_label[0][i+1].get(), indent + pad, y);
        container->add(control_setting[0][i].get(), indent + pad + cw0, y);
        container->add(control_label[1][i+1].get(), indent + pad + cw0 + cw1 + intercol, y);
        container->add(control_setting[1][i].get(), indent + pad + cw0 + cw1 + intercol + cw0, y);
        y += control_label[0][i+1]->getHeight() + pad_small;
    }

    y += pad_small;

    for (int i = 0; i < 2; ++i) {
        change_button[i].reset(new GuiButton("Change"));
        change_button[i]->addActionListener(this);
        container->add(change_button[i].get(), indent + pad + i*(cw0+cw1+intercol), y); // NOTE: y position is changed in transferToGui()
    }
    container->add(change_label.get(), indent + pad, y);

    y += change_button[0]->getHeight();

    tooltip_checkbox.reset(new gcn::CheckBox("Show action bar tool tips"));
    tooltip_checkbox->addActionListener(this);
    container->add(tooltip_checkbox.get(), pad, y - 2 * (control_label[0][1]->getHeight() + pad_small) + 25);
    
    y += 25;

    bad_key_area.reset(new GuiTextWrap);
    bad_key_area->setWidth(overall_width - 2*pad);
    bad_key_area->setForegroundColor(gcn::Color(128,0,0));
    container->add(bad_key_area.get(), pad, y);
    
    options_label.reset(new gcn::Label("Other Options"));
    options_label->setForegroundColor(gcn::Color(0,0,128));
    container->add(options_label.get(), pad, y);
    y += options_label->getHeight() + pad_small + 2;
    
    display_label.reset(new gcn::Label("Display Mode:"));
    scaling_label.reset(new gcn::Label("Graphics Scaling:"));
    const int ddx = pad + pad + scaling_label->getWidth();
    display_listmodel.reset(new DisplayListModel);
    display_dropdown.reset(new gcn::DropDown(display_listmodel.get()));
    display_dropdown->setWidth(overall_width - pad - extra_pad - ddx);
    display_dropdown->addActionListener(this);
    container->add(display_label.get(), pad, y);
    container->add(display_dropdown.get(), ddx, y);
    y += display_dropdown->getHeight() + pad_small + 2;

    scaling_listmodel.reset(new ScalingListModel);
    scaling_dropdown.reset(new gcn::DropDown(scaling_listmodel.get()));
    scaling_dropdown->setWidth(overall_width - pad - extra_pad - ddx);
    scaling_dropdown->addActionListener(this);
    container->add(scaling_label.get(), pad, y);
    container->add(scaling_dropdown.get(), ddx, y);
    y += scaling_dropdown->getHeight() + pad_small + 4;

    non_integer_checkbox.reset(new gcn::CheckBox("Allow Non-Integer Scaling"));
    non_integer_checkbox->addActionListener(this);
    container->add(non_integer_checkbox.get(), pad, y);
    y += non_integer_checkbox->getHeight() + 40;

    restore_button.reset(new GuiButton("Restore Defaults"));
    restore_button->addActionListener(this);
    container->add(restore_button.get(), overall_width/2 - restore_button->getWidth()/2, y);
    
    ok_button.reset(new GuiButton("OK"));
    ok_button->addActionListener(this);
    container->add(ok_button.get(), pad, y);

    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    container->add(cancel_button.get(), overall_width - pad - cancel_button->getWidth(), y);

    y += cancel_button->getHeight();
    y += pad;

    container->setSize(overall_width, y);
    panel.reset(new GuiPanel(container.get()));    
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());

    gui.addGlobalKeyListener(this);
}

void OptionsScreenImpl::action(const gcn::ActionEvent &event)
{
    const bool got_ok = event.getSource() == ok_button.get();
    const bool got_cancel = event.getSource() == cancel_button.get();

    if (got_ok || got_cancel) {
        if (got_ok) {
            // Note: Save the options FIRST, because switchToFullScreen or switchToWindowed could throw.
            knights_app.setAndSaveOptions(current_opts);
            if (current_opts.fullscreen && !previous_fullscreen) {
                window->switchToFullScreen();
            } else if (!current_opts.fullscreen && previous_fullscreen) {
                int w, h;
                knights_app.getWindowedModeSize(w, h);
                window->switchToWindowed(w, h);
            }
        }
        std::unique_ptr<Screen> new_screen(new TitleScreen);
        knights_app.requestScreenChange(std::move(new_screen));
        return;
    }

    if (event.getSource() == change_button[0].get()) {
        setupChangeControls(show_controls_dropdown->getSelected() ? 0 : 2);
    } else if (event.getSource() == change_button[1].get()) {
        setupChangeControls(show_controls_dropdown->getSelected() ? 1 : 3);
    } else if (event.getSource() == scaling_dropdown.get()) {
        current_opts.use_scale2x = (scaling_dropdown->getSelected() == 0);
    } else if (event.getSource() == non_integer_checkbox.get()) {
        current_opts.allow_non_integer_scaling = non_integer_checkbox->isSelected();
    } else if (event.getSource() == display_dropdown.get()) {
        current_opts.fullscreen = (display_dropdown->getSelected() != 0);
    } else if (event.getSource() == restore_button.get()) {
        current_opts = Options();
        transferToGui();
    } else if (event.getSource() == show_controls_dropdown.get()) {
        transferToGui();
    } else if (event.getSource() == control_system_dropdown.get()) {
        current_opts.new_control_system = (control_system_dropdown->getSelected() == 0);
        transferToGui();
    } else if (event.getSource() == tooltip_checkbox.get()) {
        current_opts.action_bar_tool_tips = tooltip_checkbox->isSelected();
    }
}

void OptionsScreenImpl::keyPressed(gcn::KeyEvent &ke)
{
    if (change_active) ke.consume();
}

void OptionsScreenImpl::keyReleased(gcn::KeyEvent &ke)
{
    if (change_active) ke.consume();
}

void OptionsScreenImpl::transferToGui()
{
    const bool split_screen = (show_controls_dropdown->getSelected() != 0);
    const bool new_ctrls = current_opts.new_control_system && !split_screen;

    control_system_dropdown->setSelected(new_ctrls ? 0 : 1);
    control_system_dropdown->setVisible(!split_screen);

    tooltip_checkbox->setSelected(current_opts.action_bar_tool_tips);
    //    tooltip_checkbox->setVisible(new_ctrls);
    tooltip_checkbox->setVisible(false);

    if (split_screen) {
        control_system_label->setCaption("Control keys:");
        control_label[0][0]->setCaption("Player 1");
        control_label[1][0]->setCaption("Player 2");
    } else {
        control_system_label->setCaption("Control system: ");
        control_label[0][0]->setCaption("Control keys:");
        control_label[1][0]->setCaption("Chat keys:");
    }
    control_system_label->adjustSize();
    control_label[0][0]->adjustSize();
    control_label[1][0]->adjustSize();

    // i=0: split screen keys, player 1
    // i=1: split screen keys, player 2
    // i=2: singleplayer/online keys.
    const int imin = split_screen ? 0 : 2;
    const int imax = split_screen ? 1 : 2;

    for (int i = imin; i <= imax; ++i) {
        const int which_label = i==2 ? 0 : i;
        for (int j = 0; j < 6; ++j) {
            if (change_active && change_player == i && j >= change_key) {
                if (j == change_key) {
                    control_setting[which_label][j]->setCaption("<Press>");
                } else {
                    control_setting[which_label][j]->setCaption("");
                }
            } else {
                control_setting[which_label][j]->setCaption(Coercri::KeyCodeToKeyName(current_opts.ctrls[i][j]));
            }
            control_setting[which_label][j]->adjustSize();
        }
    }

    // now the chat keys (overrides the above)
    if (!split_screen) {
        // global chat key setting
        control_setting[1][0]->setCaption(Coercri::KeyCodeToKeyName(current_opts.global_chat_key));
        if (change_active && change_player == 3 && change_key == 0) {
            control_setting[1][0]->setCaption("<Press>");
        }
        control_setting[1][0]->adjustSize();

        // team chat key setting
        control_setting[1][1]->setCaption(Coercri::KeyCodeToKeyName(current_opts.team_chat_key));
        if (change_active && change_player == 3) {
            if (change_key == 1) {
                control_setting[1][1]->setCaption("<Press>");
            } else {
                control_setting[1][1]->setCaption("");
            }
        }
        control_setting[1][1]->adjustSize();
    }
    
    if (change_active) {
        change_button[0]->setVisible(false);
        change_button[1]->setVisible(false);
        change_label->setVisible(true);

        std::string key_name = control_names[change_key];
        if (!split_screen && change_player == 3) key_name = chat_names[change_key];
        change_label->setCaption(std::string("** Please press a key for ") + key_name + " **");
        change_label->adjustSize();
        
        show_controls_dropdown->setEnabled(false);
        control_system_dropdown->setEnabled(false);
    } else {
        change_button[0]->setVisible(true);
        change_button[1]->setVisible(true);
        change_label->setVisible(false);
        show_controls_dropdown->setEnabled(true);
        control_system_dropdown->setEnabled(true);
    }
    scaling_dropdown->setSelected(1-int(current_opts.use_scale2x));
    non_integer_checkbox->setSelected(current_opts.allow_non_integer_scaling);
    display_dropdown->setSelected(current_opts.fullscreen ? 1 : 0);

    for (int i = 0; i < 2; ++i) {

        for (int j = 0; j < 6; ++j) {
            bool visible = !new_ctrls || j < 4;

            // special case for non-split-screen: i==1 case is hijacked for the chat keys.
            if (!split_screen && i==1) visible = j < 2;

            control_setting[i][j]->setVisible(visible);
            control_label[i][j+1]->setVisible(visible);
        }

        // Position the "Change" button to be just below the last visible label.
        const gcn::Label * last_label = control_label[i][new_ctrls ? 4 : 6].get();
        change_button[i]->setY(last_label->getY() + last_label->getHeight() + 10);
    }

    // Position the change label to same Y as change button (Trac #120)
    change_label->setY(change_button[0]->getY());
    
    // we also need to change the labels for the chat keys if required
    if (split_screen) {
        control_label[1][1]->setCaption("Up:");
        control_label[1][2]->setCaption("Down:");
    } else {
        control_label[1][1]->setCaption("Global:");
        control_label[1][2]->setCaption("Team:");
    }
    control_label[1][1]->adjustSize();
    control_label[1][2]->adjustSize();

    const bool show_lower = !change_active || bad_key_msg.empty();
    options_label->setVisible(show_lower);
    display_label->setVisible(show_lower);
    scaling_label->setVisible(show_lower);
    display_dropdown->setVisible(show_lower);
    scaling_dropdown->setVisible(show_lower);
    non_integer_checkbox->setVisible(show_lower);
    bad_key_area->setVisible(!show_lower);
    bad_key_area->setText(bad_key_msg);
    bad_key_area->adjustHeight();
    
    if (window) window->invalidateAll();
}

void OptionsScreenImpl::setupChangeControls(int i)
{
    change_active = true;
    bad_key_msg = UTF8String();
    change_player = i;
    change_key = 0;
    transferToGui();
}

void OptionsScreenImpl::onKey(Coercri::KeyEventType type, Coercri::KeyCode kc, Coercri::KeyModifier)
{
    if (type == Coercri::KEY_PRESSED && change_active) {

        if (kc == Coercri::KC_ESCAPE) {
            // cancel!
            change_active = false;
            bad_key_msg = UTF8String();

        } else {
            bool allowed = true;
            
            // Check for duplicate control keys
            if (change_player == 3) {
                // special case: changing chat keys
                if (change_key == 1 && kc == current_opts.global_chat_key) {
                    allowed = false;
                }
            } else {
                // normal case: changing player controls
                for (int i = 0; i < change_key; ++i) {
                    if (current_opts.ctrls[change_player][i] == kc) {
                        allowed = false;
                        break;
                    }
                }
            }

            if (!allowed) {
                bad_key_msg = UTF8String::fromUTF8("Key already in use. Please choose another key.");
            }

            if (allowed) {
                if (change_player == 3) {
                    // special case: changing chat keys
                    if (change_key == 0) {
                        current_opts.global_chat_key = kc;
                        change_key = 1;
                    } else {
                        current_opts.team_chat_key = kc;
                        change_active = false;
                    }
                } else {
                    // normal case: changing player controls
                    
                    current_opts.ctrls[change_player][change_key] = kc;
                    ++change_key;
                    
                    const bool four_key_mode = 
                        current_opts.new_control_system     // Using NEW control system (4-key) for LAN/Internet/SglPlyr
                        && show_controls_dropdown->getSelected() == 0;  // Currently editing ctrls for LAN/Internet/SglPlyr
                    
                    if (change_key >= 6 || (four_key_mode && change_key >= 4)) {
                        change_active = false;
                    }
                }

                bad_key_msg = UTF8String();
            }
        }

        transferToGui();
    }
}

bool OptionsScreen::start(KnightsApp &app, boost::shared_ptr<Coercri::Window> w, gcn::Gui &gui)
{
    pimpl.reset(new OptionsScreenImpl(app, gui));
    pimpl->transferToGui();
    w->addWindowListener(pimpl.get());
    pimpl->window = w;
    return true;
}

OptionsScreen::~OptionsScreen()
{
    if (pimpl->window) pimpl->window->rmWindowListener(pimpl.get());
}
