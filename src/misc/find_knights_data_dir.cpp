/*
 * find_knights_data_dir.cpp
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

#include 

#include <iostream>

#include "boost/filesystem.hpp"

static std::string g_knights_data_dir;

std::string FindKnightsDataDir()
{
    if (g_knights_data_dir.empty()) {

        boost::filesystem::path p = "knights_data";

#ifdef DATA_DIR
#define _QUOTEME(x) #x
#define QUOTEME(x) _QUOTEME(x)
        boost::filesystem::path p2 = QUOTEME(DATA_DIR);

        if (p2.exists()) {
            if (p.exists()) {
                // both paths exist. print a message so the user knows which one we are using.
                std::cout << "Note: Using \"knights_data\" from the current directory, instead of \"" 
                          << DATA_DIR << "\".\n";
            } else {
                // only DATA_DIR exists. use that.
                p = p2;
            }
        }
#endif

        // we've chosen our directory, now check whether it exists.
        if (!p.exists()) {
            std::cout << "Error: Could not find the \"knights_data\" directory. Exiting.\n";
            std::exit(1);
        }

        // cache the result - this means the message (if any) will be printed only once
        g_knights_data_dir = p;
    }        
    
    return g_knights_data_dir;
}
