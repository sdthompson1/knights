/*
 * find_server_screen.cpp
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

#include "adjust_list_box_size.hpp"
#include "config_map.hpp"
#include "connecting_screen.hpp"
#include "error_screen.hpp"
#include "find_server_screen.hpp"
#include "gui_button.hpp"
#include "gui_centre.hpp"
#include "gui_panel.hpp"
#include "knights_app.hpp"
#include "make_scroll_area.hpp"
#include "metaserver_urls.hpp"
#include "my_exceptions.hpp"
#include "net_msgs.hpp"
#include "start_game_screen.hpp"
#include "utf8string.hpp"

// coercri
#include "core/coercri_error.hpp"
#include "gcn/cg_font.hpp"
#include "network/udp_socket.hpp"

// boost, curl, std
#include <curl/curl.h>
#include "boost/scoped_ptr.hpp"
#include "boost/thread.hpp"
#include <cstring>
#include <set>
#include <sstream>

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

namespace {

    struct ServerInfo {
        ServerInfo() : port(0), num_players(0), age(0), password_required(false) { }
        
        std::string ip_address;
        std::string hostname;
        std::string description;
        int port;
        int num_players;
        int age;   // Number of milliseconds ago that we last had an update from this server, or -1 for metaserver entries.
        bool password_required;
        
        // used for sorting the displayed list.
        bool operator<(const ServerInfo &other) const {
            return hostname < other.hostname ? true 
                : hostname > other.hostname ? false
                : port < other.port;
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
    
    std::string ServerInfoToString(const ServerInfo &si, bool internet)
    {
        std::ostringstream str;

        if (si.hostname.find(':') != std::string::npos) {
            str << "[" << si.hostname << "]:";
        } else {
            str << si.hostname << ":" << si.port;
        }

        str << "  -  " << si.num_players << " player";
        if (si.num_players != 1) str << "s";
        if (si.password_required) {
            str << "  -  password required";
        }
        if (!si.description.empty()) {
            str << "  -  " << si.description;
        }

        return str.str();
    }


    class MetaserverThread : boost::noncopyable {
    public:
        MetaserverThread();
        ~MetaserverThread();

        void operator()();
        std::vector<ServerInfo> getServerInfos();
        bool needUpdate();

    private:
        void doUpdate();
        void parseServerList(const std::string &result);
        
    private:
        boost::mutex mutex;   // protects server_infos, need_update
        std::vector<ServerInfo> server_infos;
        bool need_update;
        int fail_count;
        
        CURL *curl;
    };

    size_t WriteCallback(void *ptr, size_t size, size_t nmemb, void *data)
    {
        size_t total_size = size * nmemb;
        std::string *result = static_cast<std::string*>(data);
        char* char_ptr = static_cast<char*>(ptr);
        result->append(char_ptr, total_size);
        return total_size;
    }

    size_t ReadCallback(void *ptr, size_t size, size_t nmemb, void *data)
    {
        return 0;
    }

    MetaserverThread::MetaserverThread()
        : need_update(true), fail_count(0)
    {
        curl = curl_easy_init();
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_READFUNCTION, ReadCallback);
        curl_easy_setopt(curl, CURLOPT_USERAGENT,
                         "Knights/" KNIGHTS_VERSION " (" KNIGHTS_PLATFORM "; " KNIGHTS_WEBSITE ")");
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 30);
    }

    MetaserverThread::~MetaserverThread()
    {
        curl_easy_cleanup(curl);
    }

    void MetaserverThread::doUpdate()
    {
        std::string result;
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &result);

        curl_easy_setopt(curl, CURLOPT_URL, g_metaserver_query_url);
        const bool success =
            curl_easy_perform(curl) == 0;

        bool do_update;
        if (success) {
            do_update = true;
            fail_count = 0;
        } else {
            // don't clear the list just because one update fails.
            // wait for 3 failures in a row before doing that.
            ++fail_count;
            if (fail_count >= 3) {
                do_update = true;
                fail_count = 3;
            } else {
                do_update = false;
            }
        }

        if (do_update) {
            boost::lock_guard<boost::mutex> lock(mutex);
            need_update = true;
            server_infos.clear();
            if (success) {
                parseServerList(result);
            }
        }
    }

    void MetaserverThread::parseServerList(const std::string &result)
    {
        // we assume the mutex is locked at this point

        std::istringstream str(result);

        ServerInfo si;
        bool first_time = true;
        
        while (str) {
            std::string line;
            std::getline(str, line);

            if (str.eof() || line == "[SERVER]") {
                if (!first_time) {
                    si.age = -1;
                    server_infos.push_back(si);
                    si = ServerInfo();
                }
                first_time = false;
            } else {
                // find '='
                const std::string::size_type pos = line.find('=');
                if (pos != std::string::npos) {
                    std::string key = line.substr(0, pos);
                    std::string val = line.substr(pos+1, std::string::npos);
                    if (key == "ip_address") {
                        si.ip_address = val;
                    } else if (key == "hostname") {
                        si.hostname = val;
                    } else if (key == "port") {
                        si.port = std::atoi(val.c_str());
                    } else if (key == "num_players") {
                        si.num_players = std::atoi(val.c_str());
                    } else if (key == "description") {
                        si.description = val;
                    } else if (key == "password_required") {
                        si.password_required = std::atoi(val.c_str()) != 0;
                    }
                }
            }
        }
    }
    
    void MetaserverThread::operator()()
    {
        try {
            while (1) {
                doUpdate();
                // update every 10 seconds.
                boost::this_thread::sleep(boost::posix_time::seconds(10));
            }
        } catch (...) {
            // Don't allow exceptions to escape the thread.
        }
    }

    std::vector<ServerInfo> MetaserverThread::getServerInfos()
    {
        std::vector<ServerInfo> result;
        {
            boost::lock_guard<boost::mutex> lock(mutex);
            result = server_infos;
        }
        return result;
    }

    bool MetaserverThread::needUpdate()
    {
        boost::lock_guard<boost::mutex> lock(mutex);
        const bool result = need_update;
        need_update = false;
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
            std::string hostname;
            {
                // Only one thread is allowed to call resolveAddress() at a time
                boost::lock_guard<boost::mutex> lock(net_driver_mutex);
                hostname = net_driver.resolveAddress(ip_address);
            }

            boost::lock_guard<boost::mutex> lock(*mutex);
            for (std::vector<ServerInfo>::iterator it = server_infos->begin(); it != server_infos->end(); ++it) {
                if (it->ip_address == ip_address) {
                    it->hostname = hostname;
                }
            }
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
        ServerList(Coercri::NetworkDriver& net_drv,
                   boost::shared_ptr<Coercri::UDPSocket> sock, 
                   Coercri::Timer &tmr, 
                   bool inet,
                   const std::string &err_msg);
        ~ServerList();
        virtual int getNumberOfElements();
        virtual std::string getElementAt(int i);
        const ServerInfo * getServerAt(int i) const;
        bool refresh();

    private:
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
        bool internet;
        std::string err_msg;

        boost::scoped_ptr<MetaserverThread> metaserver_thread_obj;
        boost::thread metaserver_thread;
    };

    ServerList::ServerList(Coercri::NetworkDriver& net_drv,
                           boost::shared_ptr<Coercri::UDPSocket> sock,
                           Coercri::Timer &tmr, 
                           bool inet,
                           const std::string &emsg)
        : mutex(new boost::mutex),
          server_infos(new std::vector<ServerInfo>),
          hostname_lookup_complete(new bool(false)),
          network_driver(net_drv),
          socket(sock),
          timer(tmr),
          last_time(0),
          last_broadcast_time(0),
          first_broadcast_sent(false),
          internet(inet),
          err_msg(emsg)
    {
        if (internet) {
            // start the metaserver thread.
            metaserver_thread_obj.reset(new MetaserverThread);
            metaserver_thread = boost::thread(boost::ref(*metaserver_thread_obj));
        }
    }

    ServerList::~ServerList()
    {
        // Stop the metaserver thread if necessary.
        if (internet) {
            metaserver_thread.interrupt();
            metaserver_thread.join();
        }
    }

    int ServerList::getNumberOfElements()
    {
        boost::lock_guard<boost::mutex> lock(*mutex);
        
        if (socket || internet) {
            return int(server_infos->size());
        } else {
            return 3;
        }
    }

    std::string ServerList::getElementAt(int i)
    {
        if (socket || internet) {
            const ServerInfo *si = getServerAt(i);
            if (si) return ServerInfoToString(*si, internet);
            else return std::string();
        } else {
            if (i==0) {
                return "Cannot autodetect LAN games";
            } else if (i==1) {
                return err_msg;
            } else {
                return "Please enter address and port manually below.";
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
                && msg.length() >= broadcast_pong_length + 5) {
                    // response from a server
                    const unsigned char high_byte = msg[broadcast_pong_length];
                    const unsigned char low_byte = msg[broadcast_pong_length+1];
                    const char type = msg[broadcast_pong_length+2];
                    const int port = int(high_byte)*256 + int(low_byte);
                    const unsigned char nply_high = msg[broadcast_pong_length+3];
                    const unsigned char nply_low = msg[broadcast_pong_length+4];
                    const int nply = int(nply_high)*256 + int(nply_low);

                    // only display games of the right type (internet or lan).
                    if ((type == 'I' && internet) || (type == 'L' && !internet)) {

                        // See if we know about this server already. If so, update it, and set its age to zero.
                        bool found = false;
                        for (std::vector<ServerInfo>::iterator it = server_infos->begin(); it != server_infos->end(); ++it) {
                            if (it->ip_address == address && it->port == port) {
                                it->age = 0;
                                if (nply != it->num_players) {
                                    it->num_players = nply;
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
                            si.port = port;
                            si.num_players = nply;
                            si.age = 0;
                            si.description = type == 'I' ? msg.substr(broadcast_pong_length + 5) : std::string();
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

        // Check the metaserver thread if necessary.
        if (metaserver_thread_obj) {
            const bool need_update = metaserver_thread_obj->needUpdate();
            if (need_update) {
                std::vector<ServerInfo> new_server_infos = metaserver_thread_obj->getServerInfos();

                // Try to find the hostnames for these new servers.
                std::set<std::string> ip_addresses;
                for (std::vector<ServerInfo>::iterator it = new_server_infos.begin(); it != new_server_infos.end(); ++it) {
                    if (it->hostname == it->ip_address) {
                        // The metaserver didn't provide the hostname.
                        // First find out whether this server is in the server_infos list already (if so,
                        // it means we tried to look up the hostname on a previous run through this loop,
                        // and should not do so again this time).
                        bool found = false;
                        for (std::vector<ServerInfo>::const_iterator it2 = server_infos->begin(); it2 != server_infos->end(); ++it2) {
                            if (it->ip_address == it2->ip_address) {
                                it->hostname = it2->hostname;
                                found = true;
                                break;
                            }
                        }
                        
                        // Check whether this is a duplicate server. (No need to look up the same ip address more than once.)
                        if (ip_addresses.find(it->ip_address) != ip_addresses.end()) found = true;
                        else ip_addresses.insert(it->ip_address);

                        if (!found) {
                            // Ok, we do not know the hostname for this ip address.
                            // Start a lookup.
                            boost::thread thr(HostnameThread(network_driver, mutex, server_infos, hostname_lookup_complete, it->ip_address));
                        }
                    }
                }

                // Delete all metaserver entries from server_infos, and then re-insert the entries from new_server_infos.
                server_infos->erase(std::remove_if(server_infos->begin(), server_infos->end(), AgeIsMinusOne()),
                                    server_infos->end());
                std::copy(new_server_infos.begin(), new_server_infos.end(), std::back_inserter(*server_infos));
                changed = true;
            }
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

    
    //
    // MyListBox
    // Reimplements ListBox::mouseClicked to listen for double clicks
    // and goes to MenuScreen or LobbyScreen if one is received.
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
    FindServerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g, const std::string &title, bool inet);
    void action(const gcn::ActionEvent &event);
    void valueChanged(const gcn::SelectionEvent &event);
    void doUpdate();

    void initiateConnection(const std::string &address, int port);

private:
    void gotoErrorDialog(const std::string &msg);
    
private:
    KnightsApp &knights_app;
    boost::shared_ptr<Coercri::Window> window;
    gcn::Gui &gui;
    bool internet;
    bool allow_conn;

    boost::scoped_ptr<ServerList> server_list;

    boost::scoped_ptr<gcn::Label> title_label;
    boost::scoped_ptr<GuiCentre> centre;
    boost::scoped_ptr<GuiPanel> panel;
    boost::scoped_ptr<gcn::Container> container;
    boost::scoped_ptr<gcn::Label> name_label;
    boost::scoped_ptr<gcn::TextField> name_field;
    boost::scoped_ptr<gcn::Label> label1;
    std::unique_ptr<gcn::ScrollArea> scroll_area;
    boost::scoped_ptr<gcn::ListBox> listbox;
    boost::scoped_ptr<gcn::Label> address_label, port_label;
    boost::scoped_ptr<gcn::TextField> address_field, port_field;
    boost::scoped_ptr<gcn::Button> connect_button;
    boost::scoped_ptr<gcn::Button> cancel_button;

    boost::scoped_ptr<GuiCentre> err_centre;
    boost::scoped_ptr<GuiPanel> err_panel;
    boost::scoped_ptr<gcn::Container> err_container;
    boost::scoped_ptr<gcn::Label> err_label;
    boost::scoped_ptr<gcn::Button> err_button;

    static std::string previous_address[2];  // 0=lan, 1=internet
    static int previous_port[2];  // 0=lan, 1=internet
};

std::string FindServerScreenImpl::previous_address[2];
int FindServerScreenImpl::previous_port[2] = {-1, -1};

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
                find_srvr_impl.initiateConnection(si->hostname, si->port);
            }
        }
    }
}

FindServerScreenImpl::FindServerScreenImpl(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &g, const std::string &title, bool inet)
    : knights_app(ka), window(win), gui(g), internet(inet), allow_conn(true)
{
    // Catch errors from creating the socket, which might fail if (for example) we start two copies of Knights
    // on the local machine (which can be useful for testing purposes).
    std::string err_msg;
    boost::shared_ptr<Coercri::UDPSocket> sock;
    try {
        // Create a UDP socket
        // Set port to -1 because we only want to listen to replies to our own outgoing msgs,
        // we do NOT want to listen for "unsolicited" incoming msgs.
        sock = knights_app.getNetworkDriver().createUDPSocket(-1, true);
    } catch (Coercri::CoercriError& e) {
        err_msg = e.what();
    }
    server_list.reset(new ServerList(knights_app.getNetworkDriver(), sock, knights_app.getTimer(), 
                                     internet, err_msg));

    container.reset(new gcn::Container);
    container->setOpaque(false);

    const int pad = 10;
    int y = 5;

    if (internet) {
        label1.reset(new gcn::Label("Available Servers (double-click to connect):"));
    } else {
        label1.reset(new gcn::Label("Available LAN Games (double-click to connect):"));
    }
    const int width = 900;

    title_label.reset(new gcn::Label(title));
    title_label->setForegroundColor(gcn::Color(0,0,128));
    container->add(title_label.get(), pad + width/2 - title_label->getWidth()/2, y);
    y += title_label->getHeight() + 2*pad;

    name_label.reset(new gcn::Label("Player Name: "));
    name_field.reset(new gcn::TextField);
    name_field->adjustSize();
    name_field->setWidth(width - name_label->getWidth());
    name_field->setText(knights_app.getPlayerName().asLatin1());
    container->add(name_label.get(), pad, y + 1);
    container->add(name_field.get(), pad + name_label->getWidth(), y);
    y += name_field->getHeight() + pad*3/2;
    
    container->add(label1.get(), pad, y);
    y += label1->getHeight() + pad;

    listbox.reset(new MyListBox(knights_app, *server_list, *this));
    listbox->setListModel(server_list.get());
    listbox->setWidth(width);
    listbox->addSelectionListener(this);
    scroll_area = MakeScrollArea(*listbox, width, 350);
    container->add(scroll_area.get(), pad, y);
    y += scroll_area->getHeight() + pad*3/2;
    
    address_label.reset(new gcn::Label("Address: "));
    port_label.reset(new gcn::Label(" Port: "));
    const int port_field_width = port_label->getFont()->getWidth("123456789");
    const int address_field_width = width - port_field_width - address_label->getWidth() - port_label->getWidth();
    
    address_field.reset(new gcn::TextField);
    address_field->adjustSize();
    address_field->setWidth(address_field_width);
    address_field->setText(previous_address[internet?1:0]);

    port_field.reset(new gcn::TextField);
    port_field->adjustSize();
    port_field->setWidth(port_field_width);
    std::ostringstream oss;
    const int pport = previous_port[internet?1:0];
    if (pport >= 0) oss << previous_port[internet?1:0];
    port_field->setText(oss.str());

    container->add(address_label.get(), pad, y + 1);
    container->add(address_field.get(), pad + address_label->getWidth(), y);
    container->add(port_label.get(), pad + address_label->getWidth() + address_field_width, y + 1);
    container->add(port_field.get(), pad + address_label->getWidth() + address_field_width + port_label->getWidth(), y);
    y += address_field->getHeight() + pad;

    cancel_button.reset(new GuiButton("Cancel"));
    cancel_button->addActionListener(this);
    connect_button.reset(new GuiButton("Connect"));
    connect_button->addActionListener(this);
    container->add(connect_button.get(), pad, y);
    container->add(cancel_button.get(), pad + width - cancel_button->getWidth(), y);

    container->setSize(2*pad + width, y + cancel_button->getHeight() + pad);

    panel.reset(new GuiPanel(container.get()));
    centre.reset(new GuiCentre(panel.get()));
    gui.setTop(centre.get());

    // make sure the first text field is focused initially
    if (name_field) {
        name_field->requestFocus();
    }
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

void FindServerScreenImpl::action(const gcn::ActionEvent &event)
{
    if (event.getSource() == cancel_button.get()) {
        // Go back to start game screen
        std::unique_ptr<Screen> start_screen(new StartGameScreen);
        knights_app.requestScreenChange(std::move(start_screen));

    } else if (event.getSource() == connect_button.get()) {
        // Initiate connection
        const std::string &address = address_field->getText();
        int port = -1;
        if (port_field->getText().empty()) {
            port = knights_app.getConfigMap().getInt("port_number");  // default
        } else {
            std::istringstream str(port_field->getText());
            str >> port;
            if (!str) {
                std::unique_ptr<Screen> error_screen(new ErrorScreen("Bad port number"));
                knights_app.requestScreenChange(std::move(error_screen));
                return;
            }
        }
        initiateConnection(address, port);

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
        std::ostringstream str;
        str << si->port;
        port_field->setText(str.str());

        address_field->logic();
        port_field->logic();

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

void FindServerScreenImpl::initiateConnection(const std::string &address, int port)
{
    if (!allow_conn) return;
    
    UTF8String player_name = name_field ? UTF8String::fromLatin1(name_field->getText()) : UTF8String();

    if (player_name.empty()) {
        gotoErrorDialog("You must enter a player name");
    } else if (address.empty()) {
        gotoErrorDialog("You must enter an address to connect to");
    } else {
        knights_app.setPlayerName(player_name); // make sure the new name gets saved when we exit
        previous_address[internet?1:0] = address;
        previous_port[internet?1:0] = port;

        std::unique_ptr<Screen> connecting_screen(new ConnectingScreen(address, port, !internet, player_name));
        knights_app.requestScreenChange(std::move(connecting_screen));
    }
}


FindServerScreen::FindServerScreen(const std::string &t, bool inet)
    : title(t), internet(inet)
{
}

bool FindServerScreen::start(KnightsApp &ka, boost::shared_ptr<Coercri::Window> win, gcn::Gui &gui)
{
    pimpl.reset(new FindServerScreenImpl(ka, win, gui, title, internet));
    return true;
}

void FindServerScreen::update()
{
    if (pimpl) pimpl->doUpdate();
}

unsigned int FindServerScreen::getUpdateInterval()
{
    return 100;
}
