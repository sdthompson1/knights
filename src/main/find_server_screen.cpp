/*
 * find_server_screen.cpp
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
#include "find_server_screen.hpp"
#include "gui_button.hpp"
#include "loading_screen.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "knights_app.hpp"
#include "localization.hpp"
#include "make_scroll_area.hpp"
#include "my_exceptions.hpp"
#include "net_msgs.hpp"
#include "player_id.hpp"
#include "start_game_screen.hpp"
#include "tab_font.hpp"
#include "title_block.hpp"
#include "utf8string.hpp"
#include "utf8_text_field.hpp"

// coercri
#include "core/coercri_error.hpp"
#include "gcn/cg_font.hpp"
#include "network/udp_socket.hpp"

// boost, curl, std
#include <curl/curl.h>
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
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
        ServerInfo() : num_players(0), age(0) { }

        std::string ip_address;
        std::string hostname;
        Coercri::UTF8String host_username;  // Host player's display name
        LocalKey quest_key;                 // Current quest key (empty = selecting quest)
        int num_players;
        int age;   // Number of milliseconds ago that we last had an update from this server

        // used for sorting the displayed list.
        bool operator<(const ServerInfo &other) const {
            return hostname < other.hostname;
        }
    };

    struct ClientInfo {
        ClientInfo() : age(0) { }
        std::string ip_address;
        int age;
    };
    
    struct OlderThan {
        explicit OlderThan(int a) : age(a) { }
        bool operator()(const ServerInfo &si) const {
            return si.age != -1 && si.age > age;
        }
        bool operator()(const ClientInfo &ci) const {
            return ci.age > age;
        }
        int age;
    };

    struct AgeIsMinusOne {
        bool operator()(const ServerInfo &si) const {
            return si.age == -1;
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

    // used to look up host names for LAN games.
    class HostnameThread {
    public:
        HostnameThread(Coercri::NetworkDriver &drv,
                       boost::shared_ptr<boost::mutex> mut,
                       boost::shared_ptr<std::vector<ServerInfo> > si,
                       boost::shared_ptr<bool> flag,
                       const std::string &addr)
            : net_driver(drv), mutex(mut), server_infos(si), hostname_lookup_complete(flag), ip_address(addr)
        { }

        void operator()() {
            try {

                std::string hostname;
                {
                    // Only one thread is allowed to call resolveAddress() at a time
                    boost::lock_guard<boost::mutex> lock(net_driver_mutex);
                    hostname = net_driver.resolveAddress(ip_address);
                }

                boost::unique_lock lock(*mutex);
                for (std::vector<ServerInfo>::iterator it = server_infos->begin(); it != server_infos->end(); ++it) {
                    if (it->ip_address == ip_address) {
                        it->hostname = hostname;
                    }
                }

            } catch (...) {
                // Ignore any errors
            }

            boost::unique_lock lock(*mutex);
            *hostname_lookup_complete = true;
        }

    private:
        Coercri::NetworkDriver &net_driver;
        boost::shared_ptr<boost::mutex> mutex;
        boost::shared_ptr<std::vector<ServerInfo> > server_infos;
        boost::shared_ptr<bool> hostname_lookup_complete;
        std::string ip_address;

        static boost::mutex net_driver_mutex; // Prevents multiple threads accessing the NetworkDriver at the same time
    };

    boost::mutex HostnameThread::net_driver_mutex;
    
    class ServerList : public gcn::ListModel, boost::noncopyable {
    public:
        ServerList(KnightsApp &app,
                   Coercri::NetworkDriver& net_drv,
                   boost::shared_ptr<Coercri::UDPSocket> sock,
                   Coercri::Timer &tmr,
                   const std::string &err_msg);
        virtual int getNumberOfElements();
        virtual std::string getElementAt(int i);
        const ServerInfo * getServerAt(int i) const;
        bool refresh();
        void forceBroadcast();

    private:
        KnightsApp &knights_app;
        boost::shared_ptr<boost::mutex> mutex;   // protects server_infos and hostname_lookup_complete.
        boost::shared_ptr<std::vector<ServerInfo> > server_infos;
        std::vector<ClientInfo> client_infos;
        boost::shared_ptr<bool> hostname_lookup_complete;
        Coercri::NetworkDriver& network_driver;
        boost::shared_ptr<Coercri::UDPSocket> socket;
        Coercri::Timer &timer;
        unsigned int last_time;
        unsigned int last_broadcast_time;
        bool first_broadcast_sent;
        std::string err_msg;
    };

    ServerList::ServerList(KnightsApp &app,
                           Coercri::NetworkDriver& net_drv,
                           boost::shared_ptr<Coercri::UDPSocket> sock,
                           Coercri::Timer &tmr,
                           const std::string &emsg)
        : knights_app(app),
          mutex(new boost::mutex),
          server_infos(new std::vector<ServerInfo>),
          hostname_lookup_complete(new bool(false)),
          network_driver(net_drv),
          socket(sock),
          timer(tmr),
          last_time(0),
          last_broadcast_time(0),
          first_broadcast_sent(false),
          err_msg(emsg)
    {
    }

    int ServerList::getNumberOfElements()
    {
        boost::lock_guard<boost::mutex> lock(*mutex);
        
        if (socket) {
            return int(server_infos->size());
        } else {
            return 3;
        }
    }

    std::string ServerList::getElementAt(int i)
    {
        if (socket) {
            const ServerInfo *si = getServerAt(i);
            if (si) return ServerInfoToString(*si, knights_app.getLocalization());
            else return std::string();
        } else {
            if (i==0) {
                return "Cannot autodetect LAN games";
            } else if (i==1) {
                return err_msg;
            } else {
                return "Please enter address manually below.";
            }
        }
    }

    const ServerInfo * ServerList::getServerAt(int i) const
    {
        boost::lock_guard<boost::mutex> lock(*mutex);
        
        if (i < 0 || i >= int(server_infos->size())) return 0;
        else return &((*server_infos)[i]);
    }

    bool ServerList::refresh()
    {
        boost::lock_guard<boost::mutex> lock(*mutex);
        
        bool changed = false;
        
        if (socket) {
            
            // we send out pings every 3*Nclients seconds.
            // doing it this way ensures that the network doesn't get flooded with broadcasts if there are a lot of clients about.
            const int nclients = std::max(1, int(client_infos.size()));
            const int lan_broadcast_interval = 3000 * nclients;

            // after how long should we remove "dead" servers/clients from our lists?
            // well, if there are Nclients clients broadcasting every lan_broadcast_interval,
            // then we should expect each server to be broadcasting itself every lan_broadcast_interval/Nclients
            // on average. We use a multiple of this as our timeout.
            const int lan_broadcast_timeout = 3 * lan_broadcast_interval / nclients + 1000;

            static const int broadcast_pong_length = std::strlen(BROADCAST_PONG_HDR);
            
            const unsigned int time_now = timer.getMsec();
            const unsigned int interval_since_last_time = time_now - last_time;
            const unsigned int interval_since_last_broadcast = time_now - last_broadcast_time;
            last_time = time_now;

            // Send a broadcast if it's time            
            if (interval_since_last_broadcast > lan_broadcast_interval || !first_broadcast_sent) {

                last_broadcast_time = time_now;
                first_broadcast_sent = true;
                
                try {
                    socket->broadcast(BROADCAST_PORT, BROADCAST_PING_MSG);
                } catch (Coercri::CoercriError&) {
                    // If a broadcast fails then ignore the error. We can try again next time.
                }
                
            }

            // Age all server/client entries by appropriate amount
            for (std::vector<ServerInfo>::iterator it = server_infos->begin(); it != server_infos->end(); ++it) {
                if (it->age != -1) {
                    it->age += interval_since_last_time;
                }
            }
            for (std::vector<ClientInfo>::iterator it = client_infos.begin(); it != client_infos.end(); ++it) {
                it->age += interval_since_last_time;
            }

            // Listen for incoming responses
            std::string msg, address;
            int port;
            while (socket->receive(address, port, msg)) {

                if (msg.substr(0, broadcast_pong_length) == BROADCAST_PONG_HDR
                && msg.length() >= broadcast_pong_length + 3) {
                    // response from a server
                    const char type = msg[broadcast_pong_length];
                    const unsigned char nply_high = msg[broadcast_pong_length+1];
                    const unsigned char nply_low = msg[broadcast_pong_length+2];
                    const int nply = int(nply_high)*256 + int(nply_low);

                    // as we now only support LAN games, type must be 'L'
                    if (type == 'L') {

                        // Parse host_username and quest_key from the extended fields (if present).
                        // These are null-terminated strings appended after the 3 fixed data bytes.
                        Coercri::UTF8String host_username;
                        LocalKey quest_key;
                        const size_t extra_start = broadcast_pong_length + 3;
                        if (msg.length() > extra_start) {
                            const size_t nul1 = msg.find('\0', extra_start);
                            if (nul1 != std::string::npos) {
                                host_username = Coercri::UTF8String::fromUTF8Safe(msg.substr(extra_start, nul1 - extra_start));
                                const size_t nul2 = msg.find('\0', nul1 + 1);
                                if (nul2 != std::string::npos) {
                                    std::string qk = msg.substr(nul1 + 1, nul2 - nul1 - 1);
                                    if (!qk.empty()) {
                                        quest_key = LocalKey(qk);
                                    }
                                }
                            }
                        }

                        // See if we know about this server already. If so, update it, and set its age to zero.
                        bool found = false;
                        for (std::vector<ServerInfo>::iterator it = server_infos->begin(); it != server_infos->end(); ++it) {
                            if (it->ip_address == address) {
                                it->age = 0;
                                if (nply != it->num_players
                                    || it->host_username != host_username
                                    || it->quest_key != quest_key) {
                                    it->num_players = nply;
                                    it->host_username = host_username;
                                    it->quest_key = quest_key;
                                    changed = true;
                                }
                                found = true;
                                break;
                            }
                        }

                        // If not found then add it.
                        if (!found) {
                            ServerInfo si;
                            si.ip_address = address;
                            si.hostname = address;
                            si.host_username = host_username;
                            si.quest_key = quest_key;
                            si.num_players = nply;
                            si.age = 0;
                            server_infos->push_back(si);
                            changed = true;

                            // Start an update thread in the background to get the hostname.
                            boost::thread thr(HostnameThread(network_driver, mutex, server_infos, hostname_lookup_complete, address));
                        }
                    }
                } else if (msg == BROADCAST_PING_MSG) {
                    // response from another client. need to maintain a list of other clients on the
                    // network so that we can throttle broadcasts appropriately.
                    bool found = false;
                    for (std::vector<ClientInfo>::iterator it = client_infos.begin(); it != client_infos.end(); ++it) {
                        if (it->ip_address == address) {
                            it->age = 0;
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        ClientInfo ci;
                        ci.ip_address = address;
                        ci.age = 0;
                        client_infos.push_back(ci);
                    }
                }
            }

            // Drop any entries older than lan_broadcast_timeout.
            OlderThan old(lan_broadcast_timeout);
            if (std::find_if(server_infos->begin(), server_infos->end(), old) != server_infos->end()) {
                server_infos->erase(std::remove_if(server_infos->begin(), server_infos->end(), old),
                                    server_infos->end());
                changed = true;
            }
            client_infos.erase(std::remove_if(client_infos.begin(), client_infos.end(), old),
                               client_infos.end());
        }

        if (*hostname_lookup_complete) {
            // the hostname lookup thread has changed something.
            *hostname_lookup_complete = false;
            changed = true;
        }
        
        if (changed) {
            // sort by hostname.
            std::stable_sort(server_infos->begin(), server_infos->end());
        }
        
        return changed;
    }

    void ServerList::forceBroadcast()
    {
        first_broadcast_sent = false;
    }


    //
    // MyListBox
    // Reimplements ListBox::mouseClicked to listen for double clicks
    // and goes to ConnectingScreen if one is received.
    //

    class MyListBox : public gcn::ListBox {
    public:
        MyListBox(KnightsApp &ka, const ServerList &sl, FindServerScreenImpl &fss)
            : knights_app(ka), server_list(sl), find_srvr_impl(fss) { }
        virtual void mouseClicked(gcn::MouseEvent &mouse_event);
    private:
        KnightsApp &knights_app;
        const ServerList &server_list;
        FindServerScreenImpl &find_srvr_impl;
    };
}


class FindServerScreenImpl : public gcn::ActionListener, public gcn::SelectionListener {
public:
    FindServerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g);
    void action(const gcn::ActionEvent &event);
    void valueChanged(const gcn::SelectionEvent &event);
    void doUpdate();

    void initiateConnection(const std::string &address);
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

std::string FindServerScreenImpl::previous_address;

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
                // Initiate connection
                find_srvr_impl.initiateConnection(si->hostname);
            }
        }
    }
}

FindServerScreenImpl::FindServerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g)
    : knights_app(ka), window(win), gui(g)
{
    // Catch errors from creating the socket, which might fail if (for example) we start two copies of Knights
    // on the local machine (which can be useful for testing purposes).
    std::string err_msg;
    boost::shared_ptr<Coercri::UDPSocket> sock;
    try {
        // Create a UDP socket
        // Set port to -1 because we only want to listen to replies to our own outgoing msgs,
        // we do NOT want to listen for "unsolicited" incoming msgs.
        sock = knights_app.getLanNetworkDriver().createUDPSocket(-1, true);
    } catch (Coercri::CoercriError& e) {
        err_msg = e.what();
    }
    server_list.reset(new ServerList(knights_app, knights_app.getLanNetworkDriver(), sock,
                                     knights_app.getTimer(), err_msg));

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
    listbox->setFont(tab_font.get());
    listbox->setWidth(width);
    listbox->addSelectionListener(this);
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

    // make sure the first text field is focused initially
#ifdef ONLINE_PLATFORM
    address_field->requestFocus();
#else
    name_field->requestFocus();
#endif
}

void FindServerScreenImpl::gotoErrorDialog(const std::string &msg)
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

PlayerID FindServerScreenImpl::getPlayerID()
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

void FindServerScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to start game screen
        std::unique_ptr<Screen> start_screen(new StartGameScreen);
        knights_app.requestScreenChange(std::move(start_screen));

    } else if (event.getSource() == connect_button.get() || event.getSource() == address_field.get()) {
        // Initiate connection
        const std::string &address = address_field->getText();
        initiateConnection(address);

    } else if (event.getSource() == create_game_button.get()) {
        // Host a new LAN game
        createGame();

    } else if (event.getSource() == refresh_list_button.get()) {
        // Force an immediate broadcast ping
        server_list->forceBroadcast();

    } else if (event.getSource() == err_button.get()) {
        int w,h;
        window->getSize(w,h);
        centre->setSize(w,h);
        gui.setTop(centre.get());
        gui.logic();
        window->invalidateAll();
    }
}
        
void FindServerScreenImpl::valueChanged(const gcn::SelectionEvent &event)
{
    const ServerInfo *si = server_list->getServerAt(listbox->getSelected());
    if (si) {
        address_field->setText(si->hostname);
        address_field->logic();
        window->invalidateAll();
    }
}

void FindServerScreenImpl::doUpdate()
{
    const bool changed = server_list->refresh();
    if (changed) {
        AdjustListBoxSize(*listbox, *scroll_area);
        gui.logic();
        window->invalidateAll();
    }
}

void FindServerScreenImpl::createGame()
{
    PlayerID player_id = getPlayerID();
    if (player_id.empty()) return;
    const int server_port = knights_app.getConfigMap().getInt("port_number");
    std::unique_ptr<Screen> loading_screen(new LoadingScreen(server_port, player_id, false, false, false, false));
    knights_app.requestScreenChange(std::move(loading_screen));
}

void FindServerScreenImpl::initiateConnection(const std::string &address)
{
    if (address.empty()) {
        gotoErrorDialog("You must enter an address to connect to");
        return;
    }

    PlayerID player_id = getPlayerID();
    if (player_id.empty()) return;

    previous_address = address;

    const int port = knights_app.getConfigMap().getInt("port_number");
    std::unique_ptr<Screen> connecting_screen(new ConnectingScreen(address, port, player_id));
    knights_app.requestScreenChange(std::move(connecting_screen));
}


bool FindServerScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new FindServerScreenImpl(ka, win, gui));
    return true;
}

void FindServerScreen::update()
{
    if (pimpl) pimpl->doUpdate();
}
