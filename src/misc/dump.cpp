/*
 * dump.cpp
 *
 * This file is part of Knights.
 *
 * Copyright (C) Stephen Thompson, 2006 - 2011.
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
 * This is just a short test program to dump out some information from
 * the binary files produced by the server's BinaryLog option. Useful
 * for debugging the recording/playback mechanism.
 *
 */

#include "../svr_main/replay_file.cpp"

#include <iostream>

int main(int argc, char** argv)
{
    if (argc < 2) return 1;
    
    ReplayFile rf(argv[1]);
    while (1) {
        std::string msg, extra;
        int arg;
        rf.readMessage(msg, arg, extra);
        std::cout << msg << "\t" << arg << "\t" << extra.size() << "\t";
        if (msg == "UPD") {
            std::cout << *reinterpret_cast<const int*>(extra.c_str());
        } else if (msg == "RCV") {
            for (int i = 0; i < 5 && i < extra.size(); ++i) {
                std::cout << (unsigned int)((unsigned char)extra[i]) << "\t";
            }
        }
        std::cout << "\n";
        
        if (msg == "EOF") break;
    }

    return 0;
}
