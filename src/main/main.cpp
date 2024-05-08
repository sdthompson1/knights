/*
 * main.cpp
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

/*
 * The main routine. Sets up a couple of error handlers etc then calls into knights_app.cpp.
 *
 */

#include "misc.hpp"

#include "find_knights_data_dir.hpp"
#include "knights_app.hpp"
#include "my_ctype.hpp"
#include "my_exceptions.hpp"
#include "version.hpp"

#ifdef WIN32
#include "windows.h"
#include <SDL.h>
#endif

#include <sstream>
#include <string>

//----------------
 
// this controls whether to catch exceptions from main & print an
// error message, or else to rely on the compiler to print enough info
// about uncaught exceptions
#define CATCH_EXCEPTIONS

//----------------

#ifdef CATCH_EXCEPTIONS
#include "guichan/exception.hpp"
#include <iostream>
using namespace std;
#endif



namespace {

    // exception classes.
    struct PrintUsageAndExit { };
    struct PrintVersionAndExit { };

    // Read one command line parameter. Throws PrintUsageAndExit if we have run out of parameters.
    std::string ReadParam(int &i, int argc, char const * const * argv)
    {
        ++i;
        if (i >= argc) throw PrintUsageAndExit();
        return argv[i];
    }

    // Parse the command line arguments. Throws PrintUsageAndExit if there is an error
    // or PrintVersionAndExit if "-v" option was given.
    void ParseCmdLineArgs(int argc, char const * const * argv,
                          DisplayType & display_type,
                          boost::filesystem::path & data_dir,
                          std::string & config_filename,
                          bool & autostart)
    {
        for (int i = 1; i < argc; ++i) {
            const std::string arg = argv[i];
            if (arg == "-f" || arg == "--fullscreen") {
                display_type = DT_FULLSCREEN;
            } else if (arg == "-w" || arg == "--window") {
                display_type = DT_WINDOWED;
            } else if (arg == "-c" || arg == "--config") {
                config_filename = ReadParam(i, argc, argv);
            } else if (arg == "-d" || arg == "--datadir") {
                data_dir = ReadParam(i, argc, argv);
            } else if (arg == "-a" || arg == "--autostart") {
                autostart = true;
            } else if (arg == "-v" || arg == "--version") {
                throw PrintVersionAndExit();
            } else {
                throw PrintUsageAndExit();
            }
        }
    }
}


extern "C" {  // needed for SDL I think

#ifdef WIN32
int KnightsMain(int argc, char **argv, const boost::filesystem::path &default_data_dir)  // On Windows this is called from WinMain below
#else
int main(int argc, char **argv)   // On other systems this is our real main function
#endif
{

#ifndef WIN32
    const boost::filesystem::path default_data_dir = FindKnightsDataDir();
#endif

    std::string err_msg;

#ifdef CATCH_EXCEPTIONS
    try {
#endif

        // Set defaults for cmd line arguments.
        DisplayType display_type = DT_DONT_CARE;
        std::string config_filename = "main.lua";
        bool autostart = false;

        boost::filesystem::path data_dir = default_data_dir;

        // Parse the cmd line arguments
        ParseCmdLineArgs(argc, argv, display_type, data_dir, config_filename, autostart);
        
        // Run the game itself:
        KnightsApp app(display_type, data_dir, config_filename, autostart);
        app.runKnights();

    } catch (PrintUsageAndExit &) {
        std::cout << "Usage: " << argv[0] << " [options]\n";
        std::cout << "\n";
        std::cout << "Display options:\n";
        std::cout << "  -f, --fullscreen:   Run in fullscreen mode\n";
        std::cout << "  -w, --window:       Run in windowed mode\n";
        std::cout << "\n";
        std::cout << "Configuration options:\n";
        std::cout << "  -a, --autostart: Automatically start game with default settings\n";
        std::cout << "     (used by map editor)\n";
        std::cout << "  -c, --config [filename]: Set config file to use\n";
        std::cout << "     (default: main.lua)\n";
        std::cout << "  -d, --datadir [directory name]: Set location of 'knights_data' directory\n";
        std::cout << "     (default: " << default_data_dir << ")\n";
        std::cout << "\n";
        std::cout << "Miscellaneous options:\n";
        std::cout << "  -v, --version:  Print Knights version and exit.\n";
        
    } catch (PrintVersionAndExit &) {
        std::cout << "Knights version " << KNIGHTS_VERSION << std::endl;
        std::cout << "Please see in-game credits for further information." << std::endl;

#ifdef CATCH_EXCEPTIONS
    } catch (LuaError &e) {
        err_msg = "ERROR: Lua error:\n";
        err_msg += e.what();
    } catch (std::exception &e) {
        err_msg = "ERROR: Caught exception:\n";
        err_msg += e.what();
    } catch (gcn::Exception &e) {
        std::ostringstream str;
        str << "ERROR: Caught guichan exception:\n";
        str << e.getMessage() + "\n";
        str << "In " << e.getFunction() << " at " << e.getFilename() << ":" << e.getLine() << "\n";
        err_msg = str.str();
    } catch (...) {
        err_msg = "ERROR: Unknown exception caught\n";
#endif
    }

    if (!err_msg.empty()) {
#ifdef WIN32
        ::MessageBox(0, err_msg.c_str(), "Knights", MB_OK | MB_ICONERROR);
#else
        cout << err_msg << endl;
#endif
    }

    return 0;
}
    
} // extern "C"


//---------------------

//
// boost::assertion_failed
//

#ifdef BOOST_ENABLE_ASSERT_HANDLER

#include "boost/assert.hpp"

void boost::assertion_failed(char const * expr, char const * function, char const * file, long line) // user defined
{
    cout << "<< BOOST ASSERTION FAILED >> " << endl;
    cout << file << ":" << line << ": " << function << ": " << expr << endl;
    exit(1);
}

#endif


#ifdef WIN32

// WinMain
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrev, LPSTR szCmdLine, int sw)
{
    // Start DDHELP.exe - this is some kind of hack to prevent files being kept open, see 
    // SDL_win32_main.c in the SDL source code
    HMODULE handle = LoadLibrary("DDRAW.DLL");
    if (handle) FreeLibrary(handle);

    // Find path to the application
    char app_path[MAX_PATH];
    DWORD pathlen = GetModuleFileName(NULL, app_path, MAX_PATH);
    while (pathlen > 0 && app_path[pathlen] != '\\') --pathlen;
    app_path[pathlen] = '\0';

    // SDL requires this:
    SDL_SetModuleHandle(GetModuleHandle(NULL));

    // Figure out the path to the 'knights_data' directory.
    std::string rdir(app_path, pathlen);
    rdir += "\\knights_data";
    
    // Parse the cmd line string into argc and argv
    std::vector<char*> arg_vec;
    arg_vec.push_back(&app_path[pathlen+1]); // program name
    char * arg = szCmdLine;
    while (arg[0]) {
        while (arg[0] && IsSpace(arg[0])) ++arg;
        if (arg[0]) {
            arg_vec.push_back(arg);
            while (arg[0] && !IsSpace(arg[0])) ++arg;
            if (arg[0]) {
                arg[0] = 0;
                ++arg;
            }
        }
    }
    
    // Call KnightsMain.
    KnightsMain(arg_vec.size(), &arg_vec[0], rdir);

    return 0;
}

#endif

// ---------------------------------

// Fix "bug" with MSVC static libs + global object constructors.
#ifdef _MSC_VER
#pragma comment (linker, "/include:_InitMagicActions")
#pragma comment (linker, "/include:_InitScriptActions")
#pragma comment (linker, "/include:_InitControls")
#endif
