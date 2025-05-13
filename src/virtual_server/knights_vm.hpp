/*
 * knights_vm.hpp
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

#ifndef KNIGHTS_VM
#define KNIGHTS_VM

#include "risc_vm.hpp"  // This is built by src/virtual_server/Makefile
#include "tick_data.hpp"

#include <istream>
#include <memory>
#include <string>
#include <vector>


// Knights Virtual Server - used with host migration.

// This class represents a virtual machine that runs a Knights server.

// The virtual machine can be "ticked" by writing tick data into it,
// and it can also be snapshotted and reconstructed on another machine
// if desired.

// To use this, risc_vm.hpp and risc_vm.cpp must first be generated.
// See the Makefile in this directory.

class KnightsVM : private RiscVM, private TickCallbacks {
public:
    // Constructor.
    // Initial timer should be set to a random value (to facilitate random seeding
    // within the VM code).
    explicit KnightsVM(uint32_t initial_time_ms);


    // runTick starts (or resumes) VM execution.

    // The data in the range "tick_data_begin" to "tick_data_end" will
    // be made available for the VM to read in the special "TICK:"
    // file.

    // If "vm_output_data" is non-NULL, then any data written by the
    // VM to "TICK:" will be appended to vm_output_data. Otherwise,
    // data written to "TICK:" will be ignored.

    // The VM is allowed to read files from the host "knights_data"
    // directory (by using paths of the form "RES:dir/filename").

    // Execution continues until the next END_TICK syscall. The
    // parameter to that syscall (representing the number of
    // milliseconds that the VM wants to sleep for) is returned from
    // runTick. The host should wait until EITHER that amount of time
    // has passed, OR some significant event has occurred (e.g. new
    // network data arrived from a client), before starting the next
    // tick.

    // Note: the return value from runTick is always in the 0 to 1000
    // range (inclusive).

    // The VM execution is guaranteed to be deterministic, so if two
    // VMs are given the same "initial_time_ms" in the constructor,
    // and the same runTick calls are made (with identical tick data),
    // and the same "RES:" files are available to both machines, then
    // the two VMs will end up in exactly the same state.

    int runTick(const unsigned char *tick_data_begin,
                const unsigned char *tick_data_end,
                std::vector<unsigned char> *vm_output_data);


    // TODO: Method to disable file loading.

    // TODO: Methods to save and restore snapshots.


private:
    // Ecall handler
    enum EcallResult { ECALL_CONTINUE, ECALL_END_TICK };
    EcallResult handleEcall();

    // Helpers for ecall handling
    enum PathType { PT_ERROR, PT_TICK_DEVICE, PT_RSTREAM_FILE };
    PathType interpretPath(std::string &resource_name);
    int openRstreamFile(const std::string &resource_name);  // returns fd or error code
    void contextSwitch();

    // RiscVM overrides
    // This implements the stack guard for the thread stack
    void handleInvalidAddress(uint32_t addr);

    // TickCallbacks handlers - we are only interested in onNewTick, to get the time.
    void onNewTick(unsigned int tick_duration) {
        timer_ms += tick_duration;
    }

private:
    // Tick data and vm_output_data references
    const unsigned char *tick_data_ptr;
    const unsigned char *tick_data_end;
    std::vector<unsigned char> *vm_output_data;

    // Timer (increased by onNewTick)
    uint32_t timer_ms;

    // Access to RStream files
    std::vector<std::unique_ptr<std::istream> > files;

    // Stack guard address for the thread stack
    uint32_t second_stack_guard;

    // Alternate register set, used for the "game thread" spawned by KnightsGame
    // (which we simulate via a coroutine system)
    uint32_t alt_ra, alt_sp, alt_gp, alt_tp;
    uint32_t alt_t0, alt_t1, alt_t2, alt_s0, alt_s1;
    uint32_t alt_a0, alt_a1, alt_a2, alt_a3, alt_a4, alt_a5, alt_a6, alt_a7;
    uint32_t alt_s2, alt_s3, alt_s4, alt_s5, alt_s6, alt_s7, alt_s8, alt_s9, alt_s10, alt_s11;
    uint32_t alt_t3, alt_t4, alt_t5, alt_t6;
    uint32_t alt_pc;
};

#endif
