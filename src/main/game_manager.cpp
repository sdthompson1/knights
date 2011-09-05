/*
 * game_manager.cpp
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

#include "misc.hpp"

#include "client_config.hpp"
#include "config_map.hpp"
#include "copy_if.hpp"
#include "error_screen.hpp"
#include "game_manager.hpp"
#include "gfx_manager.hpp"
#include "gui_numeric_field.hpp"
#include "house_colour_font.hpp"
#include "in_game_screen.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "lobby_screen.hpp"
#include "menu.hpp"
#include "menu_item.hpp"
#include "menu_screen.hpp"
#include "menu_selections.hpp"
#include "my_exceptions.hpp"
#include "password_screen.hpp"
#include "sound_manager.hpp"
#include "text_formatter.hpp"
#include "title_screen.hpp"

#include "guichan.hpp"

#include <algorithm>
#include <cctype>
#include <ctime>
#include <set>
#include <sstream>

// annoyingly, since we have 2 types of fonts in the system, need to create
// a small font wrapper class here.
class FontWrapper {
public:
    virtual ~FontWrapper() { }
    virtual int getWidth(const std::string &) const = 0;
};

class GCNFontWrapper : public FontWrapper {
public:
    explicit GCNFontWrapper(const gcn::Font *f_) : f(f_) { }
    int getWidth(const std::string &s) const { return f ? f->getWidth(s) : 0; }
private:
    const gcn::Font *f;
};

class CoercriFontWrapper : public FontWrapper {
public:
    explicit CoercriFontWrapper(const Coercri::Font*f_) : f(f_) { }
    int getWidth(const std::string &s) const { return f? f->getTextWidth(s) : 0; }
private:
    const Coercri::Font *f;
};

namespace {
    struct ChatPrinter : Printer {
        ChatPrinter(std::deque<FormattedLine> &o, const FontWrapper *f) : output(o), font(f), printed(false) { }
        
        int getTextWidth(const std::string &text) { return font ? font->getWidth(text) : 0; }
        int getTextHeight() { return 1; }  // we want to count lines not pixels.
        void printLine(const std::string &text, int, bool) {
            FormattedLine f;
            f.firstline = !printed;
            f.text = text;
            output.push_back(f);
            printed = true;
        }

        std::deque<FormattedLine> &output;
        const FontWrapper *font;
        bool printed;
    };


    // customize dropdown drawing a bit. I want to be able to change the foreground color
    // without also changing the colour of the "down-arrow" drawn in the dropdown's button.
    // I also want to disable mousewheel use, to prevent abuse. (Trac #66.)
    class MyDropDown : public gcn::DropDown {
    public:
        explicit MyDropDown(gcn::ListModel *lm) : gcn::DropDown(lm) { }
        void drawButton(gcn::Graphics * graphics) {
            gcn::Color old_fg_col = getForegroundColor();
            setForegroundColor(gcn::Color(0,0,0));
            gcn::DropDown::drawButton(graphics);
            setForegroundColor(old_fg_col);
        }
        void mouseWheelMovedDown(gcn::MouseEvent &) { }
        void mouseWheelMovedUp(gcn::MouseEvent &) { }
    };

    std::string AddTimestamp(const std::string &msg)
    {
        std::time_t time_since_epoch = std::time(0);
        std::tm * timeptr = std::localtime(&time_since_epoch);
        char buf[256];
        const int nchars = std::strftime(buf, sizeof(buf), "[%H:%M] ", timeptr);
        if (nchars > 0) return buf + msg;
        else return msg;
    }

}

void ChatList::add(const std::string &msg_in)
{
    bool msg_empty = true;
    for (std::string::const_iterator it = msg_in.begin(); it != msg_in.end(); ++it) {
        if (!std::isspace(*it)) {
            msg_empty = false;
            break;
        }
    }

    const std::string msg = (do_timestamps && !msg_empty) ? AddTimestamp(msg_in) : msg_in;
    lines.push_back(msg);
    addFormattedLine(msg);
    if (lines.size() > max_msgs) {
        lines.pop_front();
        rmFormattedLine();
    }
}

void ChatList::setGuiParams(const gcn::Font *new_font, int new_width)
{
    font.reset(new GCNFontWrapper(new_font));
    doSetWidth(new_width);
}

void ChatList::setGuiParams(const Coercri::Font *new_font, int new_width)
{
    font.reset(new CoercriFontWrapper(new_font));
    doSetWidth(new_width);
}

void ChatList::doSetWidth(int new_width)
{
    width = new_width;
    formatted_lines.clear();
    for (std::deque<std::string>::const_iterator it = lines.begin(); it != lines.end(); ++it) {
        addFormattedLine(*it);
    }
}

void ChatList::clear()
{
    lines.clear();
    formatted_lines.clear();
}

void ChatList::addFormattedLine(const std::string &msg)
{
    if (do_format) {
        ChatPrinter p(formatted_lines, font.get());
        TextFormatter formatter(p, width, false);
        formatter.printString(msg);
    } else {
        FormattedLine f;
        f.firstline = true;
        f.text = msg;
        formatted_lines.push_back(f);
    }
    is_updated = true;
}

void ChatList::rmFormattedLine()
{
    if (!formatted_lines.empty()) formatted_lines.pop_front();
    while (!formatted_lines.empty() && !formatted_lines.front().firstline) formatted_lines.pop_front();
    is_updated = true;
}

bool ChatList::isUpdated()
{
    const bool result = is_updated;
    is_updated = false;
    return result;
}

std::string NameList::Name::ToUpper(const std::string &input)
{
    std::string result;
    for (std::string::const_iterator it = input.begin(); it != input.end(); ++it) {
        result += std::toupper(*it);
    }
    return result;
}

void NameList::add(const std::string &x, bool observer, bool ready, int house_col)
{
    Name n;
    n.name = x;
    n.observer = observer;
    n.ready = ready;
    n.house_col = house_col;
    names.push_back(n);
    std::sort(names.begin(), names.end());
}

void NameList::alter(const std::string &x, const bool *observer, const bool *ready, const int *house_col)
{
    std::vector<NameList::Name>::iterator it;
    for (it = names.begin(); it != names.end(); ++it) {
        if (it->name == x) break;
    }
    if (it != names.end()) {
        if (observer) it->observer = *observer;
        if (ready) it->ready = *ready;
        if (house_col) it->house_col = *house_col;
        if (observer) std::sort(names.begin(), names.end());  // changing 'observer' flag might re-order the vector
    }
}

void NameList::clearReady()
{
    for (std::vector<NameList::Name>::iterator it = names.begin(); it != names.end(); ++it) {
        it->ready = false;
    }
}

void NameList::clear()
{
    names.clear();
}

void NameList::remove(const std::string &x)
{
    std::vector<NameList::Name>::iterator it;
    for (it = names.begin(); it != names.end(); ++it) {
        if (it->name == x) break;
    }
    if (it != names.end()) {
        names.erase(it);
    }
}

int NameList::getNumberOfElements()
{
    return names.size();
}

std::string NameList::getElementAt(int i)
{
    if (i < 0 || i >= names.size()) return "";
    std::string result = names[i].name;
    if (names[i].observer) {
        result += " (Observer)";
    } else {
        if (names[i].ready) result += " (Ready)";
        if (names[i].house_col >= 0 && names[i].house_col < house_cols.size()) {
            result += "  \001";
            const Coercri::Color & col = house_cols[names[i].house_col];
            result += (unsigned char)(col.r);
            result += (unsigned char)(col.g);
            result += (unsigned char)(col.b);
        }
    }
    return result;
}


namespace {
    class MenuListModel : public gcn::ListModel {
    public:
        virtual int getNumberOfElements() { return elts.size(); }
        virtual std::string getElementAt(int i) { if (i >= 0 && i < elts.size()) return elts[i]; else return ""; }
        std::vector<std::string> elts;
        std::vector<int> vals;
    };

    class MenuWidgets {
    public:
        MenuWidgets(const MenuItem &mi, gcn::ActionListener *listener, gcn::SelectionListener *listener2)
            : menu_item(mi)
        {
            label.reset(new gcn::Label(menu_item.getTitleString()));
            label->adjustSize();
            if (mi.getKey() == "#time") {
                // time entry (no of minutes) -- used for time limit
                textfield.reset(new GuiNumericField);
                textfield->adjustSize();
                textfield->setWidth(textfield->getFont()->getWidth("999"));
                label2.reset(new gcn::Label("mins"));
                textfield->addActionListener(listener);
            } else {
                list_model.reset(new MenuListModel);
                dropdown.reset(new MyDropDown(list_model.get()));
                dropdown->adjustHeight();
                dropdown->addSelectionListener(listener2);
            }
        }

        // The menu item
        const MenuItem &menu_item;

        // The label
        // NOTE: these have to be shared_ptrs since the MenuWidgets object gets copied.
        boost::shared_ptr<gcn::Label> label;

        // For dropdowns
        boost::shared_ptr<gcn::DropDown> dropdown;
        boost::shared_ptr<MenuListModel> list_model;

        // For (numeric) text boxes
        boost::shared_ptr<GuiNumericField> textfield;
        boost::shared_ptr<gcn::Label> label2;
    };

    int GetMenuWidgetWidth(const gcn::Font &font, const MenuWidgets &widgets)
    {
        const MenuItem & item = widgets.menu_item;

        if (widgets.dropdown) {
            int widest_text = 0;
            for (int i = item.getMinValue(); i < item.getMinValue() + item.getNumValues(); ++i) {
                const std::string & text = item.getValueString(i);
                const int text_width = font.getWidth(text);
                if (text_width > widest_text) widest_text = text_width;
            }
            return widest_text + 8 + font.getHeight();  // getHeight is to allow for the 'drop down button' which is approximately square.
        } else {
            return font.getWidth("99999 mins");
        }
    }

    int GetMenuWidgetHeight(const MenuWidgets &widgets)
    {
        if (widgets.dropdown) {
            return widgets.dropdown->getHeight();
        } else {
            return widgets.textfield->getHeight();
        }
    }
}

class GameManagerImpl {
public:
    GameManagerImpl(KnightsApp &ka, boost::shared_ptr<KnightsClient> kc, boost::shared_ptr<Coercri::Timer> timer_, 
                    bool sgl_plyr, bool tut, bool autostart, const std::string &plyr_nm) 
        : knights_app(ka), knights_client(kc), menu(0), gui_invalid(false), are_menu_widgets_enabled(true),
          timer(timer_), lobby_namelist(avail_house_colours),
          game_list_updated(false), game_namelist(avail_house_colours),
          my_house_colour(0), time_remaining(-1), my_obs_flag(false), my_ready_flag(false),
          is_split_screen(false), is_lan_game(false),
          chat_list(ka.getConfigMap().getInt("max_chat_lines"), true, true),   // want formatting and timestamps
          ingame_player_list(9999, false, false),  // unlimited number of lines; unformatted; no timestamps
          single_player(sgl_plyr), tutorial_mode(tut), autostart_mode(autostart),
          my_player_name(plyr_nm), doing_menu_widget_update(false)
    { }
    
    KnightsApp &knights_app;
    boost::shared_ptr<KnightsClient> knights_client;
    boost::shared_ptr<const ClientConfig> client_config;
    
    const Menu *menu;
    MenuSelections menu_selections;
    std::map<std::string, MenuWidgets> menu_widgets_map;
    std::string quest_description;
    bool gui_invalid;
    bool are_menu_widgets_enabled;

    boost::shared_ptr<Coercri::Timer> timer;

    // lobby info
    std::string server_name;
    NameList lobby_namelist;
    std::vector<GameInfo> game_infos;
    bool game_list_updated;

    // current game info
    std::string current_game_name;   // empty if not connected to a game. set if we have a
                                     // join game request in flight, or are currently in a game.
    NameList game_namelist;
    std::vector<Coercri::Color> avail_house_colours;
    int my_house_colour;
    int time_remaining;
    bool my_obs_flag;
    bool my_ready_flag;
    bool is_split_screen;
    bool is_lan_game;
    
    // chat
    ChatList chat_list;

    // player list
    ChatList ingame_player_list;
    std::vector<ClientPlayerInfo> saved_client_player_info;
    std::set<std::string> ready_to_end; // names of players who have clicked mouse at end of game.

    bool single_player;
    bool tutorial_mode;
    bool autostart_mode;

    std::string my_player_name;
    std::string saved_chat;

    bool doing_menu_widget_update;
};

void GameManager::setServerName(const std::string &sname)
{
    pimpl->server_name = sname;
}

const std::string & GameManager::getServerName() const
{
    return pimpl->server_name;
}

const std::vector<GameInfo> & GameManager::getGameInfos() const
{
    return pimpl->game_infos;
}

bool GameManager::isGameListUpdated()
{
    const bool result = pimpl->game_list_updated;
    pimpl->game_list_updated = false;

    if (result) {
        // Make sure it's sorted
        std::sort(pimpl->game_infos.begin(), pimpl->game_infos.end());
    }
    
    return result;
}

bool GameManager::isGuiInvalid()
{
    const bool r = pimpl->gui_invalid;
    pimpl->gui_invalid = false;
    return r;
}

const std::string & GameManager::getCurrentGameName() const
{
    return pimpl->current_game_name;
}

void GameManager::tryJoinGame(const std::string &game_name)
{
    if (!pimpl->current_game_name.empty()) return;
    pimpl->knights_client->joinGame(game_name);
    pimpl->current_game_name = game_name;
}

void GameManager::tryJoinGameSplitScreen(const std::string &game_name)
{
    if (!pimpl->current_game_name.empty()) return;    
    pimpl->knights_client->joinGameSplitScreen(game_name);
    pimpl->current_game_name = game_name;
}

void GameManager::createMenuWidgets(gcn::ActionListener *listener,
                                    gcn::SelectionListener *listener2,
                                    int initial_x,
                                    int initial_y,
                                    gcn::Container &container,
                                    int &menu_width, int &y_after_menu)
{
    const int pad = 10;  // gap between labels and dropdowns.
    const int vspace = 2;
    const int extra_vspace = 10;
    
    const Menu *menu = pimpl->menu;
    if (!menu) throw UnexpectedError("cannot create menu widgets");
    
    // create widgets, work out widths
    int max_label_w = 0, max_widget_w = 0;
    for (int i = 0; i < menu->getNumItems(); ++i) {
        const MenuItem &item = menu->getItem(i);
        MenuWidgets menu_widgets(item, listener, listener2);
        pimpl->menu_widgets_map.insert(std::make_pair(item.getKey(), menu_widgets));
        max_label_w = std::max(max_label_w, menu_widgets.label->getWidth());
        max_widget_w = std::max(max_widget_w, GetMenuWidgetWidth(*menu_widgets.label->getFont(), menu_widgets));
    }

    // resize widgets & add to container
    int y = initial_y;
    for (int i = 0; i < menu->getNumItems(); ++i) {
        const MenuItem &item(menu->getItem(i));
        std::map<std::string,MenuWidgets>::const_iterator mw = pimpl->menu_widgets_map.find(item.getKey());

        container.add(mw->second.label.get(), initial_x, y+1);

        if (mw->second.dropdown) {
            mw->second.dropdown->setWidth(max_widget_w);
            container.add(mw->second.dropdown.get(), initial_x + max_label_w + pad, y);
        } else {
            container.add(mw->second.textfield.get(), initial_x + max_label_w + pad, y);
            container.add(mw->second.label2.get(), mw->second.textfield->getX() + mw->second.textfield->getWidth() + pad, y+1);
        }
    
        y += std::max(mw->second.label->getHeight(), GetMenuWidgetHeight(mw->second)) + vspace;
        if (item.getSpaceAfter()) y += extra_vspace;
    }

    // make sure the list models are updated
    for (int i = 0; i < menu->getNumItems(); ++i) {
        updateMenuWidget(menu->getItem(i).getKey());
    }
    
    y_after_menu = y;
    menu_width = max_label_w + pad + max_widget_w;
}

void GameManager::destroyMenuWidgets()
{
    pimpl->menu_widgets_map.clear();
}

void GameManager::setMenuWidgetsEnabled(bool enabled)
{
    // note: this routine is called by MenuScreen::updateGui as needed.
    
    if (pimpl->are_menu_widgets_enabled == enabled) return;
    pimpl->are_menu_widgets_enabled = enabled;
    
    for (std::map<std::string, MenuWidgets>::iterator it = pimpl->menu_widgets_map.begin(); it != pimpl->menu_widgets_map.end(); ++it) {
        updateMenuWidget(it->second.menu_item.getKey());
    }
}

void GameManager::getMenuStrings(std::vector<std::pair<std::string, std::string> > &menu_strings) const
{
    const Menu * menu = pimpl->menu;
    if (!menu) throw UnexpectedError("cannot get menu strings");

    for (int i = 0; i < menu->getNumItems(); ++i) {
        std::pair<std::string, std::string> p;
        const MenuItem &item = menu->getItem(i);
        p.first = item.getTitleString();
        p.second = item.getValueString(pimpl->menu_selections.getValue(item.getKey()));
        
        menu_strings.push_back(p);
        if (item.getSpaceAfter()) {
            menu_strings.push_back(std::make_pair(std::string(), std::string()));
        }
    }
}

bool GameManager::getMenuWidgetInfo(gcn::Widget *source, std::string &out_key, int &out_val) const
{
    const Menu *menu = pimpl->menu;
    if (!menu) return false;

    for (int i = 0; i < menu->getNumItems(); ++i) {
        const MenuItem &item = menu->getItem(i);
        const std::string &key = item.getKey();

        const MenuWidgets &mw = pimpl->menu_widgets_map.find(key)->second;
        
        const gcn::DropDown *dropdown = mw.dropdown.get();
        const GuiNumericField *textfield = mw.textfield.get();
        if (dropdown == source) {
            // Find out what was selected
            const int selected = dropdown->getSelected();
            if (selected >= 0 && selected < mw.list_model->vals.size()) {
                out_key = key;
                out_val = mw.list_model->vals[selected];
                return true;
            }
        } else if (textfield == source) {
            const std::string & text = textfield->getText();
            if (text.empty()) {
                out_key = key;
                out_val = 0;
                return true;
            } else {
                std::istringstream str(text);
                int result = 0;
                str >> result;
                if (str) {
                    out_key = key;
                    out_val = result;
                    return true;
                }
            }
        }
    }
    // Source widget was not one of the menu widgets
    return false;
}

bool GameManager::doingMenuWidgetUpdate() const
{
    return pimpl->doing_menu_widget_update;
}

namespace {
    struct Sentinel {
        explicit Sentinel(bool &b_) : b(b_) { b = true; }
        ~Sentinel() { b = false; }
        bool &b;
    };
}

void GameManager::updateMenuWidget(const std::string &key)
{
    Sentinel sentinel(pimpl->doing_menu_widget_update);
    
    std::map<std::string, MenuWidgets>::iterator mw_iter = pimpl->menu_widgets_map.find(key);
    if (mw_iter != pimpl->menu_widgets_map.end()) {
        MenuWidgets &mw = mw_iter->second;
        
        if (mw.dropdown) {
        
            mw.list_model->elts.clear();
            mw.list_model->vals.clear();
            
            const std::vector<int> &allowed_vals = pimpl->menu_selections.getAllowedValues(key);
            for (std::vector<int>::const_iterator it = allowed_vals.begin(); it != allowed_vals.end(); ++it) {
                mw.list_model->elts.push_back(mw.menu_item.getValueString(*it));
                mw.list_model->vals.push_back(*it);
            }
            
            const int val_selected = pimpl->menu_selections.getValue(key);
            for (int idx = 0; idx < mw.list_model->vals.size(); ++idx) {
                const int val_at_idx = mw.list_model->vals[idx];
                if (val_selected == val_at_idx) {
                    mw.dropdown->setSelected(idx);
                    break;
                }
            }

            const bool is_enabled = (allowed_vals.size() != 1 || allowed_vals[0] != 0) && pimpl->are_menu_widgets_enabled;
            mw.dropdown->setForegroundColor(is_enabled ? gcn::Color(0,0,0) : gcn::Color(170,170,170));
            mw.dropdown->setEnabled(is_enabled);

        } else {

            std::ostringstream str;
            const int val = pimpl->menu_selections.getValue(key);
            if (val > 0) str << val;
            mw.textfield->setText(str.str());
        }
        
        pimpl->gui_invalid = true;
    }
}

Coercri::Color GameManager::getAvailHouseColour(int i) const
{
    if (i < 0 || i >= pimpl->avail_house_colours.size()) return Coercri::Color();
    else return pimpl->avail_house_colours[i];
}

int GameManager::getNumAvailHouseColours() const
{
    return pimpl->avail_house_colours.size();
}

bool GameManager::getMyObsFlag() const
{
    return pimpl->my_obs_flag;
}

bool GameManager::getMyReadyFlag() const
{
    return pimpl->my_ready_flag;
}

int GameManager::getMyHouseColour() const
{
    return pimpl->my_house_colour;
}

int GameManager::getTimeRemaining() const
{
    return pimpl->time_remaining;
}

NameList & GameManager::getGamePlayersList() const
{
    return pimpl->game_namelist;
}

NameList & GameManager::getLobbyPlayersList() const
{
    return pimpl->lobby_namelist;
}

ChatList & GameManager::getChatList() const
{
    return pimpl->chat_list;
}

ChatList & GameManager::getIngamePlayerList() const
{
    return pimpl->ingame_player_list;
}

bool GameManager::setSavedChat(const std::string &s)
{
    const bool result = (pimpl->saved_chat != s);
    pimpl->saved_chat = s;
    return result;
}


//
// Callback functions
//

void GameManager::connectionLost()
{
    // Go to ErrorScreen
    std::auto_ptr<Screen> error_screen(new ErrorScreen("The network connection has been lost"));
    pimpl->knights_app.requestScreenChange(error_screen);
}

void GameManager::connectionFailed()
{
    // Go to ErrorScreen
    std::auto_ptr<Screen> error_screen(new ErrorScreen("Connection failed"));
    pimpl->knights_app.requestScreenChange(error_screen);
}

void GameManager::serverError(const std::string &error)
{
    // Go to ErrorScreen
    std::auto_ptr<Screen> error_screen(new ErrorScreen("Error: " + error));
    pimpl->knights_app.requestScreenChange(error_screen);
}

void GameManager::connectionAccepted(int server_version)
{
    // Note: currently server version is ignored, although it might be used in future
    // for backwards compatibility purposes.

    if (!pimpl->is_split_screen && !pimpl->is_lan_game) {
        // Go to LobbyScreen
        auto_ptr<Screen> lobby_screen(new LobbyScreen(pimpl->knights_client, pimpl->server_name));
        pimpl->knights_app.requestScreenChange(lobby_screen);
    }
}

void GameManager::joinGameAccepted(boost::shared_ptr<const ClientConfig> conf,
                                   int my_house_colour,
                                   const std::vector<std::string> &player_names,
                                   const std::vector<bool> &ready_flags,
                                   const std::vector<int> &house_cols,
                                   const std::vector<std::string> &observers)
{
    pimpl->my_obs_flag = false;
    pimpl->my_house_colour = my_house_colour;
    pimpl->is_split_screen = (pimpl->current_game_name == "#SplitScreenGame");
    pimpl->is_lan_game = (pimpl->current_game_name == "#LanGame");

    // update my player list, also set my_obs_flag if needed
    pimpl->game_namelist.clear();
    for (int i = 0; i < player_names.size(); ++i) {
        pimpl->game_namelist.add(player_names[i], false, ready_flags[i], house_cols[i]);
    }
    for (int i = 0; i < observers.size(); ++i) {
        pimpl->game_namelist.add(observers[i], true, false, 0);
        if (observers[i] == pimpl->my_player_name) pimpl->my_obs_flag = true;
    }

    pimpl->gui_invalid = true;
    
    // Store a pointer to the config, and the menu
    pimpl->client_config = conf;
    pimpl->menu = conf->menu.get();

    // Load the graphics and sounds
    for (std::vector<const Graphic *>::const_iterator it = conf->graphics.begin(); it != conf->graphics.end(); ++it) {
        pimpl->knights_app.getGfxManager().loadGraphic(**it);
    }
    for (std::vector<const Sound *>::const_iterator it = conf->sounds.begin(); it != conf->sounds.end(); ++it) {
        pimpl->knights_app.getSoundManager().loadSound(**it);
    }
    
    // Clear messages, and add new "Joined game" messages for lan games
    pimpl->chat_list.clear();
    if (pimpl->is_lan_game) {
        if (player_names.size() == 1) {
            pimpl->chat_list.add("LAN game created.");
        }
    }

    // Go to MenuScreen
    auto_ptr<Screen> menu_screen(new MenuScreen(pimpl->knights_client, !pimpl->is_split_screen));
    pimpl->knights_app.requestScreenChange(menu_screen);
}

void GameManager::joinGameDenied(const std::string &reason)
{
    pimpl->chat_list.add("Could not join game! " + reason);
    pimpl->current_game_name.clear();
    pimpl->gui_invalid = true;
}

namespace {
    struct SecondIsEmpty {
        bool operator()(const std::pair<std::string, std::string> &x) const
        {
            return x.second.empty();
        }
    };
}

void GameManager::passwordRequested(bool first_attempt)
{
    // Go to PasswordScreen
    auto_ptr<Screen> password_screen(new PasswordScreen(pimpl->knights_client, first_attempt));
    pimpl->knights_app.requestScreenChange(password_screen);
}

void GameManager::playerConnected(const std::string &name)
{
    pimpl->lobby_namelist.add(name, false, false, 0);

    // only show connection/disconnection messages if in main lobby.
    if (pimpl->current_game_name.empty()) {
        // Print msg
        pimpl->chat_list.add(name + " has connected.");

        // Pop window to front
        pimpl->knights_app.popWindowToFront();
    }

    pimpl->gui_invalid = true;
}

void GameManager::playerDisconnected(const std::string &name)
{
    pimpl->lobby_namelist.remove(name);

    if (pimpl->current_game_name.empty()) {
        pimpl->chat_list.add(name + " has disconnected.");
    }

    pimpl->gui_invalid = true;
}

void GameManager::updateGame(const std::string &game_name, int num_players, int num_observers, GameStatus status)
{
    std::vector<GameInfo>::iterator it;
    for (it = pimpl->game_infos.begin(); it != pimpl->game_infos.end(); ++it) {
        if (it->game_name == game_name) {
            break;
        }
    }
            
    if (it == pimpl->game_infos.end()) {
        pimpl->game_infos.push_back(GameInfo());
        it = pimpl->game_infos.end();
        --it;
        it->game_name = game_name;
    }

    it->num_players = num_players;
    it->num_observers = num_observers;
    it->status = status;
    pimpl->game_list_updated = true;
}

void GameManager::dropGame(const std::string &game_name)
{
    for (std::vector<GameInfo>::iterator it = pimpl->game_infos.begin(); it != pimpl->game_infos.end(); ++it) {
        if (it->game_name == game_name) {
            pimpl->game_infos.erase(it);   // invalidates "it"
            pimpl->game_list_updated = true;
            return;
        }
    }
}

void GameManager::updatePlayer(const std::string &player, const std::string &game, bool obs_flag)
{
    pimpl->lobby_namelist.remove(player);
    if (game.empty()) {
        pimpl->lobby_namelist.add(player, false, false, 0);
    }
    pimpl->gui_invalid = true;
}

void GameManager::playerList(const std::vector<ClientPlayerInfo> &player_list)
{
    pimpl->ingame_player_list.clear();
    for (std::vector<ClientPlayerInfo>::const_iterator it = player_list.begin(); it != player_list.end(); ++it) {
        std::ostringstream str;
        str << ColToText(it->house_colour) << " " << it->name;
        if (pimpl->ready_to_end.find(it->name) != pimpl->ready_to_end.end()) str << " (Ready)"; // indicate players who have clicked.
        str << "\t";
        if (it->kills >= 0) str << it->kills;
        str << "\t";
        if (it->deaths >= 0) str << it->deaths;
        str << "\t" << it->ping;
        pimpl->ingame_player_list.add(str.str());
    }
    pimpl->saved_client_player_info = player_list;
    pimpl->gui_invalid = true;
}

void GameManager::setTimeRemaining(int milliseconds)
{
    pimpl->time_remaining = milliseconds;
}

void GameManager::playerIsReadyToEnd(const std::string &player)
{
    pimpl->ready_to_end.insert(player);
    playerList(pimpl->saved_client_player_info);
}

void GameManager::setObsFlag(const std::string &name, bool new_obs_flag)
{
    // Update players list
    pimpl->game_namelist.alter(name, &new_obs_flag, 0, 0);
    
    // do message if needed
    if (name == pimpl->my_player_name) {
        pimpl->my_obs_flag = new_obs_flag;
        if (new_obs_flag) {
            pimpl->chat_list.add("You are now observing this game.");
        } else {
            pimpl->chat_list.add("You have joined the game.");
        }
    } else {
        if (new_obs_flag) {
            pimpl->chat_list.add(name + " is now observing this game.");
        } else {
            pimpl->chat_list.add(name + " has joined the game.");
        }
    }

    pimpl->gui_invalid = true;
}

void GameManager::leaveGame()
{
    // clear chat buffer.
    pimpl->chat_list.clear();

    pimpl->current_game_name.clear();
    pimpl->game_namelist.clear();
    pimpl->avail_house_colours.clear();
    
    // go to lobby (for internet games) or title (for split screen and lan games)
    if (pimpl->is_split_screen || pimpl->is_lan_game) {
        auto_ptr<Screen> title_screen(new TitleScreen);
        pimpl->knights_app.requestScreenChange(title_screen);
    } else {
        auto_ptr<Screen> lobby_screen(new LobbyScreen(pimpl->knights_client, pimpl->server_name));
        pimpl->knights_app.requestScreenChange(lobby_screen);
    }

    // unload gfx/sounds
    pimpl->knights_app.unloadGraphicsAndSounds();
    
    pimpl->gui_invalid = true;
}

void GameManager::setMenuSelection(const std::string &key, int val, const std::vector<int> &allowed_vals)
{
    // Update menu selections
    pimpl->menu_selections.setAllowedValues(key, allowed_vals);
    pimpl->menu_selections.setValue(key, val);

    // All players are no longer ready to start
    pimpl->game_namelist.clearReady();
    pimpl->my_ready_flag = false;
    
    // Update GUI
    updateMenuWidget(key);

    pimpl->gui_invalid = true;
}

void GameManager::setQuestDescription(const std::string &quest_descr)
{
    pimpl->quest_description = quest_descr;
    pimpl->gui_invalid = true;
}

const std::string & GameManager::getQuestDescription() const
{
    return pimpl->quest_description;
}

void GameManager::startGame(int ndisplays, const std::vector<std::string> &player_names, bool already_started)
{
    if (!pimpl->client_config) throw UnexpectedError("Cannot start game -- config not loaded");

    // reset the "end of game" timer for a new game
    pimpl->time_remaining = -1;

    // Go to InGameScreen
    std::auto_ptr<Screen> in_game_screen(new InGameScreen(pimpl->knights_app, 
                                                          pimpl->knights_client, 
                                                          pimpl->client_config,
                                                          ndisplays,
                                                          player_names,
                                                          pimpl->single_player,
                                                          pimpl->tutorial_mode));
    pimpl->knights_app.requestScreenChange(in_game_screen);

    // clear chat buffer when game begins
    pimpl->chat_list.clear();
    pimpl->saved_chat.clear();
    
    if (!player_names.empty()) {  // I am an observer
        if (already_started) {
            pimpl->chat_list.add("You are now observing this game.");
        } else {
            pimpl->chat_list.add("Game started. You are observing this game.");
        }
        if (player_names.size() > 2) {
            pimpl->chat_list.add("Use left and right arrow keys to switch between players.");
        }
    }

    // clear ready flags
    pimpl->game_namelist.clearReady();
    pimpl->my_ready_flag = false;

    // clear player list
    pimpl->ingame_player_list.clear();
    pimpl->saved_client_player_info.clear();
    pimpl->ready_to_end.clear();
    
    pimpl->gui_invalid = true;
}

void GameManager::gotoMenu()
{
    if (pimpl->tutorial_mode) {
        // In tutorial, just quit back to the title screen, as there is no menu screen for tutorial games.
        std::auto_ptr<Screen> title_screen(new TitleScreen);
        pimpl->knights_app.requestScreenChange(title_screen);
    } else if (pimpl->autostart_mode) {
        // In autostart mode we quit once the first game has finished
        pimpl->knights_app.requestQuit();        
    } else {
        // Normal game. Go back to the quest selection menu
        std::auto_ptr<Screen> menu_screen(new MenuScreen(pimpl->knights_client, !pimpl->is_split_screen, pimpl->saved_chat));
        pimpl->knights_app.requestScreenChange(menu_screen);

        // add separator lines to chat list.
        pimpl->chat_list.add("\n");
        pimpl->chat_list.add("\n");

        pimpl->gui_invalid = true;
    }
}

void GameManager::playerJoinedThisGame(const std::string &name, bool obs_flag, int house_col)
{
    if (!obs_flag) {
        pimpl->chat_list.add(name + " has joined the game.");       
        pimpl->game_namelist.add(name, false, false, house_col);
    } else {
        pimpl->chat_list.add(name + " is now observing this game.");
        pimpl->game_namelist.add(name, true, false, house_col);
    }
    pimpl->gui_invalid = true;
}

void GameManager::setPlayerHouseColour(const std::string &name, int house_col)
{
    pimpl->game_namelist.alter(name, 0, 0, &house_col);
    if (name == pimpl->my_player_name) pimpl->my_house_colour = house_col;
    pimpl->gui_invalid = true;
}

void GameManager::setAvailableHouseColours(const std::vector<Coercri::Color> &cols)
{
    pimpl->avail_house_colours = cols;
    pimpl->gui_invalid = true;
}

void GameManager::playerLeftThisGame(const std::string &name, bool obs_flag)
{
    std::string msg = name;
    if (obs_flag) {
        msg += " is no longer observing this game.";
    } else {
        msg += " has left the game.";
    }
    pimpl->chat_list.add(msg);
    pimpl->game_namelist.remove(name);
    pimpl->gui_invalid = true;
}

void GameManager::setReady(const std::string &name, bool ready)
{
    if (ready) {
        pimpl->chat_list.add(name + " is ready to start.");
    } else {
        pimpl->chat_list.add(name + " is no longer ready to start.");
    }
    pimpl->game_namelist.alter(name, 0, &ready, 0);
    if (name == pimpl->my_player_name) pimpl->my_ready_flag = ready;
    pimpl->gui_invalid = true;
}

void GameManager::chat(const std::string &whofrom, bool observer, const std::string &msg)
{
    std::string out = whofrom;
    if (observer) out += " (Observer)";
    out += ": ";
    out += msg;
    pimpl->chat_list.add(out);
    pimpl->gui_invalid = true;
}

void GameManager::announcement(const std::string &msg)
{
    pimpl->chat_list.add(msg);
    pimpl->gui_invalid = true;
}


//
// ctor/dtor
//

GameManager::GameManager(KnightsApp &ka, boost::shared_ptr<KnightsClient> kc, boost::shared_ptr<Coercri::Timer> timer, 
                         bool single_player, bool tutorial_mode, bool autostart, const std::string &my_player_name)
    : pimpl(new GameManagerImpl(ka, kc, timer, single_player, tutorial_mode, autostart, my_player_name))
{ }

