/*
 * game_manager.cpp
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

#include "misc.hpp"

#include "client_config.hpp"
#include "config_map.hpp"
#include "copy_if.hpp"
#include "error_screen.hpp"
#include "game_manager.hpp"
#include "gfx_manager.hpp"
#include "graphic.hpp"
#include "gui_numeric_field.hpp"
#include "house_colour_font.hpp"
#include "in_game_screen.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "lobby_screen.hpp"
#include "menu.hpp"
#include "menu_item.hpp"
#include "menu_screen.hpp"
#include "my_ctype.hpp"
#include "my_exceptions.hpp"
#include "rstream_error.hpp"
#include "sound.hpp"
#include "sound_manager.hpp"
#include "text_formatter.hpp"
#include "title_screen.hpp"
#include "utf8string.hpp"
#include "vote_flags.hpp"

#include "guichan.hpp"

#include <algorithm>
#include <ctime>
#include <set>
#include <sstream>
#include <stdexcept>

namespace {
    struct ChatPrinter : Printer {
        ChatPrinter(std::deque<FormattedLine> &o, const gcn::Font *f) : output(o), font(f), printed(false) { }

        virtual int getTextWidth(const UTF8String &text) {
            return font ? font->getWidth(text.asUTF8()) : 0;
        }
        virtual int getTextHeight() { return 1; }  // we want to count lines not pixels.
        virtual void printLine(const UTF8String &text, int, bool) {
            FormattedLine f;
            f.firstline = !printed;
            f.text = text;
            output.push_back(f);
            printed = true;
        }

        std::deque<FormattedLine> &output;
        const gcn::Font *font;
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

    UTF8String AddTimestamp(const UTF8String &msg)
    {
        std::time_t time_since_epoch = std::time(0);
        std::tm * timeptr = std::localtime(&time_since_epoch);
        char buf[256];
        const int nchars = std::strftime(buf, sizeof(buf), "[%H:%M] ", timeptr);
        if (nchars > 0) return UTF8String::fromUTF8Safe(buf) + msg;
        else return msg;
    }
}

void ChatList::add(const UTF8String &msg_in)
{
    bool msg_empty = true;
    for (std::string::const_iterator it = msg_in.asUTF8().begin(); it != msg_in.asUTF8().end(); ++it) {
        if (!IsAsciiSpace(*it)) {
            msg_empty = false;
            break;
        }
    }

    const UTF8String msg_out = (do_timestamps && !msg_empty) ? AddTimestamp(msg_in) : msg_in;
    lines.push_back(msg_out);
    addFormattedLine(msg_out);
    if (lines.size() > max_msgs) {
        lines.pop_front();
        rmFormattedLine();
    }
}

void ChatList::setGuiParams(const gcn::Font *new_font, int new_width)
{
    font = new_font;
    doSetWidth(new_width);
}

void ChatList::doSetWidth(int new_width)
{
    width = new_width;
    formatted_lines.clear();
    for (std::deque<UTF8String>::const_iterator it = lines.begin(); it != lines.end(); ++it) {
        addFormattedLine(*it);
    }
}

void ChatList::clear()
{
    lines.clear();
    formatted_lines.clear();
}

void ChatList::addFormattedLine(const UTF8String &msg)
{
    if (do_format) {
        ChatPrinter p(formatted_lines, font);
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

NameList::NameList(const std::vector<Coercri::Color> &hc, KnightsApp &app)
    : house_cols(hc)
#ifdef ONLINE_PLATFORM
    , online_platform(app.getOnlinePlatform())
#endif
{ }

void NameList::add(const PlayerID &x, bool observer, bool ready, int house_col)
{
    Name n;
    n.id = x;
    n.observer = observer;
    n.ready = ready;
    n.house_col = house_col;
    names.push_back(n);
    sortNames();
}

void NameList::alter(const PlayerID &x, const bool *observer, const bool *ready, const int *house_col)
{
    std::vector<NameList::Name>::iterator it;
    for (it = names.begin(); it != names.end(); ++it) {
        if (it->id == x) break;
    }
    if (it != names.end()) {
        if (observer) it->observer = *observer;
        if (ready) it->ready = *ready;
        if (house_col) it->house_col = *house_col;
        if (observer != nullptr || house_col != nullptr) sortNames();
    }
}

bool NameList::query(const PlayerID &id, bool &observer, int &house_col)
{
    for (const Name &name : names) {
        if (name.id == id) {
            observer = name.observer;
            house_col = name.house_col;
            return true;
        }
    }
    return false;
}

void NameList::sortNames()
{
    struct CompareName {
        CompareName(NameList &namelist,
                    const std::map<int, UTF8String> &house_first_names)
            : namelist(namelist), house_first_names(house_first_names) {}

        bool operator()(const Name &lhs, const Name &rhs) const {
            // Observers go to the bottom of the list
            if (lhs.observer != rhs.observer) {
                return rhs.observer;
            }
            // Non-observers are sorted by house colours
            if (!lhs.observer && lhs.house_col != rhs.house_col) {
                return house_first_names.at(lhs.house_col) < house_first_names.at(rhs.house_col);
            }
            // Otherwise sort alphabetically by name (case-insensitive)
            UTF8String lhs_name = namelist.nameLookup(lhs.id);
            UTF8String rhs_name = namelist.nameLookup(rhs.id);
            return lhs_name.toUpper() < rhs_name.toUpper();
        }

        NameList &namelist;
        const std::map<int, UTF8String> &house_first_names;
    };

    // Houses are sorted by the alphabetically first player within the house
    std::map<int, UTF8String> house_first_names;
    for (const Name & name : names) {
        if (!name.observer) {
            const auto &find = house_first_names.find(name.house_col);
            UTF8String upper_name = nameLookup(name.id).toUpper();
            if (find == house_first_names.end() || upper_name < find->second) {
                house_first_names[name.house_col] = upper_name;
            }
        }
    }

    std::sort(names.begin(), names.end(), CompareName(*this, house_first_names));
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

void NameList::remove(const PlayerID &x)
{
    std::vector<NameList::Name>::iterator it;
    for (it = names.begin(); it != names.end(); ++it) {
        if (it->id == x) break;
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
    std::string result = nameLookup(names[i].id).asUTF8();
    if (names[i].observer) {
        result += " (Observer)";
    } else {
        if (names[i].ready) result += " (Ready)";
        if (names[i].house_col >= 0 && names[i].house_col < house_cols.size()) {
            result += "  ";
            result += ColToText(house_cols[names[i].house_col]).asUTF8();
        }
    }
    return result;
}

UTF8String NameList::nameLookup(const PlayerID &id) const
{
#ifdef ONLINE_PLATFORM
    return online_platform.lookupUserName(id);
#else
    return UTF8String::fromUTF8Safe(id.asString());
#endif
}


namespace {
    class MenuListModel : public gcn::ListModel {
    public:
        virtual int getNumberOfElements() { return elts.size(); }
        virtual std::string getElementAt(int i) {
            if (i >= 0 && i < elts.size()) {
                return elts[i].asUTF8();
            } else {
                return std::string();
            }
        }
        std::vector<UTF8String> elts;
        std::vector<int> vals;
    };

    class MenuWidgets {
    public:
        MenuWidgets(const Localization &loc, const MenuItem &mi, gcn::ActionListener *listener, gcn::SelectionListener *listener2)
            : menu_item(&mi)
        {
            const LocalKey &title_key = menu_item->getTitleKey();
            std::string title_utf8 = loc.get(title_key).asUTF8();
            label.reset(new gcn::Label(title_utf8));
            label->adjustSize();
            if (menu_item->isNumeric()) {
                // time entry (no of minutes) -- used for time limit
                textfield.reset(new GuiNumericField(menu_item->getNumDigits()));
                textfield->adjustSize();
                textfield->setWidth(textfield->getFont()->getWidth("9") *
                                    (menu_item->getNumDigits() + 1));
                std::string loc_utf8 = loc.get(menu_item->getSuffix()).asUTF8();
                label2.reset(new gcn::Label(loc_utf8));
                textfield->addActionListener(listener);
            } else {
                list_model.reset(new MenuListModel);
                dropdown.reset(new MyDropDown(list_model.get()));
                dropdown->adjustHeight();
                dropdown->addSelectionListener(listener2);
            }
        }

        // The menu item
        const MenuItem *menu_item;

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

    struct MenuChoices {
        // current menu settings
        int choice;
        std::vector<int> allowed_choices;  // for dropdowns
    };

    UTF8String LocalKeyOrIntegerToUTF8String(const Localization &loc, const LocalKeyOrInteger &lki)
    {
        if (lki.is_integer) {
            return UTF8String::fromUTF8Safe(std::to_string(lki.integer));
        } else {
            return loc.get(lki.local_key);
        }
    }

    int GetMenuWidgetWidth(const Localization &loc, const gcn::Font &font, const MenuWidgets &widgets)
    {
        const MenuItem & item = *widgets.menu_item;

        if (widgets.dropdown) {
            int widest_text = 0;
            for (int i = 0; i < item.getNumChoices(); ++i) {
                const LocalKeyOrInteger & lki = item.getChoice(i);
                UTF8String text_utf8 = LocalKeyOrIntegerToUTF8String(loc, lki);
                const int text_width = font.getWidth(text_utf8.asUTF8());
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

//
// VoteStatus implementation
//

void VoteStatus::setVote(const PlayerID &player, bool voted, bool is_me, int num_more)
{
    if (voted) {
        // Add player to end of vector, if they are not already there
        if (std::find(player_votes.begin(), player_votes.end(), player) == player_votes.end()) {
            player_votes.push_back(player);
        }
    } else {
        // Remove player from vector, if they are there
        auto it = std::find(player_votes.begin(), player_votes.end(), player);
        if (it != player_votes.end()) {
            player_votes.erase(it);
        }
    }

    num_more_needed = num_more;

    if (is_me) {
        i_voted = voted;
    }

    dirty = true;
}

void VoteStatus::clearVotes()
{
    player_votes.clear();
    num_more_needed = 0;
    i_voted = false;
    dirty = true;
}

UTF8String VoteStatus::getStatusMessage(const Localization &loc) const
{
    if (player_votes.empty()) {
        return UTF8String();
    }

    UTF8String msg1;
    if (player_votes.size() == 1) {
        msg1 = loc.get(LocalKey("x_wants_to_restart"),
                       std::vector<LocalParam>(1, LocalParam(player_votes[0])));

    } else if (player_votes.size() == 2) {
        std::vector<LocalParam> params;
        params.reserve(2);
        params.emplace_back(player_votes[0]);
        params.emplace_back(player_votes[1]);
        msg1 = loc.get(LocalKey("xy_want_to_restart"), params);

    } else {
        std::vector<LocalParam> params;
        int n = player_votes.size() - 2;
        params.reserve(player_votes.size());
        params.emplace_back(player_votes[0]);
        params.emplace_back(player_votes[1]);
        params.emplace_back(n);
        msg1 = loc.get(loc.pluralize(LocalKey("xyz_want_to_restart"), n), params);
    }

    UTF8String msg2;
    if (num_more_needed > 0) {
        msg2 = loc.get(loc.pluralize(LocalKey("more_vote_required"), num_more_needed),
                       std::vector<LocalParam>(1, LocalParam(num_more_needed)));
    }

    return msg1 + UTF8String::fromUTF8(" ") + msg2;
}

class GameManagerImpl {
public:
    GameManagerImpl(KnightsApp &ka, boost::shared_ptr<KnightsClient> kc, boost::shared_ptr<Coercri::Timer> timer_,
                    bool sgl_plyr, bool tut, bool autostart, bool allow_lobby, bool can_invite,
                    const PlayerID &my_player_id)
        : knights_app(ka), knights_client(kc), menu(0), gui_invalid(false), are_menu_widgets_enabled(true),
          timer(timer_), lobby_namelist(avail_house_colours, ka),
          game_list_updated(false), game_namelist(avail_house_colours, ka),
          my_house_colour(0), time_remaining(-1), my_obs_flag(false), my_ready_flag(false),
          is_split_screen(false), is_lan_game(false),
          chat_list(ka.getConfigMap().getInt("max_chat_lines"), true, true),   // want formatting and timestamps
          ingame_player_list(9999, false, false),  // unlimited number of lines; unformatted; no timestamps
          quest_rqmts_list(9999, false, false),    // ditto
          single_player(sgl_plyr), tutorial_mode(tut), autostart_mode(autostart),
          allow_lobby_screen(allow_lobby),
          can_invite(can_invite),
          my_player_id(my_player_id), doing_menu_widget_update(false), deathmatch_mode(false),
          game_in_progress(false)
    { }

    UTF8String usernameLookup(const PlayerID &id) const {
#ifdef ONLINE_PLATFORM
        return knights_app.getOnlinePlatform().lookupUserName(id);
#else
        return UTF8String::fromUTF8Safe(id.asString());
#endif
    }

    void showNewLeader();

    KnightsApp &knights_app;
    boost::shared_ptr<KnightsClient> knights_client;
    boost::shared_ptr<const ClientConfig> client_config;

    const Menu *menu;
    std::vector<MenuWidgets> menu_widgets_map;  // item_num -> MenuWidgets
    std::vector<MenuChoices> menu_choices;      // item_num -> MenuChoices
    std::vector<LocalMsg> quest_description;
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
    ChatList quest_rqmts_list;
    std::vector<ClientPlayerInfo> saved_client_player_info;
    std::set<PlayerID> ready_to_end; // IDs of players who have clicked mouse at end of game.
    VoteStatus vote_status;

    bool single_player;
    bool tutorial_mode;
    bool autostart_mode;
    bool allow_lobby_screen;
    bool can_invite;

    PlayerID my_player_id;
    UTF8String saved_chat;

    bool doing_menu_widget_update;
    bool deathmatch_mode;

    bool game_in_progress;
};

void GameManagerImpl::showNewLeader()
{
#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
    // Add message showing the new leader after host migration
    PlayerID leader_id = knights_app.getCurrentLeader();
    UTF8String msg;
    if (leader_id == knights_app.getOnlinePlatform().getCurrentUserId()) {
        msg = knights_app.getLocalization().get(LocalKey("you_are_now_leader"));
    } else {
        std::vector<LocalParam> params(1, LocalParam(leader_id));
        msg = knights_app.getLocalization().get(LocalKey("x_is_now_leader"), params);
    }
    chat_list.add(msg);
#endif
}

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

const LocalKey & GameManager::getMenuTitle() const
{
    if (!pimpl->menu) throw UnexpectedError("no menu exists");
    return pimpl->menu->getTitle();
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
        MenuWidgets menu_widgets(pimpl->knights_app.getLocalization(), item, listener, listener2);
        pimpl->menu_widgets_map.push_back(menu_widgets);
        max_label_w = std::max(max_label_w, menu_widgets.label->getWidth());
        max_widget_w = std::max(max_widget_w,
            GetMenuWidgetWidth(pimpl->knights_app.getLocalization(), *menu_widgets.label->getFont(), menu_widgets));
    }

    // resize widgets & add to container
    int y = initial_y;
    for (int i = 0; i < menu->getNumItems(); ++i) {
        const MenuItem &item(menu->getItem(i));
        MenuWidgets &mw = pimpl->menu_widgets_map[i];

        container.add(mw.label.get(), initial_x, y+1);

        if (mw.dropdown) {
            mw.dropdown->setWidth(max_widget_w);
            container.add(mw.dropdown.get(), initial_x + max_label_w + pad, y);
        } else {
            container.add(mw.textfield.get(), initial_x + max_label_w + pad, y);
            container.add(mw.label2.get(), mw.textfield->getX() + mw.textfield->getWidth() + pad, y+1);
        }

        y += std::max(mw.label->getHeight(), GetMenuWidgetHeight(mw)) + vspace;
        if (item.getSpaceAfter()) y += extra_vspace;
    }

    // make sure the list models get updated
    updateAllMenuWidgets();

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

    updateAllMenuWidgets();
}

void GameManager::getMenuStrings(std::vector<std::pair<UTF8String, UTF8String> > &menu_strings) const
{
    const Menu * menu = pimpl->menu;
    if (!menu) throw UnexpectedError("cannot get menu strings");

    const Localization & loc = pimpl->knights_app.getLocalization();

    for (int i = 0; i < menu->getNumItems(); ++i) {
        std::pair<UTF8String, UTF8String> p;
        const MenuItem &item = menu->getItem(i);
        p.first = loc.get(item.getTitleKey());
        if (item.isNumeric()) {
            std::ostringstream str;
            str << pimpl->menu_choices[i].choice;
            p.second = UTF8String::fromUTF8Safe(str.str());
        } else {
            const LocalKeyOrInteger &lki = item.getChoice(pimpl->menu_choices[i].choice);
            p.second = LocalKeyOrIntegerToUTF8String(loc, lki);
        }

        menu_strings.push_back(p);
        if (item.getSpaceAfter()) {
            menu_strings.push_back(std::make_pair(UTF8String(), UTF8String()));
        }
    }
}

bool GameManager::getMenuWidgetInfo(gcn::Widget *source, int &out_item, int &out_choice) const
{
    const Menu *menu = pimpl->menu;
    if (!menu) return false;

    for (int i = 0; i < menu->getNumItems(); ++i) {
        const MenuItem &item = menu->getItem(i);
        const MenuWidgets &mw = pimpl->menu_widgets_map[i];

        const gcn::DropDown *dropdown = mw.dropdown.get();
        const GuiNumericField *textfield = mw.textfield.get();
        if (dropdown == source) {
            // Find out what was selected
            const int selected = dropdown->getSelected();
            if (selected >= 0 && selected < mw.list_model->vals.size()) {
                out_item = i;
                out_choice = mw.list_model->vals[selected];
                return true;
            }
        } else if (textfield == source) {
            const std::string & text = textfield->getText();
            if (text.empty()) {
                out_item = i;
                out_choice = 0;
                return true;
            } else {
                std::istringstream str(text);
                int result = 0;
                str >> result;
                if (str) {
                    out_item = i;
                    out_choice = result;
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

void GameManager::updateMenuWidget(int item_num)
{
    Sentinel sentinel(pimpl->doing_menu_widget_update);

    ASSERT (item_num >= 0 && item_num < pimpl->menu_widgets_map.size());

    MenuWidgets & mw = pimpl->menu_widgets_map[item_num];
    const MenuChoices & mc = pimpl->menu_choices[item_num];

    // this works for both dropdowns and numeric fields
    const bool is_enabled = (mc.allowed_choices.size() != 1 || mc.allowed_choices[0] != 0)
        && pimpl->are_menu_widgets_enabled;
    const gcn::Color fg_col = is_enabled ? gcn::Color(0,0,0) : gcn::Color(170,170,170);

    if (mw.dropdown) {

        const Localization &loc = pimpl->knights_app.getLocalization();

        mw.list_model->elts.clear();
        mw.list_model->vals.clear();

        for (std::vector<int>::const_iterator it = mc.allowed_choices.begin(); it != mc.allowed_choices.end(); ++it) {
            const LocalKeyOrInteger &lki = mw.menu_item->getChoice(*it);
            mw.list_model->elts.push_back(LocalKeyOrIntegerToUTF8String(loc, lki));
            mw.list_model->vals.push_back(*it);
        }

        for (int idx = 0; idx < mw.list_model->vals.size(); ++idx) {
            const int val_at_idx = mw.list_model->vals[idx];
            if (val_at_idx == mc.choice) {
                mw.dropdown->setSelected(idx);
                break;
            }
        }

        mw.dropdown->setForegroundColor(fg_col);
        mw.dropdown->setEnabled(is_enabled);

    } else {

        std::ostringstream str;
        if (mc.choice > 0) str << mc.choice;
        mw.textfield->setText(str.str());

        mw.textfield->setEnabled(is_enabled);
        mw.textfield->setForegroundColor(fg_col);
    }

    pimpl->gui_invalid = true;
}

void GameManager::updateAllMenuWidgets()
{
    for (int i = 0; i < pimpl->menu_widgets_map.size(); ++i) {
        updateMenuWidget(i);
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

std::function<UTF8String(const PlayerID&)> GameManager::getPlayerNameLookup() const
{
#ifdef ONLINE_PLATFORM
    return [this](const PlayerID& id) -> UTF8String {

        OnlinePlatform &online_platform = pimpl->knights_app.getOnlinePlatform();
        const Localization &localization = pimpl->knights_app.getLocalization();

        UTF8String name = online_platform.lookupUserName(id);

        for (const auto& player : pimpl->saved_client_player_info) {
            if (player.id == id && player.client_state == ClientState::DISCONNECTED) {
                return localization.get(LocalKey("x_disconnected"), name);
            }
        }

        return name;
    };
#else
    return [](const PlayerID &id) -> UTF8String {
        return UTF8String::fromUTF8Safe(id.asString());
    };
#endif
}

bool GameManager::getMyObsFlag() const
{
    return pimpl->my_obs_flag;
}

int GameManager::getNumConnectedPlayers() const
{
    int num = 0;
    for (const auto &player : pimpl->saved_client_player_info) {
        if (player.client_state == ClientState::NORMAL) {
            ++num;
        }
    }
    return num;
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

ChatList & GameManager::getQuestRequirementsList() const
{
    return pimpl->quest_rqmts_list;
}

VoteStatus & GameManager::getVoteStatus() const
{
    return pimpl->vote_status;
}

bool GameManager::setSavedChat(const UTF8String &s)
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
    UTF8String msg = pimpl->knights_app.getLocalization().get(LocalKey("connection_lost"));
    std::unique_ptr<Screen> error_screen(new ErrorScreen(msg));
    pimpl->knights_app.requestScreenChange(std::move(error_screen));
}

void GameManager::connectionFailed()
{
    // Go to ErrorScreen
    UTF8String msg = pimpl->knights_app.getLocalization().get(LocalKey("connection_failed"));
    std::unique_ptr<Screen> error_screen(new ErrorScreen(msg));
    pimpl->knights_app.requestScreenChange(std::move(error_screen));
}

void GameManager::serverError(const LocalMsg &error)
{
    // Go to ErrorScreen
    const Localization &loc = pimpl->knights_app.getLocalization();
    UTF8String msg = loc.get(error);
    std::unique_ptr<Screen> error_screen(new ErrorScreen(msg));
    pimpl->knights_app.requestScreenChange(std::move(error_screen));
}

void GameManager::connectionAccepted(int server_version)
{
    // Note: currently server version is ignored, although it might be used in future
    // for backwards compatibility purposes.

    if (pimpl->allow_lobby_screen) {
        // Go to LobbyScreen
        std::unique_ptr<Screen> lobby_screen(new LobbyScreen(pimpl->knights_client, pimpl->server_name));
        pimpl->knights_app.requestScreenChange(std::move(lobby_screen));
    }
}

void GameManager::joinGameAccepted(boost::shared_ptr<const ClientConfig> conf,
                                   int my_house_colour,
                                   const std::vector<PlayerID> &player_ids,
                                   const std::vector<bool> &ready_flags,
                                   const std::vector<int> &house_cols,
                                   const std::vector<PlayerID> &observers,
                                   bool already_started)
{
    pimpl->game_in_progress = already_started;
    pimpl->my_obs_flag = false;
    pimpl->my_house_colour = my_house_colour;
    pimpl->is_split_screen = (pimpl->current_game_name == "#SplitScreenGame");
    pimpl->is_lan_game = (pimpl->current_game_name == "#LanGame");

    // update my player list, also set my_obs_flag if needed
    pimpl->game_namelist.clear();
    for (int i = 0; i < player_ids.size(); ++i) {
        pimpl->game_namelist.add(player_ids[i], false, ready_flags[i], house_cols[i]);
    }
    for (int i = 0; i < observers.size(); ++i) {
        pimpl->game_namelist.add(observers[i], true, false, 0);
        if (observers[i] == pimpl->my_player_id) pimpl->my_obs_flag = true;
    }

    pimpl->gui_invalid = true;

    // Store a pointer to the config, and the menu
    pimpl->client_config = conf;
    destroyMenuWidgets();
    pimpl->menu = conf->menu.get();

    // Resize the MenuChoices to the proper size
    pimpl->menu_choices.resize(pimpl->menu->getNumItems());

    // Pre-load all the graphics and sounds, so that they don't have to be
    // loaded during gameplay, and also so that any errors can be detected
    // at this point
    try {
        for (std::vector<const Graphic *>::const_iterator it = conf->graphics.begin(); it != conf->graphics.end(); ++it) {
            pimpl->knights_app.getGfxManager().loadGraphic(**it);
        }
        for (std::vector<const Sound *>::const_iterator it = conf->sounds.begin(); it != conf->sounds.end(); ++it) {
            pimpl->knights_app.getSoundManager().loadSound(**it);
        }
    } catch (RStreamError &err) {
        if (pimpl->knights_client->allowUntrustedStrings()) {
            throw;
        } else {
            // hide the filename, just in case the host put something nasty there
            throw RStreamError("#####", err.getErrorMsg());
        }
    }

    // Clear messages, and add new "Joined game" messages for lan games
    bool should_clear = true;
#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
    should_clear = !(pimpl->knights_app.getHostMigrationState() == HostMigrationState::RECONNECTING
                     || pimpl->knights_app.getHostMigrationState() == HostMigrationState::MIGRATING);
#endif
    if (should_clear) {
        pimpl->chat_list.clear();
    }
    if (pimpl->is_lan_game && player_ids.size() == 1) {
        pimpl->chat_list.add(UTF8String::fromUTF8("LAN game created."));
#if defined(ONLINE_PLATFORM) && defined(USE_VM_LOBBY)
    } else if (pimpl->knights_app.getHostMigrationState() == HostMigrationState::MIGRATING
               && !already_started) {
        // If not already started, then show the new leader here. Otherwise, show it
        // in the startGame call.
        pimpl->showNewLeader();
#endif
    }

    // Go to MenuScreen, if applicable
    if (!pimpl->tutorial_mode         // tutorial should never go into menu screen
    && !pimpl->game_in_progress) {    // don't go to menu screen if observation of a game is in progress (#214), or if we just host-migrated into an in-progress game
        std::unique_ptr<Screen> menu_screen(new MenuScreen(
            pimpl->knights_client,
            !pimpl->is_split_screen,
            pimpl->can_invite,
            pimpl->saved_chat));
        pimpl->knights_app.requestScreenChange(std::move(menu_screen));
    }

#ifdef ONLINE_PLATFORM
    if (!already_started) {
        // If not already started, set host migration state to IN_GAME here; otherwise,
        // set it in the startGame call.
        pimpl->knights_app.setHostMigrationStateInGame();
    }
#endif
}

void GameManager::joinGameDenied(const LocalKey &reason)
{
    const Localization &loc = pimpl->knights_app.getLocalization();
    std::vector<LocalParam> params;
    params.push_back(LocalParam(reason));
    UTF8String msg = loc.get(LocalKey("couldnt_join_game"), params);
    pimpl->chat_list.add(msg);
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

void GameManager::playerConnected(const PlayerID &id)
{
    pimpl->lobby_namelist.add(id, false, false, 0);

    // only show connection/disconnection messages if in main lobby.
    if (pimpl->allow_lobby_screen && pimpl->current_game_name.empty()) {
        // Print msg
        UTF8String name = pimpl->usernameLookup(id);
        pimpl->chat_list.add(name + UTF8String::fromUTF8(" has connected."));

        // Pop window to front
        pimpl->knights_app.popWindowToFront();
    }

    pimpl->gui_invalid = true;
}

void GameManager::playerDisconnected(const PlayerID &id)
{
    pimpl->lobby_namelist.remove(id);

    if (pimpl->allow_lobby_screen && pimpl->current_game_name.empty()) {
        UTF8String name = pimpl->usernameLookup(id);
        pimpl->chat_list.add(name + UTF8String::fromUTF8(" has disconnected."));
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

void GameManager::updatePlayer(const PlayerID &player, const std::string &game, bool obs_flag)
{
    pimpl->lobby_namelist.remove(player);
    if (game.empty()) {
        pimpl->lobby_namelist.add(player, false, false, 0);
    }
    pimpl->gui_invalid = true;
}

void GameManager::playerList(const std::vector<ClientPlayerInfo> &player_list_orig)
{
    // First sort the player list
    std::vector<ClientPlayerInfo> player_list = player_list_orig;

    struct ComparePlayer {
        ComparePlayer(GameManagerImpl &impl,
                      const std::map<Coercri::Color, UTF8String> &house_first_names)
            : impl(impl),
              house_first_names(house_first_names) {}

        bool operator()(const ClientPlayerInfo &lhs, const ClientPlayerInfo &rhs) const {
            // Observers go to bottom of list
            if (lhs.client_state == ClientState::OBSERVER && rhs.client_state != ClientState::OBSERVER) {
                return false;
            }
            if (lhs.client_state != ClientState::OBSERVER && rhs.client_state == ClientState::OBSERVER) {
                return true;
            }
            // Non-observers are sorted by house colours
            if (lhs.client_state != ClientState::OBSERVER && lhs.house_colour != rhs.house_colour) {
                return house_first_names.at(lhs.house_colour) < house_first_names.at(rhs.house_colour);
            }
            // Otherwise sort alphabetically by name (case-insensitive)
            UTF8String lhs_name = impl.usernameLookup(lhs.id);
            UTF8String rhs_name = impl.usernameLookup(rhs.id);
            return lhs_name.toUpper() < rhs_name.toUpper();
        }

        GameManagerImpl &impl;
        const std::map<Coercri::Color, UTF8String> &house_first_names;
    };

    // Houses are sorted by the alphabetically first player within the house
    std::map<Coercri::Color, UTF8String> house_first_names;
    for (const ClientPlayerInfo & info : player_list) {
        if (info.client_state != ClientState::OBSERVER) {
            const auto &find = house_first_names.find(info.house_colour);
            UTF8String upper_name = pimpl->usernameLookup(info.id).toUpper();
            if (find == house_first_names.end() || upper_name < find->second) {
                house_first_names[info.house_colour] = upper_name;
            }
        }
    }

    std::sort(player_list.begin(), player_list.end(), ComparePlayer(*pimpl, house_first_names));

    // Now transfer the sorted list to the UI (ingame_player_list).
    pimpl->ingame_player_list.clear();
    for (std::vector<ClientPlayerInfo>::const_iterator it = player_list.begin(); it != player_list.end(); ++it) {
        UTF8String str = ColToText(it->house_colour);
        str += UTF8String::fromUTF8(" ");
        str += pimpl->usernameLookup(it->id);

        if (pimpl->ready_to_end.find(it->id) != pimpl->ready_to_end.end()) {
            str += UTF8String::fromUTF8(" (Ready)"); // indicate players who have clicked.
        } else if (it->client_state == ClientState::DISCONNECTED) {
            str += UTF8String::fromUTF8(" (Disconnected)");
        } else if (it->client_state == ClientState::ELIMINATED) {
            str += UTF8String::fromUTF8(" (Eliminated)");
        } else if (it->client_state == ClientState::OBSERVER) {
            str += UTF8String::fromUTF8(" (Observer)");
        }

        str += UTF8String::fromUTF8("\t");
        if (pimpl->deathmatch_mode) {
            if (it->frags >= -999) str += UTF8String::fromUTF8Safe(std::to_string(it->frags));
        } else {
            if (it->kills >= 0) str += UTF8String::fromUTF8Safe(std::to_string(it->kills));
            str += UTF8String::fromUTF8("\t");
            if (it->deaths >= 0) str += UTF8String::fromUTF8Safe(std::to_string(it->deaths));
        }
        if (it->client_state != ClientState::DISCONNECTED) {
            str += UTF8String::fromUTF8("\t");
            str += UTF8String::fromUTF8Safe(std::to_string(it->ping));
        }
        pimpl->ingame_player_list.add(str);
    }

    // Also save the sorted list (in case we need it later) and set the gui_invalid flag.
    pimpl->saved_client_player_info = player_list;
    pimpl->gui_invalid = true;
}

void GameManager::setTimeRemaining(int milliseconds)
{
    pimpl->time_remaining = milliseconds;
}

void GameManager::playerIsReadyToEnd(const PlayerID &player)
{
    pimpl->ready_to_end.insert(player);
    playerList(pimpl->saved_client_player_info);
}

void GameManager::playerVotedToRestart(const PlayerID &player, uint8_t flags, int num_more_needed)
{
    const bool vote = (flags & VF_VOTE) != 0;
    const bool is_me = (flags & VF_IS_ME) != 0;
    const bool show_msg = (flags & VF_SHOW_MSG) != 0;
    const bool restarting = (flags & VF_GAME_ENDING) != 0;
    pimpl->vote_status.setVote(player, vote, is_me, num_more_needed);

    if (show_msg) {
        const Localization &loc = pimpl->knights_app.getLocalization();
        const char *key;
        if (restarting) {
            key = "game_restarted";
        } else if (vote) {
            key = "x_voted_to_restart";
        } else {
            key = "x_cancelled_vote";
        }
        UTF8String msg = loc.get(LocalKey(key), std::vector<LocalParam>(1, LocalParam(player)));
        pimpl->chat_list.add(msg);
    }

    pimpl->gui_invalid = true;
}

void GameManager::setObsFlag(const PlayerID &id, bool new_obs_flag)
{
    // Update players list
    pimpl->game_namelist.alter(id, &new_obs_flag, 0, 0);

    // do message if needed
    if (id == pimpl->my_player_id) {
        pimpl->my_obs_flag = new_obs_flag;
        if (new_obs_flag) {
            pimpl->chat_list.add(UTF8String::fromUTF8("You are now observing this game."));
        } else {
            pimpl->chat_list.add(UTF8String::fromUTF8("You have joined the game."));
        }
    } else {
        UTF8String name = pimpl->usernameLookup(id);
        if (new_obs_flag) {
            pimpl->chat_list.add(name + UTF8String::fromUTF8(" is now observing this game."));
        } else {
            pimpl->chat_list.add(name + UTF8String::fromUTF8(" has joined the game."));
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

    // go to lobby (for internet games) or title (for split screen, lan, and online platform games)
    if (!pimpl->allow_lobby_screen) {
        std::unique_ptr<Screen> title_screen(new TitleScreen);
        pimpl->knights_app.requestScreenChange(std::move(title_screen));
    } else {
        std::unique_ptr<Screen> lobby_screen(new LobbyScreen(pimpl->knights_client, pimpl->server_name));
        pimpl->knights_app.requestScreenChange(std::move(lobby_screen));
    }

    // unload gfx/sounds
    pimpl->knights_app.unloadGraphicsAndSounds();

    pimpl->gui_invalid = true;
}

void GameManager::setMenuSelection(int item_num, int choice_num, const std::vector<int> &allowed_vals)
{
    // Update menu selections
    MenuChoices &mc = pimpl->menu_choices.at(item_num); // will throw if bad item_num given by server
    mc.allowed_choices = allowed_vals;
    mc.choice = choice_num;

    // Update GUI
    // Note: May be slightly inefficient to update *everything* every time there is a SET_MENU_SELECTION.
    // But it does at least guarantee that the menu is in a consistent state, w/ no race conditions if
    // multiple players are updating menu at same time...
    updateAllMenuWidgets();

    pimpl->gui_invalid = true;
}

LocalKey GameManager::getQuestMessageCode() const
{
    // Our approach is to grab the topmost localization string from
    // the menu that isn't equal to LocalKey("custom"). This works
    // well enough for now.

    for (int i = 0; i < pimpl->menu->getNumItems(); ++i) {
        const MenuItem &item = pimpl->menu->getItem(i);

        // We only care about dropdown items
        if (item.isNumeric()) continue;

        // Find the current setting
        const int current_choice = pimpl->menu_choices[i].choice;
        const LocalKeyOrInteger &lki = item.getChoice(current_choice);

        // If it is a LocalKey, and not LocalKey("custom"), return it
        if (!lki.is_integer) {
            const LocalKey & choice_key = lki.local_key;
            if (choice_key != LocalKey("custom")) {
                return choice_key;
            }
        }
    }

    // If not found, just return "none"...
    return LocalKey("none");
}

void GameManager::setQuestDescription(const std::vector<LocalMsg> &paragraphs)
{
    pimpl->quest_description = paragraphs;
    pimpl->gui_invalid = true;
}

UTF8String GameManager::getQuestDescription() const
{
    const Localization &loc = pimpl->knights_app.getLocalization();

    UTF8String result;
    for (const LocalMsg &paragraph : pimpl->quest_description) {
        if (!result.empty()) result += UTF8String::fromUTF8("\n\n");
        result += loc.get(paragraph);
    }

    return result;
}

void GameManager::startGame(int ndisplays, bool deathmatch_mode,
                            const std::vector<PlayerID> &player_ids, bool already_started)
{
    if (!pimpl->client_config) throw UnexpectedError("Cannot start game -- config not loaded");

    // reset the "end of game" timer for a new game
    pimpl->time_remaining = -1;

    // Clear chat if this is a fresh game started from scratch
    if (!already_started) {
        pimpl->saved_chat = UTF8String();
    }

    // Go to InGameScreen
    std::unique_ptr<Screen> in_game_screen(new InGameScreen(pimpl->knights_app,
                                                            pimpl->knights_client,
                                                            pimpl->client_config,
                                                            ndisplays,
                                                            deathmatch_mode,
                                                            player_ids,
                                                            pimpl->single_player,
                                                            pimpl->tutorial_mode,
                                                            pimpl->saved_chat));

    // We need to execute the screen change immediately ("true"
    // parameter) because the server might immediately start sending
    // us gameplay messages, and we need to be ready to receive them.
    // That means our KnightsCallbacks must be created and ready - and
    // the KnightsCallbacks are only created once the InGameScreen
    // starts up!
    pimpl->knights_app.requestScreenChange(std::move(in_game_screen), true);

    pimpl->deathmatch_mode = deathmatch_mode;

    // add separator lines to chat (or clear it, if single player mode)
    if (pimpl->single_player) {
        pimpl->chat_list.clear();
    } else if (!already_started) {
        pimpl->chat_list.add(UTF8String::fromUTF8("\n"));
        pimpl->chat_list.add(UTF8String::fromUTF8("\n"));
    }

    bool host_migration_occurred = false;
    bool i_am_observer = !player_ids.empty();  // for this particular msg the server only sends player_ids to observers

#ifdef USE_VM_LOBBY
    host_migration_occurred =
        already_started &&
        pimpl->knights_app.getHostMigrationState() == HostMigrationState::MIGRATING;
#endif

    if (host_migration_occurred) {
        // Host migration completed - show the new host
        pimpl->showNewLeader();

    } else if (i_am_observer) {
        // Observer, either starting new game or rejoining existing one
        if (already_started) {
            pimpl->chat_list.add(UTF8String::fromUTF8("You are now observing this game."));
        } else {
            pimpl->chat_list.add(UTF8String::fromUTF8("Game started. You are observing this game."));
        }
        if (player_ids.size() > 2) {
            pimpl->chat_list.add(UTF8String::fromUTF8("Use arrow keys (left/right and up/down) to switch between players."));
        }

    } else if (already_started) {
        // Player, rejoining an existing game
        pimpl->chat_list.add(UTF8String::fromUTF8("You have reconnected to this game."));

    } else {
        // Player, starting a new game
        pimpl->chat_list.add(UTF8String::fromUTF8("Game started."));
    }

    // clear ready flags
    pimpl->game_namelist.clearReady();
    pimpl->my_ready_flag = false;

    // clear player list
    pimpl->ingame_player_list.clear();
    pimpl->saved_client_player_info.clear();
    pimpl->ready_to_end.clear();

    // clear quest requirements
    pimpl->quest_rqmts_list.clear();

    // clear vote status
    pimpl->vote_status.clearVotes();

    pimpl->gui_invalid = true;

    pimpl->game_in_progress = true;

    // Notify updated game status - quest is now in progress
    pimpl->knights_app.setQuestMessageCode(getQuestMessageCode());

#ifdef ONLINE_PLATFORM
    // We are now in HostMigrationState::IN_GAME, if we weren't already
    pimpl->knights_app.setHostMigrationStateInGame();
#endif
}

void GameManager::gotoMenu()
{
    if (pimpl->tutorial_mode) {
        // In tutorial, just quit back to the title screen, as there is no menu screen for tutorial games.
        std::unique_ptr<Screen> title_screen(new TitleScreen);
        pimpl->knights_app.requestScreenChange(std::move(title_screen));
    } else if (pimpl->autostart_mode) {
        // In autostart mode we quit once the first game has finished
        pimpl->knights_app.requestQuit();
    } else {
        // Normal game. Go back to the quest selection menu
        std::unique_ptr<Screen> menu_screen(new MenuScreen(
            pimpl->knights_client,
            !pimpl->is_split_screen,
            pimpl->can_invite,
            pimpl->saved_chat));
        pimpl->knights_app.requestScreenChange(std::move(menu_screen));

        // add separator lines to chat list.
        pimpl->chat_list.add(UTF8String::fromUTF8("\n"));
        pimpl->chat_list.add(UTF8String::fromUTF8("\n"));

        pimpl->gui_invalid = true;

        // Notify updated game status - quest is now empty
        pimpl->knights_app.setQuestMessageCode(LocalKey());
    }

    pimpl->game_in_progress = false;
}

void GameManager::playerJoinedThisGame(const PlayerID &id, bool obs_flag, int house_col)
{
    UTF8String name = pimpl->usernameLookup(id);
    if (!obs_flag) {
        if (pimpl->game_in_progress) {
            // The only way to join a game while it is in progress is via reconnect - there
            // is no way for "new" players to enter a game in progress (currently)
            pimpl->chat_list.add(name + UTF8String::fromUTF8(" has reconnected."));
        } else {
            pimpl->chat_list.add(name + UTF8String::fromUTF8(" has joined the game."));
        }
        pimpl->game_namelist.add(id, false, false, house_col);
    } else {
        pimpl->chat_list.add(name + UTF8String::fromUTF8(" is now observing this game."));
        pimpl->game_namelist.add(id, true, false, house_col);
    }
    pimpl->gui_invalid = true;
}

void GameManager::setPlayerHouseColour(const PlayerID &id, int house_col)
{
    pimpl->game_namelist.alter(id, 0, 0, &house_col);
    if (id == pimpl->my_player_id) pimpl->my_house_colour = house_col;
    pimpl->gui_invalid = true;
}

void GameManager::setAvailableHouseColours(const std::vector<Coercri::Color> &cols)
{
    pimpl->avail_house_colours = cols;
    pimpl->gui_invalid = true;
}

void GameManager::playerLeftThisGame(const PlayerID &id, bool obs_flag)
{
    UTF8String msg = pimpl->usernameLookup(id);
    if (obs_flag) {
        msg += UTF8String::fromUTF8(" is no longer observing this game.");
    } else {
        msg += UTF8String::fromUTF8(" has left the game.");
    }
    pimpl->chat_list.add(msg);
    pimpl->game_namelist.remove(id);
    pimpl->gui_invalid = true;
}

void GameManager::setReady(const PlayerID &id, bool ready)
{
    UTF8String name = pimpl->usernameLookup(id);
    if (ready) {
        pimpl->chat_list.add(name + UTF8String::fromUTF8(" is ready to start."));
    } else {
        pimpl->chat_list.add(name + UTF8String::fromUTF8(" is no longer ready to start."));
    }
    pimpl->game_namelist.alter(id, 0, &ready, 0);
    if (id == pimpl->my_player_id) pimpl->my_ready_flag = ready;
    pimpl->gui_invalid = true;
}

void GameManager::deactivateReadyFlags()
{
    pimpl->game_namelist.clearReady();
    pimpl->my_ready_flag = false;
    pimpl->gui_invalid = true;
}

namespace {
    // Check if a message is team chat (if allowed). If so, remove the "/t" prefix.
    // Also removes leading spaces.
    bool CheckTeamChat(const UTF8String &msg,
                       bool team_chat_allowed,
                       UTF8String &msg_stripped)
    {
        const std::string& utf8 = msg.asUTF8();
        bool is_team = false;

        // strip away any leading spaces
        int idx = 0;
        while (idx < utf8.size() && utf8[idx] == ' ') ++idx;

        if (team_chat_allowed
        && idx + 1 < utf8.size()
        && utf8[idx] == '/'
        && utf8[idx + 1] == 't') {
            // strip away "/t" and any further leading spaces
            is_team = true;
            idx += 2;
            while (idx < utf8.size() && utf8[idx] == ' ') ++idx;
        }

        // return the stripped message and the is_team flag
        msg_stripped = UTF8String::fromUTF8(utf8.substr(idx));
        return is_team;
    }
}

void GameManager::chat(const PlayerID &whofrom, const UTF8String &msg)
{
    // Convert player ID to name
    UTF8String out = pimpl->usernameLookup(whofrom);

    // Get house colour and observer status of the sender
    bool observer = false;
    int house_col = -1;
    pimpl->game_namelist.query(whofrom, observer, house_col);

    // "/t" prefix is honoured if game in progress (i.e. not on quest selection menu)
    // and the sender is not an observer
    bool team_chat_allowed = !observer && pimpl->game_in_progress;

    // Strip away leading spaces and "/t" prefix if applicable
    UTF8String msg_stripped;
    bool is_team = CheckTeamChat(msg, team_chat_allowed, msg_stripped);

    // Filter the message text if required
#ifdef ONLINE_PLATFORM
    msg_stripped = pimpl->knights_app.getOnlinePlatform().filterChatMessage(whofrom, msg_stripped);
#endif

    // If the message was blocked (or was empty to begin with), we drop it.
    if (msg_stripped.empty()) {
        return;
    }

    // Add "(Team)" or "(Observer)" marker if needed
    if (is_team) {
        if (pimpl->my_house_colour != house_col || pimpl->my_obs_flag) {
            // Drop this message as it is for a different team
            return;
        }
        out += UTF8String::fromUTF8(" (Team)");
    } else if (observer) {
        out += UTF8String::fromUTF8(" (Observer)");
    }

    // Add colon and the stripped message itself
    out += UTF8String::fromUTF8(": ");
    out += msg_stripped;

    // Add it to the chat list in the GUI
    pimpl->chat_list.add(out);
    pimpl->gui_invalid = true;
}

void GameManager::announcementLoc(const LocalMsg &msg, bool err)
{
    pimpl->chat_list.add(pimpl->knights_app.getLocalization().get(msg));
    pimpl->gui_invalid = true;

    if (err && pimpl->single_player && !pimpl->game_in_progress) {
        // This is needed because there is no way to display error messages on the
        // quest selection screen in single player mode
        throw ExceptionBase(msg);
    }
}


//
// ctor/dtor
//

GameManager::GameManager(KnightsApp &ka,
                         boost::shared_ptr<KnightsClient> kc,
                         boost::shared_ptr<Coercri::Timer> timer,
                         bool single_player,
                         bool tutorial_mode,
                         bool autostart,
                         bool allow_lobby,
                         bool can_invite,
                         const PlayerID &my_player_id)
    : pimpl(new GameManagerImpl(ka, kc, timer, single_player, tutorial_mode, autostart,
                                allow_lobby, can_invite, my_player_id))
{ }
