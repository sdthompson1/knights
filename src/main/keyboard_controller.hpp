/*
 * keyboard_controller.hpp
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
 * KeyboardController: Controller that reads input from the Keyboard
 * Keys to use must be configured in the constructor.
 *
 */

#ifndef KEYBOARD_CONTROLLER_HPP
#define KEYBOARD_CONTROLLER_HPP

#include "controller.hpp"

// coercri
#include "gfx/key_code.hpp"
#include "gfx/window.hpp"

#include "boost/scoped_ptr.hpp"

class KeyboardListener;

class KeyboardController : public Controller {
public:
    KeyboardController(bool using_action_bar_,
                       Coercri::RawKey up_key_, Coercri::RawKey right_key_, Coercri::RawKey down_key_,
                       Coercri::RawKey left_key_, Coercri::RawKey fire_key_, Coercri::RawKey suicide_key_,
                       boost::shared_ptr<Coercri::Window> w);
    virtual ~KeyboardController();
    
    virtual void get(ControllerState &state) const;
    virtual bool usingActionBar() const { return using_action_bar; }
    
private:
    friend class KeyboardListener;
    boost::scoped_ptr<KeyboardListener> kbd_listener;
    boost::shared_ptr<Coercri::Window> win;
    Coercri::RawKey up_key, right_key, down_key, left_key, fire_key, suicide_key;
    bool up, right, down, left, fire, suicide;
    bool using_action_bar;
};

#endif
