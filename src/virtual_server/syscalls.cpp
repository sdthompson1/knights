/*
 * syscalls.cpp
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

#include "syscalls.hpp"

#include <stdexcept>

volatile bool g_game_thread_running = false;
volatile bool g_interrupt_flag = false;

void vs_start_game_thread(void *code_addr, void *this_ptr)
{
    if (g_game_thread_running) {
        throw std::runtime_error("Game thread already running");
    }
    g_interrupt_flag = false;
    g_game_thread_running = true;
    asm volatile(
        "mv a0, %0\n\t"     // put code_addr into a0
        "mv a1, %1\n\t"     // put this_ptr into a1
        "li a7, 5000\n\t"   // syscall number in a7
        "ecall"             // Make syscall
        : // No outputs
        : "r" (code_addr), "r" (this_ptr)   // Inputs
        : "a0", "a1", "a7"                  // Clobbered values
    );
}

unsigned int vs_switch_to_game_thread()
{
    if (!g_game_thread_running) {
        throw std::runtime_error("Game thread is not running");
    }
    unsigned int param;
    asm volatile(
        "li a7, 5002\n\t"    // Syscall number in a7
        "ecall\n\t"          // Make syscall
        "mv %0, a0"          // Return result from a0
        : "=r" (param)       // Output: param
        :                    // No inputs
        : "a0", "a7", "memory"   // Clobbered values
    );
    return param;
}

void vs_interrupt_game_thread()
{
    if (g_game_thread_running) {
        g_interrupt_flag = true;
        vs_switch_to_game_thread();  // Allow game thread to take the interrupt
        if (g_game_thread_running) {
            // Interrupt failed - this is an error
            throw std::runtime_error("Could not interrupt game thread");
        }
    }
}

void vs_end_tick(unsigned int ms)
{
    asm volatile(
        "li a7, 5003\n\t"   // Syscall number in a7
        "mv a0, %0\n\t"     // Number of ms in a0
        "ecall"             // Make syscall
        :                   // No outputs
        : "r" (ms)          // Input: ms
        : "a0", "a7"        // Clobbered values
    );
}

bool vs_game_thread_running()
{
    return g_game_thread_running;
}

void vs_switch_to_main_thread(unsigned int param)
{
    asm volatile(
        "mv a0, %0\n\t"    // Put parameter in a0
        "li a7, 5001\n\t"  // Syscall number in a7
        "ecall"            // Make syscall
        :                  // No outputs
        : "r" (param)      // Input: param
        : "a0", "a7", "memory"    // Clobbered values
    );
    if (g_interrupt_flag) {
        throw boost::thread_interrupted();
    }
}

void vs_exit_game_thread()
{
    g_game_thread_running = false;
    vs_switch_to_main_thread(0);
}
