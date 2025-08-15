/*
 * sync_client.hpp
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

#ifndef SYNC_CLIENT_HPP
#define SYNC_CLIENT_HPP

#ifdef USE_VM_LOBBY

#include <vector>

class KnightsVM;

namespace Coercri {
    class InputByteBuf;
    class NetworkConnection;
}

class SyncClient {
public:
    SyncClient(Coercri::NetworkConnection &conn,
               KnightsVM &vm_);

    // Returns true if sync done (and SyncClient should be destroyed).
    bool processMessagesFromLeader(Coercri::InputByteBuf &buf,
                                   const std::vector<unsigned char> &msg,
                                   std::vector<unsigned char> &vm_output_data);

private:
    void receiveCatchupTicks(Coercri::InputByteBuf &buf,
                             const std::vector<unsigned char> &msg,
                             std::vector<unsigned char> &vm_output_data);

private:
    Coercri::NetworkConnection &connection;
    KnightsVM &vm;
    bool vm_config_received;
};

#endif  // USE_VM_LOBBY

#endif  // SYNC_CLIENT_HPP
