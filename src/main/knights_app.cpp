/*
 * knights_app.cpp
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

#include "misc.hpp"

#include "config_map.hpp"
#include "credits_screen.hpp"
#include "error_screen.hpp"
#include "file_cache.hpp"
#include "game_manager.hpp"
#include "gfx_manager.hpp"
#include "gfx_resizer_compose.hpp"
#include "gfx_resizer_nearest_nbr.hpp"
#include "gfx_resizer_scale2x.hpp"
#include "graphic.hpp"
#include "keyboard_controller.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
#include "knights_server.hpp"
#include "lua_exec.hpp"
#include "lua_func_wrapper.hpp"
#include "lua_load_from_rstream.hpp"
#include "lua_sandbox.hpp"
#include "load_font.hpp"
#include "loading_screen.hpp"
#include "my_ctype.hpp"
#include "my_exceptions.hpp"
#include "net_msgs.hpp"
#include "options.hpp"
#include "potion_renderer.hpp"
#include "rng.hpp"
#include "rstream.hpp"
#include "skull_renderer.hpp"
#include "sound_manager.hpp"
#include "title_screen.hpp"

// coercri
#include "core/coercri_error.hpp"
#include "enet/enet_network_driver.hpp"
#include "gcn/cg_font.hpp"
#include "gcn/cg_listener.hpp"
#include "gfx/freetype_ttf_loader.hpp"
#include "gfx/window_listener.hpp"
#include "network/network_connection.hpp"
#include "network/udp_socket.hpp"
#include "sdl/gfx/sdl_gfx_driver.hpp"
#include "sdl/sound/sdl_sound_driver.hpp"
#include "timer/generic_timer.hpp"

#ifdef WIN32
#include "dx11/gfx/dx11_gfx_driver.hpp"
#endif

// curl
#include <curl/curl.h>

// guichan
#include "guichan.hpp"
#include "lua.hpp"

// boost
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"
#include "boost/scoped_ptr.hpp"

#include <cstdlib>

#ifdef __LP64__
#include <stdint.h>
#define int_pointer_type intptr_t
#else
#define int_pointer_type unsigned long
#endif

#ifdef WIN32
#include <shlobj.h>
#include "SDL_syswm.h"
#endif

#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

/////////////////////////////////////////////////////////////
// WindowListener derived classes
/////////////////////////////////////////////////////////////

namespace {
    class WindowCloseListener : public Coercri::WindowListener {
    public:
        explicit WindowCloseListener(KnightsApp &k) : app(k) { }
        void onClose() { app.requestQuit(); }
    private:
        KnightsApp &app;
    };
}

/////////////////////////////////////////////////////////////
// Lua stuff
/////////////////////////////////////////////////////////////

namespace {
    typedef std::vector<boost::shared_ptr<Graphic> > GfxVector;

    int MakeGraphic(lua_State *lua)
    {
        GfxVector *gfx_vector = static_cast<GfxVector*>(lua_touserdata(lua, lua_upvalueindex(2)));
        const unsigned int old_size = gfx_vector->size();

        std::auto_ptr<Graphic> gfx(CreateGraphicFromLua(lua));

        if (!gfx->getFileInfo().isStandardFile()) {
            throw std::runtime_error("Error in client config, non-local filename '" + 
                gfx->getFileInfo().getPath().generic_string() 
                + "' used (try adding '+' at start of filename)");
        }

        boost::shared_ptr<Graphic> g(gfx.release());
        gfx_vector->push_back(g);

        void * lua_graphic = reinterpret_cast<void*>(old_size + 1);
        lua_pushlightuserdata(lua, lua_graphic);
        return 1;
    }

    void SetupLuaConfigFunctions(lua_State *lua, GfxVector *gfx_vector)
    {
        lua_pushlightuserdata(lua, gfx_vector);
        PushCClosure(lua, &MakeGraphic, 1);
        lua_setglobal(lua, "Graphic");
    }

    Graphic * PopGraphic(lua_State *lua, const GfxVector *gfx_vector)
    {
        void * lua_graphic = lua_touserdata(lua, -1);
        lua_pop(lua, 1);
        const unsigned int idx = reinterpret_cast<int_pointer_type>(lua_graphic);
        if (idx > 0 && idx <= gfx_vector->size()) return (*gfx_vector)[idx-1].get();
        else return 0;
    }

    Colour PopColour(lua_State *lua)
    {
        // colours are represented as {r,g,b} triple e.g. {255,0,0} is bright red
        lua_pushinteger(lua, 1);
        lua_gettable(lua, -2);
        const int r = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        lua_pushinteger(lua, 2);
        lua_gettable(lua, -2);
        const int g = lua_tointeger(lua, -1);
        lua_pop(lua, 1);

        lua_pushinteger(lua, 3);
        lua_gettable(lua, -2);
        const int b = lua_tointeger(lua, -1);
        lua_pop(lua, 2);  // gets rid of 'b' and the table itself

        return Colour(r, g, b);
    }
}


/////////////////////////////////////////////////////////////
// Definition of KnightsAppImpl
/////////////////////////////////////////////////////////////

class KnightsAppImpl {
public:
    boost::shared_ptr<Coercri::GfxDriver> gfx_driver;
    boost::shared_ptr<Coercri::SoundDriver> sound_driver;
    boost::shared_ptr<Coercri::Timer> timer;
    boost::shared_ptr<Coercri::TTFLoader> ttf_loader;
    boost::shared_ptr<Coercri::Window> window;
    boost::shared_ptr<WindowCloseListener> wcl;

    boost::shared_ptr<gcn::Gui> gui;
    boost::shared_ptr<Coercri::CGListener> cg_listener;

    // we do not want these shared, we want to be sure there is only one copy of each:
    auto_ptr<Screen> current_screen, requested_screen;
    
    unsigned int last_time;  // last time update() was called
    unsigned int update_interval;
    
    bool running;

    boost::shared_ptr<GfxManager> gfx_manager;
    boost::shared_ptr<SoundManager> sound_manager;
    FileCache file_cache;
    boost::shared_ptr<Controller> left_controller, right_controller, net_game_controller;

    boost::shared_ptr<Coercri::Font> font;
    boost::scoped_ptr<gcn::Font> gcn_font;

    boost::scoped_ptr<Options> options;
    boost::filesystem::path options_filename;
    bool player_name_changed;  // has options->player_name been changed

    ConfigMap config_map;
    GfxVector config_gfx;  // Gfx loaded from the client config file.

    Graphic *winner_image;
    Graphic *loser_image;
    Graphic *menu_gfx_centre, *menu_gfx_empty, *menu_gfx_highlight;
    Graphic *speech_bubble;
    std::auto_ptr<PotionRenderer> potion_renderer;
    std::auto_ptr<SkullRenderer> skull_renderer;


    // network driver
    boost::scoped_ptr<Coercri::NetworkDriver> net_driver;

    
    // server object.
    string server_config_filename;
    boost::scoped_ptr<KnightsServer> knights_server;

    // incoming network connections (to the knights_server above)
    struct IncomingConn {
        ServerConnection * server_conn;
        boost::shared_ptr<Coercri::NetworkConnection> remote;
    };
    std::vector<IncomingConn> incoming_conns;

    // outgoing network connections
    struct OutgoingConn {
        boost::shared_ptr<KnightsClient> knights_client;
        boost::shared_ptr<Coercri::NetworkConnection> remote;
    };
    std::vector<OutgoingConn> outgoing_conns;

    // local "loopback" connections
    struct LocalConn {
        boost::shared_ptr<KnightsClient> knights_client;
        ServerConnection * server_conn;
    };
    std::vector<LocalConn> local_conns;

    // broadcast socket
    boost::shared_ptr<Coercri::UDPSocket> broadcast_socket;
    unsigned int broadcast_last_time;
    int server_port;
    
    // game manager
    boost::scoped_ptr<GameManager> game_manager;

    // autostart mode
    bool autostart;

    // are we using DX11 or SDL
    bool using_dx11;
    
    // functions
    KnightsAppImpl() : last_time(0), update_interval(0), running(true), player_name_changed(false) { }

    void saveOptions();
    
    void popPotionSetup(lua_State*);
    void popSkullSetup(lua_State*);

    bool processIncomingNetMsgs();
    bool processOutgoingNetMsgs();
    bool processLocalNetMsgs();
    bool processBroadcastMsgs();
};

/////////////////////////////////////////////////////
// Constructor (One-Time Initialization)
/////////////////////////////////////////////////////

KnightsApp::KnightsApp(DisplayType display_type, const string &resource_dir, const string &config_filename,
                       bool autostart)
    : pimpl(new KnightsAppImpl)
{
    const char * game_name = "Knights";

    pimpl->server_config_filename = config_filename;
    pimpl->autostart = autostart;
    
    // initialize RNG
    g_rng.initialize();
    g_rng.setSeed(static_cast<unsigned int>(std::time(0)));

    // initialize resource lib
    RStream::Initialize(resource_dir);

    // read the client config
    {
        // we use a simplified lua context in which all light userdatas represent
        // a (1-based) index into the config_gfx vector
        // (We use 1-based indices so that if lua_touserdata is called on a nil value,
        // which returns 0, we end up with an invalid index.)
        boost::shared_ptr<lua_State> lua_sh_ptr = MakeLuaSandbox();
        lua_State * const lua = lua_sh_ptr.get();

        SetupLuaConfigFunctions(lua, &pimpl->config_gfx);
        LuaExecRStream(lua, "client/client_config.lua", 0, 0, 
            false);   // look in root dir only

        lua_getglobal(lua, "MISC_CONFIG");
        PopConfigMap(lua, pimpl->config_map);
 
        lua_getglobal(lua, "WINNER_IMAGE");
        pimpl->winner_image = PopGraphic(lua, &pimpl->config_gfx);
        lua_getglobal(lua, "LOSER_IMAGE");
        pimpl->loser_image = PopGraphic(lua, &pimpl->config_gfx);
        lua_getglobal(lua, "MENU_CENTRE");
        pimpl->menu_gfx_centre = PopGraphic(lua, &pimpl->config_gfx);
        lua_getglobal(lua, "MENU_EMPTY");
        pimpl->menu_gfx_empty = PopGraphic(lua, &pimpl->config_gfx);
        lua_getglobal(lua, "MENU_HIGHLIGHT");
        pimpl->menu_gfx_highlight = PopGraphic(lua, &pimpl->config_gfx);
        lua_getglobal(lua, "POTION_SETUP");
        pimpl->popPotionSetup(lua);
        lua_getglobal(lua, "SKULL_SETUP");
        pimpl->popSkullSetup(lua);
        lua_getglobal(lua, "SPEECH_BUBBLE");
        pimpl->speech_bubble = PopGraphic(lua, &pimpl->config_gfx);
    }

    // initialize game options
    pimpl->options.reset(new Options);
#ifdef WIN32
    // options stored in "app data" directory
    wchar_t path[MAX_PATH];
    if(SUCCEEDED(SHGetFolderPathW(NULL, 
                                  CSIDL_APPDATA,
                                  NULL,
                                  0,
                                  path))) {
        pimpl->options_filename = path;
        pimpl->options_filename += "/knights_config.txt";
    }
#else
    // options stored in home directory (assume getenv("HOME") will work)
    pimpl->options_filename = std::getenv("HOME");
    pimpl->options_filename /= ".knights_config";
#endif
    if (!pimpl->options_filename.empty()) {
        boost::filesystem::ifstream str(pimpl->options_filename);
        *pimpl->options = LoadOptions(str);
    }
    
    // Set up Coercri
    // -- Use DX11 if available, but fall back to SDL if that fails.
    try {

#ifdef WIN32
        unsigned int flags = 0;

#ifndef NDEBUG
        flags = D3D11_CREATE_DEVICE_DEBUG;
#endif
        pimpl->gfx_driver.reset(new Coercri::DX11GfxDriver(D3D_DRIVER_TYPE_HARDWARE, 
                                                           flags,
                                                           D3D_FEATURE_LEVEL_9_1));
        pimpl->using_dx11 = true;

#else // WIN32
        throw Coercri::CoercriError("This platform does not support DirectX");
#endif // WIN32

    } catch (Coercri::CoercriError &) {
        pimpl->gfx_driver.reset(new Coercri::SDLGfxDriver);
        pimpl->using_dx11 = false;
    }

#ifndef DISABLE_SOUND
    try {
        pimpl->sound_driver.reset(new Coercri::SDLSoundDriver(pimpl->config_map.getInt("sound_volume") / 100.0f));
    } catch (std::exception &e) {
        // Print warning message to cout, and continue without a sound_driver
        std::cout << "Problem initializing audio: " << e.what() << std::endl;
    }
#endif
    
    pimpl->timer.reset(new Coercri::GenericTimer);
    pimpl->ttf_loader.reset(new Coercri::FreetypeTTFLoader);

    // use the enet network driver
    pimpl->net_driver.reset(new Coercri::EnetNetworkDriver(32, 1, true));
    pimpl->net_driver->enableServer(false);  // start off disabled.

    // initialize curl. tell it not to init winsock since EnetNetworkDriver will have done that already.
    curl_global_init(CURL_GLOBAL_NOTHING);
    
    // Set the Windows resource number for the window icon
    pimpl->gfx_driver->setWindowsIcon(1);
    
    // Open the game window.
    bool fullscreen = pimpl->options->fullscreen;
    if (display_type == DT_WINDOWED) fullscreen = false;
    if (display_type == DT_FULLSCREEN) fullscreen = true;
    int width, height;
    if (fullscreen) {
        getDesktopResolution(width, height);
    } else {
        getWindowedModeSize(width, height);
    }

    try {
        pimpl->window = pimpl->gfx_driver->createWindow(width, height, true, fullscreen, game_name);
    } catch (Coercri::CoercriError &) {
        // If creation in fullscreen mode fails then try again in windowed mode (Trac #118)
        // (If it wasn't fullscreen mode then re-throw the exception)
        if (fullscreen) {
            // shrink the window a little bit, so it doesn't fill the entire screen
            width = std::max(width-200, 400);
            height = std::max(height-200, 400);
            pimpl->window = pimpl->gfx_driver->createWindow(width, height, true, false, game_name);
        } else {
            throw;
        }
    }
    
    // Get the font names.
    // NOTE: we always try to load TTF before bitmap fonts, irrespective of the order they are in the file.
    vector<string> ttf_font_names, bitmap_font_names;
    {
        RStream str("client/fonts.txt");

        while (str) {
            string x;
            getline(str, x);
        
            // Left trim
            x.erase(x.begin(), find_if(x.begin(), x.end(), not1(ptr_fun<char,bool>(IsSpace))));
            // Right trim
            x.erase(find_if(x.rbegin(), x.rend(), not1(ptr_fun<char,bool>(IsSpace))).base(), x.end());
            
            if (x.empty()) continue;
        
            if (x[0] == '+') bitmap_font_names.push_back(x.substr(1));
            else if (x[0] != '#') ttf_font_names.push_back(x);
        }
    }
    
    // Font for the gui
    pimpl->font = LoadFont(*pimpl->ttf_loader,
                           ttf_font_names, 
                           bitmap_font_names, 
                           pimpl->config_map.getInt("font_size"));

    // Setup gfx & sound managers
    pimpl->gfx_manager.reset(
        new GfxManager(pimpl->gfx_driver, pimpl->ttf_loader,
                       ttf_font_names, bitmap_font_names,
                       static_cast<unsigned char>(pimpl->config_map.getInt("invisalpha")),
                       pimpl->file_cache));
    setupGfxResizer();
    pimpl->sound_manager.reset(new SoundManager(pimpl->sound_driver, pimpl->file_cache));

    // Load all the graphics at this point
    // These are added as "permanent" so that we don't have to keep reloading them after a reset.
    for (std::vector<boost::shared_ptr<Graphic> >::iterator it = pimpl->config_gfx.begin(); it != pimpl->config_gfx.end(); ++it) {
        pimpl->gfx_manager->loadGraphic(**it, true);
    }
    
    // Setup Controllers
    setupControllers();

    // Set up Guichan and CGListener
    // NOTE: No need to set Input and Graphics for guichan since this is handled by CGListener.
    pimpl->gui.reset(new gcn::Gui);
    pimpl->cg_listener.reset(new Coercri::CGListener(pimpl->window, pimpl->gui, pimpl->timer));
    pimpl->window->addWindowListener(pimpl->cg_listener.get());

    // Add the WindowCloseListener
    pimpl->wcl.reset(new WindowCloseListener(*this));
    pimpl->window->addWindowListener(pimpl->wcl.get());
    
    // Set guichan's global widget font
    pimpl->gcn_font.reset(new Coercri::CGFont(pimpl->font));
    gcn::Widget::setGlobalFont(pimpl->gcn_font.get());
}

void KnightsApp::getWindowedModeSize(int &width, int &height)
{
    getDesktopResolution(width, height);
    width = std::min(width, std::max(100, pimpl->options->window_width));
    height = std::min(height, std::max(100, pimpl->options->window_height));
}


//////////////////////////////////////////////
// Screen handling
//////////////////////////////////////////////

void KnightsApp::requestScreenChange(auto_ptr<Screen> screen)
{
    pimpl->requested_screen = screen;
}

void KnightsApp::executeScreenChange()
{
    if (pimpl->requested_screen.get() != pimpl->current_screen.get() && pimpl->requested_screen.get()) {
        // Completely destroy the gui, then re-create it. This seems to be necessary
        // to avoid certain dangling reference problems (Trac #18).
        pimpl->window->rmWindowListener(pimpl->cg_listener.get());
        pimpl->cg_listener.reset();
        pimpl->gui.reset(new gcn::Gui);
        pimpl->cg_listener.reset(new Coercri::CGListener(pimpl->window, pimpl->gui, pimpl->timer));
        pimpl->window->addWindowListener(pimpl->cg_listener.get());
        pimpl->cg_listener->disableGui();
        
        // Swap screens
        pimpl->current_screen = pimpl->requested_screen;   // clears requested_screen

        // Initialize the new screen, and bring up the gui if required
        const bool requires_gui = pimpl->current_screen->start(*this, pimpl->window, *pimpl->gui);
        if (requires_gui) pimpl->cg_listener->enableGui();
        
        // Reset timer.
        pimpl->last_time = pimpl->timer->getMsec();
        pimpl->update_interval = pimpl->current_screen->getUpdateInterval();

        // Ensure the screen gets repainted
        pimpl->window->invalidateAll();
    }
}

void KnightsApp::requestQuit()
{
    pimpl->running = false;
}

void KnightsApp::popWindowToFront()
{
    pimpl->window->popToFront();
}

void KnightsApp::repeatLastMouseInput()
{
    pimpl->cg_listener->repeatLastMouseInput();
}

const std::string & KnightsApp::getPlayerName() const
{
    return pimpl->options->player_name;
}

void KnightsApp::setPlayerName(const std::string &name) 
{
    if (name != pimpl->options->player_name) {
        pimpl->options->player_name = name;
        pimpl->player_name_changed = true;
    }
}

bool KnightsApp::usingDX11() const
{
    return pimpl->using_dx11;
}

//////////////////////////////////////////////
// Game state handling
//////////////////////////////////////////////

void KnightsApp::resetAll()
{
    // Clean up outgoing connections
    for (std::vector<KnightsAppImpl::OutgoingConn>::iterator it = pimpl->outgoing_conns.begin();
    it != pimpl->outgoing_conns.end(); ++it) {
        ASSERT(it->knights_client && "resetAll: outgoing");
        it->knights_client->setClientCallbacks(0);
        it->knights_client->setKnightsCallbacks(0);
        it->knights_client->connectionClosed();
        it->remote->close();
    }
    pimpl->outgoing_conns.clear();

    // Clean up incoming connections
    for (std::vector<KnightsAppImpl::IncomingConn>::iterator it = pimpl->incoming_conns.begin();
    it != pimpl->incoming_conns.end(); ++it) {
        ASSERT(pimpl->knights_server && "resetAll: incoming");  // otherwise there would not be any incoming_conns!
        pimpl->knights_server->connectionClosed(*it->server_conn);
        it->remote->close();
    }
    pimpl->incoming_conns.clear();

    // Clean up local connections
    pimpl->local_conns.clear();

    // Shut down the server
    pimpl->knights_server.reset();
    pimpl->net_driver->enableServer(false);

    // Shut down the GameManager (important to do this BEFORE accessing gfxmanager/soundmanager)
    destroyGameManager();

    // Wipe out all loaded graphics / sounds.
    unloadGraphicsAndSounds();

    // Stop responding to broadcasts
    pimpl->broadcast_socket.reset();
    pimpl->broadcast_last_time = 0;
    pimpl->server_port = 0;
}    

void KnightsApp::unloadGraphicsAndSounds()
{
    pimpl->gfx_manager->deleteAllGraphics();
    pimpl->sound_manager->clear();
}

//////////////////////////////////////////////
// Other methods
//////////////////////////////////////////////

void KnightsApp::setupGfxResizer()
{
    boost::shared_ptr<GfxResizer> gfx_resizer_nearest_nbr(new GfxResizerNearestNbr);
    boost::shared_ptr<GfxResizer> gfx_resizer_scale2x(new GfxResizerScale2x);
    boost::shared_ptr<GfxResizer> gfx_resizer_comp(new GfxResizerCompose(gfx_resizer_nearest_nbr,
        pimpl->options->use_scale2x ? gfx_resizer_scale2x : boost::shared_ptr<GfxResizer>(),
        !pimpl->options->allow_non_integer_scaling));
    pimpl->gfx_manager->setGfxResizer(gfx_resizer_comp);
}

void KnightsApp::setupControllers()
{
    pimpl->left_controller.reset(new KeyboardController(false,
                                                        pimpl->options->ctrls[0][0],
                                                        pimpl->options->ctrls[0][3],
                                                        pimpl->options->ctrls[0][1],
                                                        pimpl->options->ctrls[0][2],
                                                        pimpl->options->ctrls[0][4],
                                                        pimpl->options->ctrls[0][5],
                                                        pimpl->window));
    pimpl->right_controller.reset(new KeyboardController(false,
                                                         pimpl->options->ctrls[1][0],
                                                         pimpl->options->ctrls[1][3],
                                                         pimpl->options->ctrls[1][1],
                                                         pimpl->options->ctrls[1][2],
                                                         pimpl->options->ctrls[1][4],
                                                         pimpl->options->ctrls[1][5],
                                                         pimpl->window));
    pimpl->net_game_controller.reset(new KeyboardController(pimpl->options->new_control_system,
                                                            pimpl->options->ctrls[2][0],
                                                            pimpl->options->ctrls[2][3],
                                                            pimpl->options->ctrls[2][1],
                                                            pimpl->options->ctrls[2][2],
                                                            pimpl->options->ctrls[2][4],
                                                            pimpl->options->ctrls[2][5],
                                                            pimpl->window));
}    

boost::shared_ptr<Coercri::Font> KnightsApp::getFont() const
{
    return pimpl->font;
}

const Controller & KnightsApp::getLeftController() const
{
    return *pimpl->left_controller;
}

const Controller & KnightsApp::getRightController() const
{
    return *pimpl->right_controller;
}

const Controller & KnightsApp::getNetGameController() const
{
    return *pimpl->net_game_controller;
}

GfxManager & KnightsApp::getGfxManager() const
{
    return *pimpl->gfx_manager;
}

SoundManager & KnightsApp::getSoundManager() const
{
    return *pimpl->sound_manager;
}

FileCache & KnightsApp::getFileCache() const
{
    return pimpl->file_cache;
}

void KnightsApp::getDesktopResolution(int &w, int &h) const
{
    Coercri::GfxDriver::DisplayMode mode = pimpl->gfx_driver->getDesktopMode();
    w = mode.width;
    h = mode.height;
}

const Graphic * KnightsApp::getLoserImage() const
{
    return pimpl->loser_image;
}

const Graphic * KnightsApp::getWinnerImage() const
{
    return pimpl->winner_image;
}

const Graphic * KnightsApp::getMenuGfxCentre() const
{
    return pimpl->menu_gfx_centre;
}

const Graphic * KnightsApp::getMenuGfxEmpty() const
{
    return pimpl->menu_gfx_empty;
}

const Graphic * KnightsApp::getMenuGfxHighlight() const
{
    return pimpl->menu_gfx_highlight;
}

const Graphic * KnightsApp::getSpeechBubble() const
{
    return pimpl->speech_bubble;
}

const PotionRenderer * KnightsApp::getPotionRenderer() const
{
    return pimpl->potion_renderer.get();
}

const SkullRenderer * KnightsApp::getSkullRenderer() const
{
    return pimpl->skull_renderer.get();
}

const ConfigMap & KnightsApp::getConfigMap() const
{
    return pimpl->config_map;
}

Coercri::Timer & KnightsApp::getTimer() const
{
    return *pimpl->timer;
}

Coercri::NetworkDriver & KnightsApp::getNetworkDriver() const
{
    return *pimpl->net_driver;
}

const Options & KnightsApp::getOptions() const
{
    return *pimpl->options;
}

void KnightsApp::setAndSaveOptions(const Options &opts)
{
    *pimpl->options = opts;
    pimpl->saveOptions();
    setupControllers();
    setupGfxResizer();
}

void KnightsAppImpl::saveOptions()
{
    if (!options_filename.empty()) {
        ofstream str(options_filename.c_str(), std::ios_base::out | std::ios_base::trunc);
        SaveOptions(*options, str);
        player_name_changed = false;
    }
}

void KnightsAppImpl::popPotionSetup(lua_State *lua)
{
    potion_renderer.reset(new PotionRenderer(config_map));

    lua_pushstring(lua, "colours");
    lua_gettable(lua, -2);  // 'colours' list is now at top of stack, with potion setup table 1 below that.

    const int num_colours = lua_rawlen(lua, -1);
    
    for (int i = 0; i < num_colours; ++i) {
        lua_pushinteger(lua, i+1);
        lua_gettable(lua, -2);
        Colour col = PopColour(lua);
        potion_renderer->addColour(col);
    }

    lua_pop(lua, 1);  // get rid of 'colours' list. original potion setup table is now top of stack

    lua_pushstring(lua, "graphics");
    lua_gettable(lua, -2);  // 'graphics' table is top of stack; potion setup table is below

    const int num_graphics = lua_rawlen(lua, -1);

    for (int i = 0; i < num_graphics; ++i) {
        lua_pushinteger(lua, i+1);
        lua_gettable(lua, -2);
        Graphic *g = PopGraphic(lua, &config_gfx);
        potion_renderer->addGraphic(g);
    }

    lua_pop(lua, 2);  // get rid of 'graphics' table AND potion setup table.
}

void KnightsAppImpl::popSkullSetup(lua_State *lua)
{
    skull_renderer.reset(new SkullRenderer);

    lua_pushstring(lua, "columns");
    lua_gettable(lua, -2);  // 'columns' at top of stack, orig table below

    const int num_columns = lua_rawlen(lua, -1);

    for (int i = 0; i < num_columns; ++i) {
        lua_pushinteger(lua, i+1);
        lua_gettable(lua, -2);
        const int col = lua_tointeger(lua, -1);
        lua_pop(lua, 1);
        skull_renderer->addColumn(col);
    }

    lua_pop(lua, 1);  // get rid of 'columns', orig table now top of stack

    lua_pushstring(lua, "graphics");
    lua_gettable(lua, -2);  // gfx table now top of stack, orig table below

    const int num_graphics = lua_rawlen(lua, -1);

    for (int i = 0; i < num_graphics; ++i) {
        lua_pushinteger(lua, i+1);
        lua_gettable(lua, -2);
        Graphic *g = PopGraphic(lua, &config_gfx);
        skull_renderer->addGraphic(g);
    }

    lua_pop(lua, 1);  // get rid of 'graphics', orig table now top of stack

    lua_pushstring(lua, "rows");
    lua_gettable(lua, -2);   // rows table top of stack, orig table below

    const int num_rows = lua_rawlen(lua, -1);

    for (int i = 0; i < num_rows; ++i) {
        lua_pushinteger(lua, i+1);
        lua_gettable(lua, -2);
        const int row = lua_tointeger(lua, -1);
        lua_pop(lua, 1);
        skull_renderer->addRow(row);
    }
}

//////////////////////////////////////////////
// Main Routine (runKnights)
//////////////////////////////////////////////

void KnightsApp::runKnights()
{
    auto_ptr<Screen> initial_screen;

    if (pimpl->autostart) {
        initial_screen.reset(new LoadingScreen(-1, "", true, true, false, true));  // single player on, menu-strict on, tutorial off, autostart on
    } else {
        // Go to title screen.
        if (pimpl->options->first_time) {
            initial_screen.reset(new CreditsScreen("client/first_time_message.txt", 60));
            pimpl->saveOptions();  // make sure they don't get the first time screen again
        } else {
            initial_screen.reset(new TitleScreen);
        }
    }

    requestScreenChange(initial_screen);
    executeScreenChange();

    // Error Handling system.
    string error;
    int num_errors = 0;
    
    // Main Loop
    while (pimpl->running) {

        bool did_something = false;
        
        if (!error.empty() && num_errors > 100) {
            // Too many errors
            // Abort game.
            throw UnexpectedError("FATAL: " + error);
        }
        
        try {
            if (!error.empty()) {
                // Error detected: try to go to ErrorScreen.
                // (If going to ErrorScreen itself throws an error, then num_errors will keep increasing
                // and eventually we will abort, see above.)
                auto_ptr<Screen> screen;
                screen.reset(new ErrorScreen(error));
                requestScreenChange(screen);
                executeScreenChange();
                error.clear();
                ++num_errors;
                did_something = true;
            }

            // Handle screen changes if necessary
            executeScreenChange();

            // Before running network events, make sure outgoing messages have been sent
            did_something = pimpl->processOutgoingNetMsgs() || did_something;
            
            // Empty out all event queues
            while (pimpl->gfx_driver->pollEvents()) did_something = true;
            while (pimpl->cg_listener->processInput()) did_something = true;
            while (pimpl->net_driver && pimpl->net_driver->doEvents()) did_something = true;

            // Make sure any incoming net messages (or dropped connections) get processed at this point.
            did_something = pimpl->processIncomingNetMsgs() || did_something;

            // Also: make sure local net messages are routed properly
            did_something = pimpl->processLocalNetMsgs() || did_something;

            // Also: do the broadcast msgs
            did_something = pimpl->processBroadcastMsgs() || did_something;
            
            // Do updates
            if (pimpl->update_interval > 0) {
                const unsigned int time_now = pimpl->timer->getMsec();
                unsigned int time_since_last_update = time_now - pimpl->last_time;

                // if it's time for an update then do one.
                if (time_since_last_update >= pimpl->update_interval) {
                    pimpl->last_time += pimpl->update_interval;
                    time_since_last_update -= pimpl->update_interval;
                    if (time_since_last_update > pimpl->update_interval) {
                        pimpl->last_time = time_now - pimpl->update_interval;  // don't let it get too far behind.
                    }
                    pimpl->current_screen->update();
                    did_something = true;
                }
            }

            // Draw the screen if necessary
            const bool screen_change_imminent = pimpl->requested_screen.get() != pimpl->current_screen.get() 
                   && pimpl->requested_screen.get();
            if (pimpl->window->needsRepaint() && !screen_change_imminent) {
                std::auto_ptr<Coercri::GfxContext> gc = pimpl->window->createGfxContext();
                gc->clearClipRectangle();
                gc->clearScreen(Coercri::Color(0,0,0));
                pimpl->cg_listener->draw(*gc);
                pimpl->current_screen->draw(*gc);
                pimpl->window->cancelInvalidRegion();
                did_something = true;
            }

            // Sleep if necessary (so that we don't consume 100% CPU).
            if (!did_something) {
                unsigned int delay = 0u;
                if (pimpl->update_interval == 0) {
                    delay = 20u;
                } else {
                    const unsigned int time_since_last_update = pimpl->timer->getMsec() - pimpl->last_time;
                    if (time_since_last_update < pimpl->update_interval) {
                        delay = pimpl->update_interval - time_since_last_update;
                    }
                }
                if (delay > 20u) delay = 20u;
                if (delay > 0u) pimpl->timer->sleepMsec(delay);
                num_errors = 0;  // if we can get to a sleep w/o error, then assume things
                                 // are going well & can reset num_errors.
            }

        } catch (LuaError&) {
            // These are big and better displayed on stdout than in a guichan dialog box:
            throw;
        } catch (std::exception &e) {
            error = string("ERROR: ") + e.what();
        } catch (gcn::Exception &e) {
            error = string("Guichan Error: ") + e.getMessage();
        } catch (...) {
            error = "Unknown Error";
        }
    }

    bool save_options_required = false;

#ifdef WIN32
    if (pimpl->options) {
        // Get our HWND from SDL.
        SDL_SysWMinfo inf = {0};
        SDL_VERSION(&inf.version);
        if (SDL_GetWMInfo(&inf)) {

            // If the window is maximized then don't try to save the size
            // as it then looks weird when we next start up the program.
            // Ditto if we are in full screen mode.
            if (!IsZoomed(inf.window) && !pimpl->window->isFullScreen()) {
            
                // Get current window size
                int width = 0, height = 0;
                pimpl->window->getSize(width, height);

                // If it differs from the saved window size, then update the
                // saved window size and re-save the options file.
                if (width != pimpl->options->window_width || height != pimpl->options->window_height) {
                    pimpl->options->window_width = width;
                    pimpl->options->window_height = height;
                    save_options_required = true;
                }
            }
        }
    }
#endif

    // If player name has been changed then we need to save options
    if (pimpl->player_name_changed) save_options_required = true;
    
    if (save_options_required) {
        pimpl->saveOptions();
    }
    
    // Delete things -- this is to make sure that destructors run in the order we want them to.
    pimpl->current_screen.reset();
    pimpl->requested_screen.reset();
    pimpl->left_controller.reset();
    pimpl->right_controller.reset();

    pimpl->sound_driver.reset();
    pimpl->window.reset();
    pimpl->wcl.reset();
    pimpl->cg_listener.reset();
    pimpl->gfx_driver.reset();

    // Call resetAll -- this closes network connections among other things.
    resetAll();

    // delete GfxManager, this has the last reference to the Coercri::GfxDriver.
    pimpl->gfx_manager.reset();
    
    // Shut down curl.
    curl_global_cleanup();
    
    // Wait up to ten seconds for network connections to be cleaned up.
    const int WAIT_SECONDS = 10;
    for (int i = 0; i < WAIT_SECONDS*10; ++i) {
        if (!pimpl->net_driver->outstandingConnections()) break;
        while (pimpl->net_driver->doEvents()) ;
        pimpl->timer->sleepMsec(100);
    }
}


//////////////////////////////////////////////
// Network Game Handling
//////////////////////////////////////////////

boost::shared_ptr<KnightsClient> KnightsApp::openRemoteConnection(const std::string &address, int port)
{
    KnightsAppImpl::OutgoingConn out;
    out.knights_client.reset(new KnightsClient);
    out.remote = pimpl->net_driver->openConnection(address, port);
    pimpl->outgoing_conns.push_back(out);
    return out.knights_client;
}

boost::shared_ptr<KnightsClient> KnightsApp::openLocalConnection()
{
    if (!pimpl->knights_server) throw UnexpectedError("server must be created before calling openLocalConnection");
    KnightsAppImpl::LocalConn conn;
    conn.knights_client.reset(new KnightsClient);
    conn.server_conn = &pimpl->knights_server->newClientConnection();
    pimpl->local_conns.push_back(conn);
    return conn.knights_client;
}

void KnightsApp::closeConnection(KnightsClient *knights_client)
{
    // see if it's an outgoing (network) connection
    std::vector<KnightsAppImpl::OutgoingConn>::iterator it;
    for (it = pimpl->outgoing_conns.begin(); it != pimpl->outgoing_conns.end(); ++it) {
        if (it->knights_client.get() == knights_client) break;
    }
    if (it != pimpl->outgoing_conns.end()) {
        knights_client->connectionClosed();
        it->remote->close();
        pimpl->outgoing_conns.erase(it);
    } else {

        // see if it's a local connection
        std::vector<KnightsAppImpl::LocalConn>::iterator it;
        for (it = pimpl->local_conns.begin(); it != pimpl->local_conns.end(); ++it) {
            if (it->knights_client.get() == knights_client) break;
        }
        if (it != pimpl->local_conns.end()) {
            knights_client->connectionClosed();
            pimpl->knights_server->connectionClosed(*it->server_conn);
            pimpl->local_conns.erase(it);
        }
    }
}

const std::string & KnightsApp::getKnightsConfigFilename() const
{
    return pimpl->server_config_filename;
}

KnightsServer * KnightsApp::createServer(int port)
{
    if (pimpl->knights_server) throw UnexpectedError("KnightsServer created twice");
    pimpl->knights_server.reset(new KnightsServer(pimpl->timer, false, "", "", ""));
    pimpl->net_driver->setServerPort(port);
    pimpl->net_driver->enableServer(true);
    return pimpl->knights_server.get();
}

KnightsServer * KnightsApp::createLocalServer()
{
    if (pimpl->knights_server) throw UnexpectedError("KnightsServer created twice");
    pimpl->knights_server.reset(new KnightsServer(pimpl->timer, true, "", "", ""));
    return pimpl->knights_server.get();
}

bool KnightsAppImpl::processIncomingNetMsgs()
{
    // Check for any incoming network data and route it to the
    // appropriate KnightsClient or KnightsServer object

    bool did_something = false;
    std::vector<unsigned char> net_msg;

    // Do "outgoing" connections first
    
    for (int i = 0; i < outgoing_conns.size(); ) {
        OutgoingConn &out = outgoing_conns[i];
        ASSERT(out.knights_client);

        // see if there is any data, if so, route it to the KnightsClient
        out.remote->receive(net_msg);
        if (!net_msg.empty()) {
            did_something = true;
            out.knights_client->receiveInputData(net_msg);
        }
        
        // if connection has dropped, then remove it from the list
        const Coercri::NetworkConnection::State state = out.remote->getState();
        if (state == Coercri::NetworkConnection::CLOSED || state == Coercri::NetworkConnection::FAILED) {
            if (state == Coercri::NetworkConnection::CLOSED) {
                out.knights_client->connectionClosed();
            } else {
                out.knights_client->connectionFailed();
            }
            outgoing_conns.erase(outgoing_conns.begin() + i);
            did_something = true;
        } else {
            ++i;
        }
    }

    // Now do "incoming" connections
    
    for (int i = 0; i < incoming_conns.size(); /* incremented below */) {
        ASSERT(knights_server && "processIncomingNetMsgs: loop over incoming connections");
        IncomingConn &in = incoming_conns[i];

        in.remote->receive(net_msg);
        if (!net_msg.empty()) {
            did_something = true;
            knights_server->receiveInputData(*in.server_conn, net_msg);
        }
        
        const Coercri::NetworkConnection::State state = in.remote->getState();
        ASSERT(state != Coercri::NetworkConnection::FAILED); // incoming connections can't fail...
        if (state == Coercri::NetworkConnection::CLOSED) {
            // connection lost: remove it from the list, and inform the server
            knights_server->connectionClosed(*in.server_conn);
            incoming_conns.erase(incoming_conns.begin() + i);
            did_something = true;
        } else {
            ++i;
        }
    }

    // Listen for new incoming connections.
    Coercri::NetworkDriver::Connections new_conns = net_driver->pollIncomingConnections();
    for (Coercri::NetworkDriver::Connections::const_iterator it = new_conns.begin(); it != new_conns.end(); ++it) {
        ASSERT(knights_server && "processIncomingNetMsgs: listen for new incoming connections");
        IncomingConn in;
        in.server_conn = &knights_server->newClientConnection();
        in.remote = *it;
        incoming_conns.push_back(in);
        did_something = true;
    }

    return did_something;
}

bool KnightsAppImpl::processOutgoingNetMsgs()
{
    // Pick up any outgoing network data and route it to the
    // appropriate NetworkConnection

    // NOTE: we don't bother to check for dropped connections here,
    // instead that is done in processIncomingNetMsgs.

    bool did_something = false;
    std::vector<unsigned char> net_msg;
    
    for (std::vector<OutgoingConn>::iterator it = outgoing_conns.begin(); it != outgoing_conns.end(); ++it) {
        ASSERT(it->knights_client && "processOutgoingNetMsgs");
        it->knights_client->getOutputData(net_msg);
        if (!net_msg.empty()) {
            did_something = true;
            it->remote->send(net_msg);
        }
    }

    // tell the server what the ping times are
    for (std::vector<IncomingConn>::iterator it = incoming_conns.begin(); it != incoming_conns.end(); ++it) {
        knights_server->setPingTime(*it->server_conn, it->remote->getPingTime());
    }

    for (std::vector<IncomingConn>::iterator it = incoming_conns.begin(); it != incoming_conns.end(); ++it) {
        ASSERT(knights_server && "processOutgoingNetMsgs");
        knights_server->getOutputData(*it->server_conn, net_msg);
        if (!net_msg.empty()) {
            did_something = true;
            it->remote->send(net_msg);
        }
    }

    return did_something;
}

bool KnightsAppImpl::processLocalNetMsgs()
{
    bool did_something = false;
    std::vector<unsigned char> net_msg;
    
    for (std::vector<LocalConn>::iterator it = local_conns.begin(); it != local_conns.end(); ++it) {
        ASSERT(knights_server && "processLocalNetMsgs");
        ASSERT(it->knights_client && "processLocalNetMsgs");

        it->knights_client->getOutputData(net_msg);
#ifdef DEBUG_NET_MSGS
        if (!net_msg.empty()) {
            OutputDebugString("client: ");
            for (int i = 0; i < net_msg.size(); ++i) {
                char buf[256] = {0};
                sprintf(buf, "%d ", int (net_msg[i]));
                OutputDebugString(buf);
            }
            OutputDebugString("\n");
        }
#endif
        if (!net_msg.empty()) did_something = true;
        knights_server->receiveInputData(*it->server_conn, net_msg);

        knights_server->getOutputData(*it->server_conn, net_msg);
#ifdef DEBUG_NET_MSGS
        if (!net_msg.empty()) {
            OutputDebugString("server: ");
            for (int i = 0; i < net_msg.size(); ++i) {
                char buf[256] = {0};
                sprintf(buf, "%d ", int (net_msg[i]));
                OutputDebugString(buf);
            }
            OutputDebugString("\n");
        }
#endif
        if (!net_msg.empty()) did_something = true;
        it->knights_client->receiveInputData(net_msg);
    }

    return did_something;
}

//////////////////////////////////////////////
// Broadcast Replies
//////////////////////////////////////////////

void KnightsApp::startBroadcastReplies(int server_port)
{
    // Listen for client requests on the BROADCAST_PORT.
    pimpl->broadcast_socket = pimpl->net_driver->createUDPSocket(BROADCAST_PORT, true);
    pimpl->broadcast_last_time = 0;
    pimpl->server_port = server_port;
}

bool KnightsAppImpl::processBroadcastMsgs()
{
    if (!broadcast_socket) return false;
    
    // don't run this more than once per second.
    const unsigned int time_now = timer->getMsec();
    if (time_now - broadcast_last_time < 1000) return false;
    broadcast_last_time = time_now;
    
    const int num_players = knights_server ? knights_server->getNumberOfPlayers() : 0;

    // check for incoming messages, send replies if necessary.
    bool sent_reply = false;
    std::string msg, address;
    int port;
    while (broadcast_socket->receive(address, port, msg)) {
        if (msg == BROADCAST_PING_MSG) {
            // Construct the reply string
            std::string reply = BROADCAST_PONG_HDR;
            reply += static_cast<unsigned char>(server_port >> 8);
            reply += static_cast<unsigned char>(server_port & 0xff);
            reply += 'L';
            reply += static_cast<unsigned char>(num_players >> 8);
            reply += static_cast<unsigned char>(num_players & 0xff);
            
            // Send the reply back to the address/port where the broadcast came from.
            broadcast_socket->send(address, port, reply);            

            sent_reply = true;
        }
    }

    return (sent_reply);
}


//////////////////////////////////////////////
// Game Manager
//////////////////////////////////////////////

void KnightsApp::createGameManager(boost::shared_ptr<KnightsClient> knights_client, bool single_player, 
                                   bool tutorial_mode, bool autostart_mode, const std::string &my_player_name)
{
    if (pimpl->game_manager) throw UnexpectedError("GameManager created twice");
    pimpl->game_manager.reset(new GameManager(*this, knights_client, pimpl->timer, single_player, tutorial_mode,
                                              autostart_mode, my_player_name));
}

void KnightsApp::destroyGameManager()
{
    pimpl->game_manager.reset();
}

GameManager & KnightsApp::getGameManager()
{
    if (!pimpl->game_manager) throw UnexpectedError("GameManager unavailable");
    return *pimpl->game_manager;
}
