/*
 * game_info.hpp
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
 * Holds information about a game running on a server.
 *
 */

#ifndef GAME_INFO_HPP
#define GAME_INFO_HPP

#include <string>

enum GameStatus {
    GS_WAITING_FOR_PLAYERS,
    GS_SELECTING_QUEST,
    GS_RUNNING
};

class GameInfo {
public:
    std::string game_name;
    int num_players;
    int num_observers;
    GameStatus status;
};

// GameInfos are compared by name (we assume game names are unique).
inline bool operator<(const GameInfo &left, const GameInfo &right)
{ return left.game_name < right.game_name; }

#endif
