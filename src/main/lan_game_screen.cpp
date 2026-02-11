/*
 * lan_game_screen.cpp
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

#include "adjust_list_box_size.hpp"
#include "config_map.hpp"
#include "connecting_screen.hpp"
#include "error_screen.hpp"
#include "lan_game_screen.hpp"
#include "gui_button.hpp"
#include "loading_screen.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "knights_app.hpp"
#include "localization.hpp"
#include "make_scroll_area.hpp"
#include "my_exceptions.hpp"
#include "mdns_discovery.hpp"
#include "player_id.hpp"
#include "start_game_screen.hpp"
#include "tab_font.hpp"
#include "title_block.hpp"
#include "utf8string.hpp"
#include "utf8_text_field.hpp"

// coercri
#include "core/coercri_error.hpp"
#include "gcn/cg_font.hpp"

// boost, std
#include "boost/scoped_ptr.hpp"
#include <cstring>
#include <numeric>
#include <set>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace {

    struct ServerInfo {
        ServerInfo() : num_players(0) { }

        std::string ip_address;
        std::string hostname;
        Coercri::UTF8String host_username;  // Host player's display name
        LocalKey quest_key;                 // Current quest key (empty = selecting quest)
        int num_players;

        // used for sorting the displayed list.
        bool operator<(const ServerInfo &other) const {
            return host_username < other.host_username
                || (host_username == other.host_username && hostname < other.hostname);
        }
    };

    std::string ServerInfoToString(const ServerInfo &si, const Localization &loc)
    {
        std::string result;

        // Host column
        result += si.host_username.asUTF8();
        result += '\t';

        // Address column
        result += si.hostname;
        result += '\t';

        // Players column
        result += std::to_string(si.num_players);
        result += '\t';

        // Status column
        if (si.quest_key == LocalKey()) {
            result += loc.get(LocalKey("selecting_quest")).asUTF8();
        } else {
            std::vector<LocalParam> params;
            params.push_back(LocalParam(si.quest_key));
            result += loc.get(LocalKey("playing_x"), params).asUTF8();
        }

        return result;
    }

    class ServerList : public gcn::ListModel {
    public:
        ServerList(KnightsApp &app, Coercri::Timer &tmr);
        virtual int getNumberOfElements();
        virtual std::string getElementAt(int i);
        const ServerInfo * getServerAt(int i) const;
        bool refresh(int &current_selection);
        void forceRefresh();
        bool hasError() const { return !(discoverer && discoverer->isValid()); }

    private:
        KnightsApp &knights_app;
        std::unique_ptr<MdnsDiscoverer> discoverer;
        Coercri::Timer &timer;
        std::vector<ServerInfo> server_infos;
    };

    ServerList::ServerList(KnightsApp &app, Coercri::Timer &tmr)
        : knights_app(app),
          discoverer(new MdnsDiscoverer),
          timer(tmr)
    {
    }

    int ServerList::getNumberOfElements()
    {
        if (discoverer && discoverer->isValid()) {
            return int(server_infos.size());
        } else {
            return 2;
        }
    }

    std::string ServerList::getElementAt(int i)
    {
        if (discoverer && discoverer->isValid()) {
            const ServerInfo *si = getServerAt(i);
            if (si) return ServerInfoToString(*si, knights_app.getLocalization());
            else return std::string();
        } else {
            if (i==0) {
                return "Cannot autodetect LAN games";
            } else {
                return "Please enter address manually below.";
            }
        }
    }

    const ServerInfo * ServerList::getServerAt(int i) const
    {
        if (i < 0 || i >= int(server_infos.size())) return 0;
        else return &server_infos[i];
    }

    bool ServerList::refresh(int &current_selection)
    {
        if (!discoverer || !discoverer->isValid()) return false;

        // Remember the currently selected IP address and hostname before refreshing
        std::string current_ip_address;
        UTF8String current_username;
        if (current_selection >= 0 && current_selection < server_infos.size()) {
            current_ip_address = server_infos[current_selection].ip_address;
            current_username = server_infos[current_selection].host_username;
        }

        // Poll the discoverer. This sends a query every 3 seconds and notifies us
        // if any responses came in since the previous query
        bool changed = discoverer->poll(timer.getMsec());

        if (changed) {
            // Rebuild server_infos from discoverer's service list
            server_infos.clear();
            for (const auto &svc : discoverer->getServices()) {
                ServerInfo si;
                si.ip_address = svc.ip_address;
                // Use IP address as hostname if the mDNS hostname is empty
                if (svc.hostname.empty()) {
                    si.hostname = svc.ip_address;
                } else {
                    si.hostname = svc.hostname;
                    // Strip trailing ".local." for display
                    const std::string suffix = ".local.";
                    if (si.hostname.size() > suffix.size() &&
                        si.hostname.compare(si.hostname.size() - suffix.size(), suffix.size(), suffix) == 0) {
                        si.hostname = si.hostname.substr(0, si.hostname.size() - suffix.size());
                    }
                }
                si.host_username = Coercri::UTF8String::fromUTF8Safe(svc.host_username);
                if (!svc.quest_key.empty()) {
                    si.quest_key = LocalKey(svc.quest_key);
                }
                si.num_players = svc.num_players;
                server_infos.push_back(si);
            }
            std::stable_sort(server_infos.begin(), server_infos.end());

            // Try to reselect the same item again
            current_selection = -1;
            for (int i = 0; i < server_infos.size(); ++i) {
                if (server_infos[i].ip_address == current_ip_address
                && server_infos[i].host_username == current_username) {
                    current_selection = i;
                    break;
                }
            }
        }

        return changed;
    }

    void ServerList::forceRefresh()
    {
        if (discoverer) discoverer->forceQuery();
    }


    //
    // MyListBox
    // Reimplements ListBox::mouseClicked to listen for double clicks
    // and goes to ConnectingScreen if one is received.
    //

    class MyListBox : public gcn::ListBox {
    public:
        MyListBox(KnightsApp &ka, const ServerList &sl, LanGameScreenImpl &fss)
            : knights_app(ka), server_list(sl), find_srvr_impl(fss) { }
        virtual void mouseClicked(gcn::MouseEvent &mouse_event);
    private:
        KnightsApp &knights_app;
        const ServerList &server_list;
        LanGameScreenImpl &find_srvr_impl;
    };
}


class LanGameScreenImpl : public gcn::ActionListener {
public:
    LanGameScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g);
    void action(const gcn::ActionEvent &event);
    void doUpdate();

    void initiateConnection(const std::string &address, const std::string &display_name);
    void createGame();

private:
    void gotoErrorDialog(const std::string &msg);
    PlayerID getPlayerID();
    
private:
    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;

    boost::scoped_ptr<ServerList> server_list;

    boost::scoped_ptr<gcn::Label> title_label;
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
#ifndef ONLINE_PLATFORM
    boost::scoped_ptr<gcn::Label> name_label;
    boost::scoped_ptr<UTF8TextField> name_field;
#endif
    boost::scoped_ptr<gcn::Label> label1;
    boost::scoped_ptr<TitleBlock> games_titleblock;
    boost::shared_ptr<gcn::Font> cg_font;
    std::unique_ptr<gcn::Font> tab_font;
    std::unique_ptr<gcn::ScrollArea> scroll_area;
    boost::scoped_ptr<gcn::ListBox> listbox;
    boost::scoped_ptr<gcn::Label> address_label;
    boost::scoped_ptr<UTF8TextField> address_field;
    boost::scoped_ptr<gcn::Button> connect_button;
    boost::scoped_ptr<gcn::Button> create_game_button;
    boost::scoped_ptr<gcn::Button> refresh_list_button;
    boost::scoped_ptr<gcn::Button> cancel_button;

    boost::scoped_ptr<GuiCentre> err_centre;
    boost::scoped_ptr<GuiPanel> err_panel;
    boost::scoped_ptr<gcn::Container> err_container;
    boost::scoped_ptr<gcn::Label> err_label;
    boost::scoped_ptr<gcn::Button> err_button;

    static std::string previous_address;
};

std::string LanGameScreenImpl::previous_address;

namespace
{
    void MyListBox::mouseClicked(gcn::MouseEvent &mouse_event)
    {
        // Make sure the base class gets called.
        gcn::ListBox::mouseClicked(mouse_event);

        // See if it was a double click
        if (mouse_event.getButton() == gcn::MouseEvent::LEFT && mouse_event.getClickCount() == 2) {
            const ServerInfo *si = server_list.getServerAt(getSelected());
            if (si) {
                // Initiate connection using IP address
                find_srvr_impl.initiateConnection(si->ip_address.empty() ? si->hostname : si->ip_address,
                                                  si->hostname);
            }
        }
    }
}

LanGameScreenImpl::LanGameScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(ka), window(win), gui(g)
{
    server_list.reset(new ServerList(knights_app, knights_app.getTimer()));

    // Set up TabFont for formatting columns: Host | Address | Players | Status
    std::vector<int> widths;
    widths.push_back(200);  // Host username column
    widths.push_back(250);  // Address column
    widths.push_back(80);   // Player count column
    widths.push_back(270);  // Status column

    cg_font.reset(new Coercri::CGFont(knights_app.getFont()));
    tab_font.reset(new TabFont(cg_font, widths));

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    int y = 5;
    const int width = std::accumulate(widths.begin(), widths.end(), 0);

    label1.reset(new gcn::Label("Available LAN Games (double-click to connect):"));

    title_label.reset(new gcn::Label("LAN Games"));
    title_label->setForegroundColor(gcn::Color(0,0,128));
    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

#ifndef ONLINE_PLATFORM
    name_label.reset(new gcn::Label("Player Name: "));
    name_field.reset(new UTF8TextField);
    name_field->adjustSize();
    name_field->setWidth(width - name_label->getWidth());
    name_field->setText(knights_app.getPlayerName().asUTF8());
    container->add(name_label.get(), pad, y + 2);
    container->add(name_field.get(), pad + name_label->getWidth(), y);
    y += name_field->getHeight() + pad*3/2;
#endif

    container->add(label1.get(), pad, y);
    y += label1->getHeight() + pad;

    std::vector<std::string> titles;
    titles.push_back("Host");
    titles.push_back("Address");
    titles.push_back("Players");
    titles.push_back("Status");
    games_titleblock.reset(new TitleBlock(titles, widths));
    games_titleblock->setBaseColor(gcn::Color(200, 200, 200));
    container->add(games_titleblock.get(), pad, y);
    y += games_titleblock->getHeight();

    listbox.reset(new MyListBox(knights_app, *server_list, *this));
    listbox->setListModel(server_list.get());
    if (!server_list->hasError()) {
        listbox->setFont(tab_font.get());
    }
    listbox->setWidth(width);
    scroll_area = MakeScrollArea(*listbox, width, 350 - games_titleblock->getHeight());
    container->add(scroll_area.get(), pad, y);
    y += scroll_area->getHeight() + pad*3/2;
    
    connect_button.reset(new GuiButton("Connect"));
    connect_button->addActionListener(this);

    address_label.reset(new gcn::Label("Address: "));
    const int address_field_width = width - address_label->getWidth() - connect_button->getWidth() - pad;

    address_field.reset(new UTF8TextField);
    address_field->adjustSize();
    address_field->setWidth(address_field_width);
    address_field->setText(previous_address);
    address_field->addActionListener(this);

    container->add(address_label.get(), pad, y + 5);
    container->add(address_field.get(), pad + address_label->getWidth(), y + 3);
    container->add(connect_button.get(), pad + address_label->getWidth() + address_field_width + pad, y);
    y += connect_button->getHeight() + 15;

    create_game_button.reset(new GuiButton("Create Game"));
    create_game_button->addActionListener(this);
    refresh_list_button.reset(new GuiButton("Refresh List"));
    refresh_list_button->addActionListener(this);
    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    container->add(create_game_button.get(), pad, y);
    container->add(refresh_list_button.get(), 25 + create_game_button->getWidth() + pad, y);
    container->add(cancel_button.get(), pad + width - cancel_button->getWidth(), y);

    container->setSize(2*pad + width, y + cancel_button->getHeight() + pad);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());

#ifndef ONLINE_PLATFORM
    // Focus the player name field initially, as typing something here is mandatory
    name_field->requestFocus();
#endif
}

void LanGameScreenImpl::gotoErrorDialog(const std::string &msg)
{
    err_container.reset(new gcn::Container);
    err_container->setOpaque(false);
    const int err_pad = 10;
    int err_y = err_pad;
    err_label.reset(new gcn::Label(msg));
    err_container->add(err_label.get(), err_pad, err_y);
    err_y += err_label->getHeight() + err_pad;
    err_button.reset(new GuiButton("Back"));
    err_button->addActionListener(this);
    err_container->add(err_button.get(), err_pad + err_label->getWidth() / 2 - err_button->getWidth() / 2, err_y);
    err_container->setSize(2*err_pad + err_label->getWidth(), err_y + err_button->getHeight() + err_pad);
    err_panel.reset(new GuiPanel(err_container.get()));
    err_centre.reset(new GuiCentre(err_panel.get()));

    int w,h;
    window->getSize(w,h);
    err_centre->setSize(w,h);
    gui.setTop(err_centre.get());
    gui.logic();
    window->invalidateAll();
}

PlayerID LanGameScreenImpl::getPlayerID()
{
#ifdef ONLINE_PLATFORM
    return knights_app.getOnlinePlatform().getCurrentUserId();
#else
    if (name_field->getText().empty()) {
        gotoErrorDialog("You must enter a player name");
        return PlayerID();
    } else {
        UTF8String name = UTF8String::fromUTF8Safe(name_field->getText());
        knights_app.setPlayerName(name);
        return PlayerID(name);
    }
#endif
}

void LanGameScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to start game screen
        std::unique_ptr<Screen> start_screen(new StartGameScreen);
        knights_app.requestScreenChange(std::move(start_screen));

    } else if (event.getSource() == connect_button.get() || event.getSource() == address_field.get()) {
        // Initiate connection
        const std::string &address = address_field->getText();
        previous_address = address;
        initiateConnection(address, "");  // Use entered address directly, without a separate "display name"

    } else if (event.getSource() == create_game_button.get()) {
        // Host a new LAN game
        createGame();

    } else if (event.getSource() == refresh_list_button.get()) {
        // Force an immediate mDNS query
        server_list->forceRefresh();

    } else if (event.getSource() == err_button.get()) {
        int w,h;
        window->getSize(w,h);
        centre->setSize(w,h);
        gui.setTop(centre.get());
        gui.logic();
        window->invalidateAll();
    }
}

void LanGameScreenImpl::doUpdate()
{
    int selection = listbox->getSelected();
    const bool changed = server_list->refresh(selection);
    if (changed) {
        listbox->setSelected(selection);
        AdjustListBoxSize(*listbox, *scroll_area);
        gui.logic();
        window->invalidateAll();
    }
}

void LanGameScreenImpl::createGame()
{
    PlayerID player_id = getPlayerID();
    if (player_id.empty()) return;
    const int server_port = knights_app.getConfigMap().getInt("port_number");
    std::unique_ptr<Screen> loading_screen(new LoadingScreen(server_port, player_id, false, false, false, false));
    knights_app.requestScreenChange(std::move(loading_screen));
}

void LanGameScreenImpl::initiateConnection(const std::string &address, const std::string &display_name)
{
    if (address.empty()) {
        gotoErrorDialog("You must enter an address to connect to");
        return;
    }

    PlayerID player_id = getPlayerID();
    if (player_id.empty()) return;

    const int port = knights_app.getConfigMap().getInt("port_number");
    std::unique_ptr<Screen> connecting_screen(new ConnectingScreen(address, port, display_name, player_id));
    knights_app.requestScreenChange(std::move(connecting_screen));
}


bool LanGameScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new LanGameScreenImpl(ka, win, gui));
    return true;
}

void LanGameScreen::update()
{
    if (pimpl) pimpl->doUpdate();
}
