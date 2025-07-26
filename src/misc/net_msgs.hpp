/*
 * net_msgs.hpp
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
 * Messages used in the broadcast protocol (used for LAN game discovery)
 * 
 */

#ifndef NET_MSGS_HPP
#define NET_MSGS_HPP

#include "version.hpp"

#define BROADCAST_PING_MSG ("KTS" KNIGHTS_VERSION "\x02")
#define BROADCAST_PONG_HDR ("KTS" KNIGHTS_VERSION "\x03")

const int BROADCAST_PORT = 16398;

#endif
