/*
 * my_menu_listeners.hpp
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

#ifndef MY_MENU_LISTENERS_HPP
#define MY_MENU_LISTENERS_HPP

#include "menu_listener.hpp"

#include "network/byte_buf.hpp"  // coercri

#include <iosfwd>
#include <vector>

// Implementation of MenuListener to send out a SERVER_SET_MENU_SELECTION message
class MyMenuListener : public MenuListener {
public:
    MyMenuListener() : changed(false) { }
    void addBuf(Coercri::OutputByteBuf buf) { bufs.push_back(buf); }
    void addBuf(std::vector<unsigned char> &vec) { bufs.push_back(Coercri::OutputByteBuf(vec)); }
    bool wereThereChanges() const { return changed; }
    
    virtual void settingChanged(int item_num, const char *item_key,
                                int choice_num, const char *choice_string,
                                const std::vector<int> &allowed_choices);
    virtual void questDescriptionChanged(const std::string &desc);
    
private:
    std::vector<Coercri::OutputByteBuf> bufs;
    bool changed;
};


// Another implementation of MenuListener to write messages to the knights log
class LogMenuListener : public MenuListener {
public:
    explicit LogMenuListener(std::ostream &str_) : str(str_) { }
    virtual void settingChanged(int item_num, const char *item_key,
                                int choice_num, const char *choice_string,
                                const std::vector<int> &allowed_choices);
private:
    std::ostream &str;
};


// This implementation logs the quest in a QST binary log message

class RandomQuestMenuListener : public MenuListener {
public:
    explicit RandomQuestMenuListener(std::ostream &str_) : str(str_) { }
    virtual void settingChanged(int item_num, const char *item_key,
                                int choice_num, const char *choice_string,
                                const std::vector<int> &allowed_choices);
private:
    std::ostream &str;
};


#endif
