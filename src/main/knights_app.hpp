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

#include "online_platform.hpp"
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
class KnightsConfig;
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
    // OnlinePlatform access
    //
#ifdef ONLINE_PLATFORM
    OnlinePlatform & getOnlinePlatform();
#endif

    // Notify of changes to the current quest.
    // This is used for feeding the game status back to the current PlatformLobby, if any.
    //  - quest_msg_code = 0 means no quest in progress (i.e. status is "Selecting Quest")
    //  - Otherwise, quest_msg_code is a localization string naming the current quest
    void setQuestMessageCode(int quest_msg_code);


    //
    // KnightsLobby methods
    //
    // These create an appropriate KnightsLobby representing a new
    // game, and return a KnightsClient object for the game code to
    // use. (For online platform games, a corresponding PlatformLobby
    // is also created.)
    //
    // The lobby objects will be deleted when resetAll is called.
    //

    const std::string & getKnightsConfigFilename() const;

    boost::shared_ptr<KnightsClient> startLocalGame(boost::shared_ptr<KnightsConfig> config,
                                                    const std::string &game_name);
    boost::shared_ptr<KnightsClient> hostLanGame(int port,
                                                 boost::shared_ptr<KnightsConfig> config,
                                                 const std::string &game_name);
    boost::shared_ptr<KnightsClient> joinRemoteServer(const std::string &address,
                                                      int port);

#ifdef ONLINE_PLATFORM
    boost::shared_ptr<KnightsClient> hostOnlinePlatformGame(boost::shared_ptr<KnightsConfig> config,
                                                            OnlinePlatform::Visibility vis,
                                                            const std::string &game_name);
    boost::shared_ptr<KnightsClient> joinOnlinePlatformGame(const std::string &lobby_id,
                                                            const std::string &game_name);
#endif


    //
    // LAN broadcast replies. (Reset when we return to title screen.)
    // NOTE: the port number for this is fixed currently, see net_msgs.hpp.
    //
    void startBroadcastReplies(int server_port);


    //
    // Localization strings (WIP)
    //
    const Coercri::UTF8String & getLocalizationString(int msg_code) const;
    Coercri::UTF8String getLocalizationString(int msg_code, const Coercri::UTF8String &param1) const;


private:
    void executeScreenChange();
    void setupControllers();
    void setupGfxResizer();
    void readLocalizationStrings();
    
private:
    boost::shared_ptr<KnightsAppImpl> pimpl;
};

#endif
