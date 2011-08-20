/*
 * keyboard_controller.cpp
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

#include "keyboard_controller.hpp"

// coercri
#include "gfx/window_listener.hpp"

class KeyboardListener : public Coercri::WindowListener {
public:
    explicit KeyboardListener(KeyboardController &c) : ctrlr(c) { }
    virtual void onKey(Coercri::KeyEvent ke, Coercri::KeyCode kc, int character, int modifiers);

private:
    KeyboardController &ctrlr;
};

KeyboardController::KeyboardController(bool using_action_bar_,
                                       Coercri::KeyCode u, Coercri::KeyCode r, Coercri::KeyCode d,
                                       Coercri::KeyCode l, Coercri::KeyCode f, Coercri::KeyCode s,
                                       boost::shared_ptr<Coercri::Window> w)
    : kbd_listener(new KeyboardListener(*this)), win(w),
      up_key(u), right_key(r), down_key(d), left_key(l), fire_key(f), suicide_key(s),
      up(false), right(false), down(false), left(false), fire(false), suicide(false),
      using_action_bar(using_action_bar_)
{
    win->addWindowListener(kbd_listener.get());
}

KeyboardController::~KeyboardController()
{
    win->rmWindowListener(kbd_listener.get());
}

void KeyboardController::get(ControllerState &state) const
{
    state.fire = fire;
    state.suicide = suicide;
    state.centred = !up && !right && !down && !left;
    if (!state.centred) {
        if (up) state.dir = D_NORTH;
        else if (right) state.dir = D_EAST;
        else if (down) state.dir = D_SOUTH;
        else state.dir = D_WEST;
    }

    if (using_action_bar) {
        // Old Suicide and Fire keys do not apply
        state.suicide = false;
        state.fire = false;
    }
}

void KeyboardListener::onKey(Coercri::KeyEvent ke, Coercri::KeyCode kc, int character, int modifiers)
{
    if (ke == Coercri::KE_AUTO_REPEAT) return;  // we are not interested in keyboard repeat here.

    const bool p = ke == Coercri::KE_PRESSED;

    if (kc == ctrlr.up_key) { ctrlr.up = p; }
    else if (kc == ctrlr.right_key) { ctrlr.right = p; }
    else if (kc == ctrlr.down_key) { ctrlr.down = p; }
    else if (kc == ctrlr.left_key) { ctrlr.left = p; }
    else if (kc == ctrlr.fire_key) { ctrlr.fire = p; }
    else if (kc == ctrlr.suicide_key) { ctrlr.suicide = p; }
}
