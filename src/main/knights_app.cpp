/*
 * knights_app.cpp
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

#include "config_map.hpp"
#include "credits_screen.hpp"
#include "dummy_online_platform.hpp"
#include "error_screen.hpp"
#include "file_cache.hpp"
#include "frame_timer.hpp"
#include "game_manager.hpp"
#include "gfx_manager.hpp"
#include "gfx_resizer_compose.hpp"
#include "gfx_resizer_nearest_nbr.hpp"
#include "gfx_resizer_scale2x.hpp"
#include "graphic.hpp"
#include "keyboard_controller.hpp"
#include "knights_app.hpp"
#include "knights_client.hpp"
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
#include "simple_knights_lobby.hpp"
#include "skull_renderer.hpp"
#include "sound_manager.hpp"
#include "title_screen.hpp"

// coercri
#include "core/coercri_error.hpp"
#include "enet/enet_network_driver.hpp"
#include "gcn/cg_font.hpp"
#include "gcn/cg_listener.hpp"
#include "gfx/freetype_ttf_loader.hpp"
#include "gfx/load_bmp.hpp"
#include "gfx/window_listener.hpp"
#include "network/network_connection.hpp"
#include "network/udp_socket.hpp"
#include "sdl/gfx/sdl_gfx_driver.hpp"
#include "sdl/sound/sdl_sound_driver.hpp"
#include "timer/generic_timer.hpp"


// curl
#include <curl/curl.h>

// guichan
#include "guichan.hpp"

// lua
#include "include_lua.hpp"

// boost
#include "boost/filesystem.hpp"
#include "boost/filesystem/fstream.hpp"

#include <cstdlib>
#include <iostream>
#include <random>
#include <sstream>
#include <unordered_map>

#ifdef __LP64__
#include <stdint.h>
#define int_pointer_type intptr_t
#else
#define int_pointer_type unsigned long
#endif

#ifdef WIN32
#include <shlobj.h>
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

        std::unique_ptr<Graphic> gfx(CreateGraphicFromLua(lua));

        if (!gfx->getFileInfo().isStandardFile()) {
            throw std::runtime_error("Error in client config, non-local filename '" + 
                gfx->getFileInfo().getPath()
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
    std::unique_ptr<Screen> current_screen, requested_screen;
    
    bool running;

    boost::shared_ptr<GfxManager> gfx_manager;
    boost::shared_ptr<SoundManager> sound_manager;
    FileCache file_cache;
    boost::shared_ptr<Controller> left_controller, right_controller, net_game_controller;

    boost::shared_ptr<Coercri::Font> font;
    std::unique_ptr<gcn::Font> gcn_font;

    std::unique_ptr<Options> options;
    boost::filesystem::path options_filename;
    bool player_name_changed;  // has options->player_name been changed

    ConfigMap config_map;
    GfxVector config_gfx;  // Gfx loaded from the client config file.

    Graphic *winner_image;
    Graphic *loser_image;
    Graphic *menu_gfx_centre, *menu_gfx_empty, *menu_gfx_highlight;
    Graphic *speech_bubble;
    std::unique_ptr<PotionRenderer> potion_renderer;
    std::unique_ptr<SkullRenderer> skull_renderer;

    // online platform stuff and net driver
#ifdef ONLINE_PLATFORM
    std::unique_ptr<OnlinePlatform> online_platform;
    std::unique_ptr<PlatformLobby> platform_lobby;
#else
    std::unique_ptr<Coercri::NetworkDriver> net_driver;
#endif

    // knights lobby
    std::string server_config_filename;
    std::unique_ptr<KnightsLobby> knights_lobby;
    boost::shared_ptr<KnightsClient> knights_client;

    // broadcast socket
    boost::shared_ptr<Coercri::UDPSocket> broadcast_socket;
    unsigned int broadcast_last_time;
    int server_port;

    // game manager
    std::unique_ptr<GameManager> game_manager;

    // autostart mode
    bool autostart;
    
    // localization strings
    std::unordered_map<int, Coercri::UTF8String> localization_strings;

    // functions
    KnightsAppImpl() : running(true), player_name_changed(false) { }

    void saveOptions();
    
    void popPotionSetup(lua_State*);
    void popSkullSetup(lua_State*);

    void processBroadcastMsgs();

    void updateOnlinePlatform();
};

/////////////////////////////////////////////////////
// Constructor (One-Time Initialization)
/////////////////////////////////////////////////////

KnightsApp::KnightsApp(DisplayType display_type, const boost::filesystem::path &resource_dir, const std::string &config_filename,
                       bool autostart)
    : pimpl(new KnightsAppImpl)
{
    const char * game_name = "Knights";

    pimpl->server_config_filename = config_filename;
    pimpl->autostart = autostart;
    
    // initialize RNG
    g_rng.initialize();

    // initialize resource lib
    std::cout << "Loading data files from \"" << resource_dir.string() << "\".\n";
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
            false,    // look in root dir only
            false);   // no dofile namespace proposal

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
    
    // Read knights_data/client/localization_strings.txt
    readLocalizationStrings();

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
    pimpl->gfx_driver.reset(new Coercri::SDLGfxDriver);

#ifndef DISABLE_SOUND
    try {
        pimpl->sound_driver.reset(new Coercri::SDLSoundDriver(pimpl->config_map.getInt("sound_volume") / 100.0f));
    } catch (std::exception &e) {
        // Print warning message to cout, and continue without a sound_driver
        std::cout << "Problem initializing audio: " << e.what() << std::endl;
    }
#endif
    
    pimpl->timer.reset(new Coercri::GenericTimer);
    pimpl->ttf_loader.reset(new Coercri::FreetypeTTFLoader(pimpl->gfx_driver));

#ifdef ONLINE_PLATFORM
    // initialize online platform
#ifdef ONLINE_PLATFORM_DUMMY
    pimpl->online_platform.reset(new DummyOnlinePlatform);
#else
#error "Online platform not defined"
#endif
#else
    // use the enet network driver
    pimpl->net_driver.reset(new Coercri::EnetNetworkDriver(32, 1, true));
    pimpl->net_driver->enableServer(false);  // start off disabled.
#endif

    // initialize curl. tell it not to init winsock since EnetNetworkDriver will have done that already.
    curl_global_init(CURL_GLOBAL_NOTHING);

    // Open the game window.
    Coercri::WindowParams params;
    params.resizable = true;
    params.fullscreen = pimpl->options->fullscreen;
    params.maximized = pimpl->options->maximized;
    params.vsync = (pimpl->config_map.getInt("vsync") != 0);
    params.title = game_name;
    if (display_type == DT_WINDOWED) params.fullscreen = false;
    if (display_type == DT_FULLSCREEN) params.fullscreen = true;
    if (!params.fullscreen) {
        getWindowedModeSize(params.width, params.height);
    }

    try {
        pimpl->window = pimpl->gfx_driver->createWindow(params);
    } catch (Coercri::CoercriError &) {
        // If creation in fullscreen mode fails then try again in windowed mode (Trac #118)
        // (If it wasn't fullscreen mode then re-throw the exception)
        if (params.fullscreen) {
            // shrink the window a little bit, so it doesn't fill the entire screen
            params.width = std::max(params.width - 200, 400);
            params.height = std::max(params.height - 200, 400);
            params.maximized = params.fullscreen = false;
            pimpl->window = pimpl->gfx_driver->createWindow(params);
        } else {
            throw;
        }
    }

    // Set the window icon
    {
        RStream str("client/knights_icon_48.bmp");
        boost::shared_ptr<Coercri::PixelArray> parr = Coercri::LoadBMP(str);
        // do color keying
        for (int j = 0; j < parr->getHeight(); ++j) {
            for (int i = 0; i < parr->getWidth(); ++i) {
                if ((*parr)(i, j) == Coercri::Color(255, 0, 255, 255)) {
                    (*parr)(i, j) = Coercri::Color(0, 0, 0, 0);
                }
            }
        }
        try {
            pimpl->window->setIcon(*parr);
        } catch (const Coercri::CoercriError &) {
            // ignore
        }
    }

    // Get the font names.
    // NOTE: we always try to load TTF before bitmap fonts, irrespective of the order they are in the file.
    std::vector<std::string> ttf_font_names, bitmap_font_names;
    {
        RStream str("client/fonts.txt");

        while (str) {
            std::string x;
            getline(str, x);
        
            // Left trim
            x.erase(x.begin(), find_if(x.begin(), x.end(), [](char c) { return !IsSpace(c); }));
            // Right trim
            x.erase(find_if(x.rbegin(), x.rend(), [](char c) { return !IsSpace(c); }).base(), x.end());
            
            if (x.empty()) continue;
        
            if (x[0] == '+') bitmap_font_names.push_back(x.substr(1));
            else if (x[0] != '#') ttf_font_names.push_back(x);
        }
    }

    // Font for the gui
    pimpl->font = LoadFont(pimpl->gfx_driver,
                           *pimpl->ttf_loader,
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
    width = std::max(100, pimpl->options->window_width);
    height = std::max(100, pimpl->options->window_height);
}


//////////////////////////////////////////////
// Screen handling
//////////////////////////////////////////////

void KnightsApp::requestScreenChange(std::unique_ptr<Screen> screen)
{
    pimpl->requested_screen = std::move(screen);
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
        pimpl->current_screen = std::move(pimpl->requested_screen);   // clears requested_screen

        // Initialize the new screen, and bring up the gui if required
        const bool requires_gui = pimpl->current_screen->start(*this, pimpl->window, *pimpl->gui);
        if (requires_gui) pimpl->cg_listener->enableGui();

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

const UTF8String & KnightsApp::getPlayerName() const
{
    return pimpl->options->player_name;
}

void KnightsApp::setPlayerName(const UTF8String &name) 
{
    if (name != pimpl->options->player_name) {
        pimpl->options->player_name = name;
        pimpl->player_name_changed = true;
    }
}


//////////////////////////////////////////////
// Game state handling
//////////////////////////////////////////////

void KnightsApp::resetAll()
{
    // Destroy the KnightsLobby and KnightsClient if these exist
    pimpl->knights_lobby.reset();
    pimpl->knights_client.reset();

    // Exit the platform lobby if applicable
#ifdef ONLINE_PLATFORM
    pimpl->platform_lobby.reset();
#endif

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
#ifdef ONLINE_PLATFORM
    return pimpl->online_platform->getNetworkDriver();
#else
    return *pimpl->net_driver;
#endif
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
        std::ofstream str(options_filename.c_str(), std::ios_base::out | std::ios_base::trunc);
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
    std::unique_ptr<Screen> initial_screen;

    if (pimpl->autostart) {
        initial_screen.reset(new LoadingScreen(-1, UTF8String(), true, true, false, true));  // single player on, menu-strict on, tutorial off, autostart on
    } else {
        // Go to title screen.
        if (pimpl->options->first_time) {
            initial_screen.reset(new CreditsScreen("client/first_time_message.txt", 60));
            pimpl->saveOptions();  // make sure they don't get the first time screen again
        } else {
            initial_screen.reset(new TitleScreen);
        }
    }

    requestScreenChange(std::move(initial_screen));
    executeScreenChange();

    // Error Handling system.
    std::string error;
    int num_errors = 0;

    // Frame timing
    FrameTimer frame_timer(*pimpl->timer,
                           pimpl->config_map.getInt("max_fps"));
    bool actively_drawing = false;

    // Main Loop
    while (pimpl->running) {

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
                std::unique_ptr<Screen> screen;
                screen.reset(new ErrorScreen(error));
                requestScreenChange(std::move(screen));
                executeScreenChange();
                error.clear();
                ++num_errors;
            }

            // Handle screen changes if necessary
            executeScreenChange();

            // Handle LAN broadcasts if necessary
            pimpl->processBroadcastMsgs();

            // Update Online Platform
            pimpl->updateOnlinePlatform();


            // Read input from the network (e.g. server telling us to move a knight on-screen)
            // (Do this first, so that the frame we are about to draw can include the latest updates)
            while (getNetworkDriver().doEvents()) {}
            if (pimpl->knights_lobby) {
                pimpl->knights_lobby->readIncomingMessages(*pimpl->knights_client);
            }

            // Read window system events (e.g. mouse/keyboard control inputs)
            // Note: If we are not actively drawing frames then we are prepared to wait
            // a few milliseconds for the first event (to reduce CPU usage)
            if (!actively_drawing) {
                pimpl->gfx_driver->waitEventMsec(100);
            }
            while (pimpl->gfx_driver->pollEvents()) {}
            while (pimpl->cg_listener->processInput()) {}

            // Update the current screen
            pimpl->current_screen->update();

            // Screen::update might have sent outgoing network messages (e.g. in response
            // to the user clicking the mouse). Send these to the server now
            if (pimpl->knights_lobby) {
                pimpl->knights_lobby->sendOutgoingMessages(*pimpl->knights_client);
                while (getNetworkDriver().doEvents()) {}
            }


            // Work out if we need to draw.
            // (Draw if window needsRepaint(), but not if the screen is about to change.)
            bool need_draw = pimpl->window->needsRepaint();
            if (pimpl->requested_screen.get() != pimpl->current_screen.get()
            && pimpl->requested_screen.get()) {
                need_draw = false;
            }

            if (need_draw) {
                // Repaint the window.
                std::unique_ptr<Coercri::GfxContext> gc = pimpl->window->createGfxContext();

                gc->clearClipRectangle();
                gc->clearScreen(Coercri::Color(0,0,0));
                pimpl->cg_listener->draw(*gc);
                pimpl->current_screen->draw(
                    frame_timer.getFrameTimestampUsec(),
                    *gc);
                pimpl->window->cancelInvalidRegion();

                gc.reset();
            }

            actively_drawing = need_draw;

            frame_timer.markEndOfFrame();


            // Sleep if necessary (so as not to exceed max_fps)
            int64_t sleep_time_us = frame_timer.getSleepTimeUsec();
            if (sleep_time_us > 0) {
                pimpl->timer->sleepUsec(sleep_time_us);
            }


            // If we reach the end of the main loop without error, then
            // assume things are going well, and reset num_errors.
            num_errors = 0;


        } catch (LuaError&) {
            // These are big and better displayed on stdout than in a guichan dialog box:
            throw;
        } catch (std::exception &e) {
            error = std::string("ERROR: ") + e.what();
        } catch (gcn::Exception &e) {
            error = std::string("Guichan Error: ") + e.getMessage();
        } catch (...) {
            error = "Unknown Error";
        }
    }

    bool save_options_required = false;

    if (pimpl->options) {
        // If the window is maximized then don't try to save the size
        // as it then looks weird when we next start up the program.
        // Ditto if we are in full screen mode.

        bool is_maximized = false;
        try {
            is_maximized = pimpl->window->isMaximized();
        } catch (const Coercri::CoercriError &) {
            // empty
        }

        bool is_fullscreen = pimpl->window->isFullScreen();

        if (!is_maximized && !is_fullscreen) {
            
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

        // (Knights version 026+) Save window maximized state to the
        // options file (but only when running in windowed mode).
        if (is_maximized != pimpl->options->maximized && !is_fullscreen) {
            pimpl->options->maximized = is_maximized;
            save_options_required = true;
        }
    }

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
        if (!getNetworkDriver().outstandingConnections()) break;
        while (getNetworkDriver().doEvents()) ;
        pimpl->timer->sleepMsec(100);
    }
}


//////////////////////////////////////////////
// Network Game Handling
//////////////////////////////////////////////

const std::string & KnightsApp::getKnightsConfigFilename() const
{
    return pimpl->server_config_filename;
}

boost::shared_ptr<KnightsClient> KnightsApp::startLocalGame(boost::shared_ptr<KnightsConfig> config,
                                                            const std::string &game_name)
{
    pimpl->knights_lobby.reset(new SimpleKnightsLobby(pimpl->timer, config, game_name));
    pimpl->knights_client.reset(new KnightsClient);
    return pimpl->knights_client;
}

boost::shared_ptr<KnightsClient> KnightsApp::hostLanGame(int port,
                                                         boost::shared_ptr<KnightsConfig> config,
                                                         const std::string &game_name)
{
    pimpl->knights_lobby.reset(new SimpleKnightsLobby(getNetworkDriver(), pimpl->timer, port, config, game_name));
    pimpl->knights_client.reset(new KnightsClient);
    return pimpl->knights_client;
}

boost::shared_ptr<KnightsClient> KnightsApp::joinRemoteServer(const std::string &address,
                                                              int port)
{
    pimpl->knights_lobby.reset(new SimpleKnightsLobby(getNetworkDriver(), pimpl->timer, address, port));
    pimpl->knights_client.reset(new KnightsClient);
    return pimpl->knights_client;
}

#ifdef ONLINE_PLATFORM

boost::shared_ptr<KnightsClient> KnightsApp::hostOnlinePlatformGame(boost::shared_ptr<KnightsConfig> config,
                                                                    OnlinePlatform::Visibility vis,
                                                                    const std::string &game_name)
{
    pimpl->platform_lobby = pimpl->online_platform->createLobby(vis);
    if (!pimpl->platform_lobby) {
        throw std::runtime_error("Failed to create lobby");
    }

    // TODO: Replace with VMKnightsLobby when ready
    pimpl->knights_lobby.reset(new SimpleKnightsLobby(getNetworkDriver(), pimpl->timer, 0, config, game_name));
    pimpl->knights_client.reset(new KnightsClient);
    return pimpl->knights_client;
}

boost::shared_ptr<KnightsClient> KnightsApp::joinOnlinePlatformGame(const std::string &lobby_id,
                                                                    const std::string &game_name)
{
    pimpl->platform_lobby = pimpl->online_platform->joinLobby(lobby_id);
    if (!pimpl->platform_lobby) {
        throw std::runtime_error("Failed to join lobby");
    }

    // KnightsLobby creation is deferred until we know who the lobby leader is

    pimpl->knights_client.reset(new KnightsClient);
    return pimpl->knights_client;
}

#endif  // ONLINE_PLATFORM


//////////////////////////////////////////////
// Online Platform Update
//////////////////////////////////////////////

void KnightsAppImpl::updateOnlinePlatform()
{
#ifdef ONLINE_PLATFORM
    if (platform_lobby && !knights_lobby) {
        // We are in the state where we are joining a platform lobby,
        // but we haven't found out who the lobby leader is yet. When
        // we do find the leader, we can create the KnightsLobby and
        // join the game.
        std::string leader_id = platform_lobby->getLeaderId();
        if (!leader_id.empty()) {
            // We know the leader so we can now try to connect to them
            knights_lobby.reset(new SimpleKnightsLobby(online_platform->getNetworkDriver(), timer, leader_id, 0));
        }
    }
#endif
}


//////////////////////////////////////////////
// Broadcast Replies
//////////////////////////////////////////////

void KnightsApp::startBroadcastReplies(int server_port)
{
    // Listen for client requests on the BROADCAST_PORT.
    pimpl->broadcast_socket = getNetworkDriver().createUDPSocket(BROADCAST_PORT, true);
    pimpl->broadcast_last_time = 0;
    pimpl->server_port = server_port;
}

void KnightsAppImpl::processBroadcastMsgs()
{
    if (!broadcast_socket) return;
    
    // don't run this more than once per second.
    const unsigned int time_now = timer->getMsec();
    if (time_now - broadcast_last_time < 1000) return;
    broadcast_last_time = time_now;
    
    const int num_players = knights_lobby ? knights_lobby->getNumberOfPlayers() : 0;

    // check for incoming messages, send replies if necessary.
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
        }
    }
}


//////////////////////////////////////////////
// Game Manager
//////////////////////////////////////////////

void KnightsApp::createGameManager(boost::shared_ptr<KnightsClient> knights_client, bool single_player, 
                                   bool tutorial_mode, bool autostart_mode, const UTF8String &my_player_name)
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

#ifdef ONLINE_PLATFORM
OnlinePlatform & KnightsApp::getOnlinePlatform()
{
    return *pimpl->online_platform;
}
#endif


//////////////////////////////////////////////
// setQuestMessageCode
//////////////////////////////////////////////

void KnightsApp::setQuestMessageCode(int quest_msg_code)
{
#ifdef ONLINE_PLATFORM
    if (pimpl->platform_lobby
    && pimpl->platform_lobby->getLeaderId() == pimpl->online_platform->getCurrentUserId()) {
        pimpl->platform_lobby->setStatusCode(quest_msg_code);
    }
#endif
}


//////////////////////////////////////////////
// Localization support
//////////////////////////////////////////////

void KnightsApp::readLocalizationStrings()
{
    RStream file("client/localization_strings.txt");
    std::string line;
    while (std::getline(file, line)) {
        if (line.empty() || line[0] == '#') continue; // skip empty lines and comments

        std::istringstream iss(line);
        int code;
        if (iss >> code) {
            // Skip whitespace after the code
            while (iss.peek() == ' ' || iss.peek() == '\t') {
                iss.get();
            }
            // Rest of line is the message
            std::string message;
            if (std::getline(iss, message)) {
                pimpl->localization_strings[code] = Coercri::UTF8String::fromUTF8(message);
            }
        }
    }
}

const Coercri::UTF8String & KnightsApp::getLocalizationString(int msg_code) const
{
    static const Coercri::UTF8String empty_string;
    auto iter = pimpl->localization_strings.find(msg_code);
    if (iter == pimpl->localization_strings.end()) {
        return empty_string;
    } else {
        return iter->second;
    }
}

Coercri::UTF8String KnightsApp::getLocalizationString(int msg_code, const Coercri::UTF8String &param1) const
{
    const Coercri::UTF8String &base_str = getLocalizationString(msg_code);
    std::string result = base_str.asUTF8();
    
    size_t pos = result.find("%1");
    if (pos != std::string::npos) {
        result.replace(pos, 2, param1.asUTF8());
    }
    
    return Coercri::UTF8String::fromUTF8(result);
}
