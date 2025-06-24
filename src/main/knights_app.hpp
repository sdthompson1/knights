/*
 * knights_app.hpp
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

/*
 * Top level class for Knights. The main() function constructs this
 * class and calls runKnights().
 * 
 */

#ifndef KNIGHTS_APP_HPP
#define KNIGHTS_APP_HPP

#include "utf8string.hpp"

// coercri
#include "gfx/font.hpp"
#include "gfx/key_code.hpp"
#include "network/network_driver.hpp"
#include "timer/timer.hpp"

#include "boost/filesystem.hpp"
#include "boost/shared_ptr.hpp"

#include <iosfwd>
#include <memory>
#include <string>

class ConfigMap;
class Controller;
class FileCache;
class GameManager;
class GfxManager;
class Graphic;
class KnightsAppImpl;
class KnightsClient;
class KnightsServer;
class Options;
class PotionRenderer;
class Screen;
class SkullRenderer;
class SoundManager;

enum DisplayType {
    DT_DONT_CARE,
    DT_WINDOWED,
    DT_FULLSCREEN
};

class KnightsApp {
public:
    explicit KnightsApp(DisplayType dt, const boost::filesystem::path &resource_dir, const std::string &config_filename,
                        bool autostart);

    // Start the main loop
    void runKnights();

    // Screen change and Quit requests
    void requestScreenChange(std::unique_ptr<Screen> screen);
    void requestQuit();

    // Pop window to front
    void popWindowToFront();
    
    // Resets global game data (called when we go back to the title screen)
    void resetAll();

    // Just reset graphics/sounds, don't clear network connections
    void unloadGraphicsAndSounds();
    

    // "Get" methods
    boost::shared_ptr<Coercri::Font> getFont() const;  // the Coercri font being used for the guichan global widget font.
    const Controller & getLeftController() const;
    const Controller & getRightController() const;
    const Controller & getNetGameController() const;
    GfxManager & getGfxManager() const;
    SoundManager & getSoundManager() const;
    FileCache & getFileCache() const;
    const Graphic * getWinnerImage() const;
    const Graphic * getLoserImage() const;
    const Graphic * getMenuGfxCentre() const;
    const Graphic * getMenuGfxEmpty() const;
    const Graphic * getMenuGfxHighlight() const;
    const Graphic * getSpeechBubble() const;
    const PotionRenderer * getPotionRenderer() const;
    const SkullRenderer * getSkullRenderer() const;
    const ConfigMap & getConfigMap() const;
    Coercri::Timer & getTimer() const;
    Coercri::NetworkDriver & getNetworkDriver() const;

    // get the width/height to be used for windowed mode.
    void getWindowedModeSize(int &width, int &height);
    
    // Options. KnightsApp ctor will load options from disk. setAndSaveOptions will set new
    // options and save them back to disk.
    const Options & getOptions() const;
    void setAndSaveOptions(const Options &);
    
    // to work around a guichan bug. This should be called if new gui-buttons are created
    void repeatLastMouseInput();

    // used for saving/loading the player name.
    const UTF8String & getPlayerName() const;
    void setPlayerName(const UTF8String &name);


    //
    // GameManager methods
    //

    // single player means PAUSE requests will be sent to server, and chat will be blocked.
    void createGameManager(boost::shared_ptr<KnightsClient> knights_client, bool single_player, bool tutorial_mode,
                           bool autostart_mode, const UTF8String &my_player_name);
    void destroyGameManager();
    GameManager & getGameManager();
    

    //
    // KnightsClient methods
    //
    // Create KnightsClient objects. Packets will be automatically
    // routed to/from the network (or locally) as needed, and
    // callbacks will be called automatically from within the main
    // loop. All connections will be closed (and the corresponding
    // KnightsClient objects deleted) when resetAll() is called.
    //
    // (Currently the code only ever creates one KnightsClient at a
    // time, but it is theoretically possible to create multiple
    // KnightsClients if required.)
    //
    
    boost::shared_ptr<KnightsClient> openRemoteConnection(const std::string &address, int port);
    boost::shared_ptr<KnightsClient> openLocalConnection();


    //
    // KnightsServer methods
    //
    // Create KnightsServer objects. In the case of createServer() we
    // start listening for incoming connections on the given port, as
    // well as "local" connections (via openLocalConnection). For
    // createLocalServer() we will accept local connections only. Only
    // one server can be created at a time. The server will be
    // deleted (and any network connections closed) when resetAll is
    // called.
    //

    const std::string & getKnightsConfigFilename() const;
    KnightsServer * createServer(int port);
    KnightsServer * createLocalServer();

    //
    // LAN broadcast replies. (Reset when we return to title screen.)
    // NOTE: the port number for this is fixed currently, see net_msgs.hpp.
    //
    void startBroadcastReplies(int server_port);

        
private:
    void executeScreenChange();
    void setupControllers();
    void setupGfxResizer();
    
private:
    boost::shared_ptr<KnightsAppImpl> pimpl;
};

#endif
