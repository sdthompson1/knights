/*
 * syscalls.hpp
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

#ifndef SYSCALLS_HPP
#define SYSCALLS_HPP

// Since the RISC-V environment doesn't support multithreading, this
// file provides functions that allow us to "fake" a second thread by
// doing coroutine-like thread switching.


namespace boost {
    // Replacement for boost::thread_interrupted, required because
    // boost::thread is not available in this environment.
    struct thread_interrupted {};
}


// Functions called from main thread:

// Set up the game thread. It will start at code_addr, passing
// this_ptr as the only parameter. (This doesn't yet switch to the
// game thread; it simply sets it up.)
void vs_start_game_thread(void *code_addr, void *this_ptr);

// Switch from main to game thread. Returns 'param' passed when the
// game thread switches back.
unsigned int vs_switch_to_game_thread();

// Interrupt the game thread. If the game thread is running, this
// switches to the game thread throwing boost::thread_interrupted; the
// game thread must react by calling vs_exit_game_thread. If the game
// thread is NOT running, this is a no-op.
void vs_interrupt_game_thread();

// End the current "tick" and return control to the host. "ms" is the
// recommended time that the host should wait before starting the next
// tick.
void vs_end_tick(unsigned int ms);

// Returns true if the game thread is currently running.
bool vs_game_thread_running();

// Get random seed data (used during init)
void vs_get_random_data(void *ptr, int num_bytes);

// Functions called from game thread:

// Switch from game to main thread. 'param' is passed back. (This will
// throw boost::thread_interrupted if vs_interrupt_thread was called.)
void vs_switch_to_main_thread(unsigned int param);

// End the game thread. Switches back to main thread, stopping the
// game thread entirely.
void vs_exit_game_thread();


#endif
