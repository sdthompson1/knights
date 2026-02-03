/*
 * vote_flags.hpp
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

#ifndef VOTE_FLAGS_HPP
#define VOTE_FLAGS_HPP

// Bit flags for SERVER_VOTED_TO_RESTART
enum VoteFlags {
    VF_VOTE = 1,     // Voting (set) or cancelling vote (clear)
    VF_IS_ME = 2,    // Set if the player receiving the message is the voter
    VF_SHOW_MSG = 4,     // Set if an announcement message should be printed
    VF_GAME_ENDING = 8,  // Set if the game is ending as a result of this vote
};

#endif
