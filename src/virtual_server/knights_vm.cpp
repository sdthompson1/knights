/*
 * knights_vm.cpp
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

#include "knights_vm.hpp"

#include "rstream.hpp"
#include "rstream_error.hpp"

#include <iostream>

// Debugging options
//#define LOG_ECALLS
//#define LOG_BRK_CALLS
#define LOG_UNKNOWN_CALLS

#if defined(LOG_ECALLS) || defined(LOG_BRK_CALLS) || defined(LOG_UNKNOWN_CALLS)
#include <cstdio>
#endif

namespace {
    // Fds 0, 1, 2 reserved for stdin, stdout, stderr respectively.
    // Fd 3 is reserved for the tick device.
    // Fds 4 and above are used for RStream files.
    const uint32_t TICK_DEVICE_FD = 3;
    const uint32_t BASE_FILE_DESCRIPTOR = 4;

    // One plus address of last byte of the main stack area.
    // We place this 64 KB below the top of memory, just to allow a little
    // bit of a buffer zone above the stack, before the addresses start
    // wrapping around.
    const uint32_t MAIN_STACK_TOP = 0xffff0000;

    // Resource limits - hard coded here.
    const int MAX_OPEN_FILES = 1000;
    const uint32_t MAX_DATA_BYTES = 100 * 1024 * 1024;  // Max difference between program break and initial program break.
    const uint32_t MAX_STACK_BYTES = 512 * 1024;   // Size of each stack area, in bytes.

    // Error codes.
    const int NEWLIB_ENOENT = 2;     // No such file or directory
    const int NEWLIB_EIO = 5;        // I/O error
    const int NEWLIB_EBADF = 9;      // Bad file number
    const int NEWLIB_EINVAL = 22;    // Invalid argument
    const int NEWLIB_ENFILE = 23;    // Too many open files in system
    const int NEWLIB_ENOSYS = 88;    // Function not implemented
}

KnightsVM::KnightsVM(uint32_t initial_time_ms)
    : RiscVM(MAIN_STACK_TOP - 16)   // Start with 16 zero bytes pushed onto the main stack.
{
    // There is no tick data initially
    tick_data_ptr = tick_data_end = NULL;
    vm_output_data = NULL;

    // Initial timer
    timer_ms = initial_time_ms;

    // RStream files enabled initially
    rstream_enabled = true;

    // The main stack area runs from MAIN_STACK_TOP - MAX_STACK_BYTES (inclusive)
    // to MAIN_STACK_TOP (exclusive).
    //
    // The buffer zone between the stack areas (allowing for a guard page) is
    // from MAIN_STACK_TOP - MAX_STACK_BYTES - 0x10000 (inclusive)
    // to MAIN_STACK_TOP - MAX_STACK_BYTES (exclusive).
    //
    // The secondary stack area is
    // from MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - 0x10000 (inclusive)
    // to MAIN_STACK_TOP - MAX_STACK_BYTES - 0x10000 (exclusive).
    //
    // The buffer zone below the secondary stack area (allowing for a guard page) is
    // from MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - 0x20000 (inclusive)
    // to MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - 0x10000 (exclusive).
    //
    // We will pre-allocate the top-most page of the secondary stack; this will
    // automatically prevent the main stack growing into the secondary stack area.
    // Then place the secondary stack guard another 64 KB (0x10000 bytes) below that.
    //
    allocatePage(MAIN_STACK_TOP - MAX_STACK_BYTES - 0x20000);
    second_stack_guard = MAIN_STACK_TOP - MAX_STACK_BYTES - 0x30000;

    // All "alternate" registers are zero initially.
    alt_ra = alt_sp = alt_gp = alt_tp = 0;
    alt_t0 = alt_t1 = alt_t2 = alt_s0 = alt_s1 = 0;
    alt_a0 = alt_a1 = alt_a2 = alt_a3 = alt_a4 = alt_a5 = alt_a6 = alt_a7 = 0;
    alt_s2 = alt_s3 = alt_s4 = alt_s5 = alt_s6 = alt_s7 = alt_s8 = alt_s9 = alt_s10 = alt_s11 = 0;
    alt_t3 = alt_t4 = alt_t5 = alt_t6 = 0;
    alt_pc = 0;
}

int KnightsVM::runTick(const unsigned char *tick_data_begin,
                             const unsigned char *tick_data_end_,
                             std::vector<unsigned char> *vm_output_data_)
{
    // Interpret the tick data. The only thing we are interested in
    // here is "onNewTick" which will advance timer_ms appropriately.
    // This is the only way in which time can advance inside the VM!
    ReadTickData(tick_data_begin, tick_data_end_, *this);

    // Set up pointers so that we can handle reads and writes to the
    // "TICK:" file.
    tick_data_ptr = tick_data_begin;
    tick_data_end = tick_data_end_;
    vm_output_data = vm_output_data_;

    // Now just keep calling execute() until we get an END_TICK syscall.
    EcallResult ecall_result = ECALL_CONTINUE;
    while (ecall_result == ECALL_CONTINUE) {
        execute();
        ecall_result = handleEcall();
    }

    // The sleep time is in A0.
    // Limit this to the range 0 to 1000 inclusive. (Note getA0() is unsigned.)
    if (getA0() > uint32_t(1000)) {
        return 1000;
    } else {
        return int(getA0());
    }
}

KnightsVM::EcallResult KnightsVM::handleEcall()
{
    // Syscall number is in A7. Return value goes in A0.

    switch (getA7()) {
    case 93:
        // EXIT
#ifdef LOG_ECALLS
        printf("EXIT\n");
#endif
        // This is an error, as the server is not supposed to call exit()
        // or return from main() -- it is supposed to run forever.
        // (It does happen though, e.g. if some kind of exception is thrown
        // inside the server.)
        throw std::runtime_error("Knights game simulation failed");

    case 214:
        // BRK
        // New program break is in A0
        // If this is below the original program break, just return the
        // current break. Otherwise, set and return new break.
        {
#if defined(LOG_ECALLS) || defined(LOG_BRK_CALLS)
            double megabytes = 0.0;
            if (getA0() >= getInitialProgramBreak()) {
                megabytes = (getA0() - getInitialProgramBreak()) / (1024.0 * 1024.0);
            }
            printf("BRK a0=%08x megabytes=%.2f\n", getA0(), megabytes);
#endif
            if (getA0() >= getInitialProgramBreak()
            && getA0() <= getInitialProgramBreak() + MAX_DATA_BYTES) {
                setProgramBreak(getA0());
            }
            setA0(getProgramBreak());
        }
        return ECALL_CONTINUE;

    case 403:
        // CLOCK_GETTIME
#if defined(LOG_ECALLS)
        printf("CLOCK_GETTIME a0=%d a1=%8x\n", getA0(), getA1());
#endif
        {
            // The format is a 64-bit number of seconds written to (a1)
            // followed by a 32-bit number of nanoseconds written to 8(a1).
            uint32_t seconds = timer_ms / 1000u;
            uint32_t msec = timer_ms % 1000u;
            writeWord(getA1(), seconds);
            writeWord(getA1() + 4, 0);
            writeWord(getA1() + 8, msec * 1000000u);

            // Return value should be 0 for success.
            setA0(0);
        }
        return ECALL_CONTINUE;

    case 1024:
        // OPEN. a0=pathname, a1=flags, a2=mode.
        // We ignore flags, assuming that the VM is using the flags appropriate
        // to the file (O_RDONLY for stdin, O_WRONLY for stdout, etc.).
        // We also ignore mode as this is only applicable for creating new files
        // (which we do not support).
#ifdef LOG_ECALLS
        printf("OPEN\n");
#endif
        {
            std::string filename;
            PathType pt = interpretPath(filename);
            switch (pt) {
            case PT_TICK_DEVICE:
                // This doesn't actually do anything; it just returns TICK_DEVICE_FD
#ifdef LOG_ECALLS
                printf(" - Opening tick device\n");
#endif
                setA0(TICK_DEVICE_FD);
                break;

            case PT_RSTREAM_FILE:
#ifdef LOG_ECALLS
                printf(" - Opening RStream file: %s\n", filename.c_str());
#endif
                setA0(openRstreamFile(filename));
                break;

            default:
                // Filename could not be interpreted; return "file not found" error
#ifdef LOG_ECALLS
                printf(" - Filename not recognised, returning ENOENT\n");
#endif
                setA0(-NEWLIB_ENOENT);
                break;
            }
        }
        return ECALL_CONTINUE;

    case 63:
        // READ
        // File descriptor is in A0
        // Pointer to data area is in A1
        // Max number of bytes to read is in A2
#ifdef LOG_ECALLS
        printf("READ(fd:%d)\n", getA0());
#endif
        {
            uint32_t fd = getA0();
            uint32_t file_index = fd - BASE_FILE_DESCRIPTOR;

            uint32_t addr = getA1();
            uint32_t size = getA2();

            if (fd == 0) {
                // Stdin just reads as EOF in this implementation
                setA0(0);

            } else if (fd == 1 || fd == 2) {
                // Stdout and stderr are not open for reading
                setA0(-NEWLIB_EBADF);

            } else if (fd == TICK_DEVICE_FD) {
                // Tick device: read bytes from tick_data buffer
                uint32_t count = 0;
                while (size > 0 && tick_data_ptr != tick_data_end) {
                    writeByte(addr++, *tick_data_ptr++);
                    --size;
                    ++count;
                }
                // Return number of bytes read
                setA0(count);

            } else if (fd >= BASE_FILE_DESCRIPTOR
                       && file_index < files.size()
                       && files[file_index]) {
                // RStream file

                if (files[file_index]->eof()) {
                    // We already reached eof for this file
                    setA0(0);
                } else {
                    // Read upto 1K chunks
                    char buf[1024];
                    files[file_index]->read(buf, size);
                    if (files[file_index]->fail() && !files[file_index]->eof()) {
                        // Read error
                        setA0(-NEWLIB_EIO);
                    } else {
                        // Successful read (might have been zero bytes if at eof, but that's fine)
                        uint32_t count = files[file_index]->gcount();
                        for (uint32_t i = 0; i < count; ++i) {
                            writeByte(addr + i, buf[i]);
                        }
                        setA0(count);
                    }
                }

            } else {
                // Not a valid FD
                setA0(-NEWLIB_EBADF);
            }
        }
        return ECALL_CONTINUE;

    case 64:
        // WRITE
        // File descriptor is in A0
        // Pointer to data to write is in A1
        // Number of bytes to write is in A2
#ifdef LOG_ECALLS
        printf("WRITE(fd:%d) ", getA0());
#endif
        {
            uint32_t fd = getA0();
            uint32_t addr = getA1();
            uint32_t size = getA2();

            if (fd == 1 || fd == 2) {
                // Allow writes to stdout or stderr
                std::ostream &str = (fd == 1) ? std::cout : std::cerr;
                for (uint32_t i = 0; i < size; ++i) {
                    str << char(readByteU(addr + i));
                }

                // Return number of bytes written
                setA0(size);

            } else if (fd == TICK_DEVICE_FD) {
                // Append data to vm_output_data if available (ignore otherwise)
                if (vm_output_data) {
                    uint32_t count = size;
                    while (count > 0) {
                        vm_output_data->push_back(readByteU(addr++));
                        --count;
                    }
                }

                // Successful write
                setA0(size);

            } else {
                // This file descriptor does not support writing (or is not open)
                setA0(-NEWLIB_EBADF);
            }
        }
        return ECALL_CONTINUE;

    case 57:
        // CLOSE
        // File descriptor is in A0
#ifdef LOG_ECALLS
        printf("CLOSE(fd:%d)\n", getA0());
#endif
        {
            uint32_t fd = getA0();
            uint32_t file_index = fd - BASE_FILE_DESCRIPTOR;
            if (fd >= BASE_FILE_DESCRIPTOR
            && file_index < files.size()
            && files[file_index]) {
                // RStream file is being closed
                files[file_index].reset();
                setA0(0);  // success

            } else if (fd == TICK_DEVICE_FD) {
                // Closing the TICK: file.
                // This doesn't do anything special. The call just succeeds.
                setA0(0);

            } else {
                // Either the fd is not open, or they are trying to close fd 0, 1 or 2
                // (which we do not support).
                setA0(-NEWLIB_EBADF);
            }
        }
        return ECALL_CONTINUE;

    case 80:
        // FSTAT - We don't support this, and just return ENOSYS.
        // (This doesn't seem to do any harm.)
#if defined(LOG_ECALLS)
        printf("FSTAT - ignored\n");
#endif
        setA0(-NEWLIB_ENOSYS);
        return ECALL_CONTINUE;

    case 5000:
        // Reset game thread to start running from addr a0.
        // The parameter (passed in a1) is placed in the game thread's a0 register.
#ifdef LOG_ECALLS
        printf("RESET_GAME_THREAD\n");
#endif
        alt_ra = alt_tp = 0;
        alt_t0 = alt_t1 = alt_t2 = alt_s0 = alt_s1 = 0;
        alt_a1 = alt_a2 = alt_a3 = alt_a4 = alt_a5 = alt_a6 = alt_a7 = 0;
        alt_s2 = alt_s3 = alt_s4 = alt_s5 = alt_s6 = alt_s7 = alt_s8 = alt_s9 = alt_s10 = alt_s11 = 0;
        alt_t3 = alt_t4 = alt_t5 = alt_t6 = 0;
        alt_gp = getGP();
        alt_pc = getA0();
        alt_a0 = getA1();
        alt_sp = MAIN_STACK_TOP - MAX_STACK_BYTES - 0x10000; // Top of secondary stack (see ctor for calculations)
        return ECALL_CONTINUE;

    case 5001:
        // Switch to Main Thread. Parameter in a0.
#ifdef LOG_ECALLS
        printf("SWITCH_TO_MAIN_THREAD\n");
#endif
        {
            uint32_t param = getA0();
            contextSwitch();
            setA0(param);
        }
        return ECALL_CONTINUE;

    case 5002:
        // Switch to Game Thread. No parameter is passed.
#ifdef LOG_ECALLS
        printf("SWITCH_TO_GAME_THREAD");
#endif
        contextSwitch();
        return ECALL_CONTINUE;

    case 5003:
        // End Tick. Recommended wait time (in ms) is in A0.
#ifdef LOG_ECALLS
        printf("END_TICK %d ms\n", getA0());
#endif
        // Disable rstream files after the first tick
        rstream_enabled = false;
        files.clear();  // Closes any files that are still open
        return ECALL_END_TICK;

    default:
        // Unknown syscall.
#if defined(LOG_ECALLS) || defined(LOG_UNKNOWN_CALLS)
        printf("UNKNOWN SYSCALL %d\n", getA7());
#endif
        setA0(-NEWLIB_ENOSYS);
        return ECALL_CONTINUE;
    }
}

KnightsVM::PathType KnightsVM::interpretPath(std::string &resource_filename)
{
    std::string path;
    uint32_t addr = getA0();
    while (true) {
        uint8_t ch = readByteU(addr);
        if (ch == 0) break;
        path += ch;
        if (path.size() > 10000) throw std::runtime_error("Filename too long");
        ++addr;
    }

    if (path.substr(0, 4) == "RES:") {
        resource_filename = path.substr(4);
        return PT_RSTREAM_FILE;

    } else if (path.substr(0, 5) == "TICK:") {
        return PT_TICK_DEVICE;

    } else {
        return PT_ERROR;
    }
}

int KnightsVM::openRstreamFile(const std::string &resource_filename)
{
    if (!rstream_enabled) {
        // Not allowed to read RStream files after first tick
        return -NEWLIB_ENOENT;
    }

    bool error = false;
    std::unique_ptr<RStream> str;
    try {
        str.reset(new RStream(resource_filename));
    } catch (RStreamError&) {
        // Ignore RStream errors, pass them back to the VM instead
        error = true;
    }
    // Also set error flag if str is invalid for any reason
    if (!*str) error = true;

    // If error occurred, assume it is "file not found" and pass it back to VM
    if (error) {
        return -NEWLIB_ENOENT;
    }

    // Find a spare FD
    for (int i = 0; i < files.size(); ++i) {
        if (!files[i]) {
            files[i] = std::move(str);
            return i + BASE_FILE_DESCRIPTOR;
        }
    }

    if (files.size() >= MAX_OPEN_FILES) {
        return -NEWLIB_ENFILE;
    }

    files.push_back(std::move(str));
    return int(files.size() + BASE_FILE_DESCRIPTOR - 1);
}

void KnightsVM::contextSwitch()
{
    uint32_t tmp;
    tmp = getRA(); setRA(alt_ra); alt_ra = tmp;
    tmp = getSP(); setSP(alt_sp); alt_sp = tmp;
    tmp = getGP(); setGP(alt_gp); alt_gp = tmp;
    tmp = getTP(); setTP(alt_tp); alt_tp = tmp;
    tmp = getT0(); setT0(alt_t0); alt_t0 = tmp;
    tmp = getT1(); setT1(alt_t1); alt_t1 = tmp;
    tmp = getT2(); setT2(alt_t2); alt_t2 = tmp;
    tmp = getS0(); setS0(alt_s0); alt_s0 = tmp;
    tmp = getS1(); setS1(alt_s1); alt_s1 = tmp;
    tmp = getA0(); setA0(alt_a0); alt_a0 = tmp;
    tmp = getA1(); setA1(alt_a1); alt_a1 = tmp;
    tmp = getA2(); setA2(alt_a2); alt_a2 = tmp;
    tmp = getA3(); setA3(alt_a3); alt_a3 = tmp;
    tmp = getA4(); setA4(alt_a4); alt_a4 = tmp;
    tmp = getA5(); setA5(alt_a5); alt_a5 = tmp;
    tmp = getA6(); setA6(alt_a6); alt_a6 = tmp;
    tmp = getA7(); setA7(alt_a7); alt_a7 = tmp;
    tmp = getS2(); setS2(alt_s2); alt_s2 = tmp;
    tmp = getS3(); setS3(alt_s3); alt_s3 = tmp;
    tmp = getS4(); setS4(alt_s4); alt_s4 = tmp;
    tmp = getS5(); setS5(alt_s5); alt_s5 = tmp;
    tmp = getS6(); setS6(alt_s6); alt_s6 = tmp;
    tmp = getS7(); setS7(alt_s7); alt_s7 = tmp;
    tmp = getS8(); setS8(alt_s8); alt_s8 = tmp;
    tmp = getS9(); setS9(alt_s9); alt_s9 = tmp;
    tmp = getS10(); setS10(alt_s10); alt_s10 = tmp;
    tmp = getS11(); setS11(alt_s11); alt_s11 = tmp;
    tmp = getT3(); setT3(alt_t3); alt_t3 = tmp;
    tmp = getT4(); setT4(alt_t4); alt_t4 = tmp;
    tmp = getT5(); setT5(alt_t5); alt_t5 = tmp;
    tmp = getT6(); setT6(alt_t6); alt_t6 = tmp;
    tmp = getPC(); setPC(alt_pc); alt_pc = tmp;
}

void KnightsVM::handleInvalidAddress(uint32_t addr)
{
    if ((addr & 0xffff0000) == second_stack_guard) {
        allocatePage(second_stack_guard);
        second_stack_guard -= 0x10000;
        if (isPageAllocated(second_stack_guard)) {
            throw std::runtime_error("Thread stack guard page is allocated (shouldn't happen?)");
        }

        // Do not allow the secondary stack guard page to sink below its minimum allowed value.
        // (See comments in KnightsVM constructor for the calculations.)
        if (second_stack_guard < MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - 0x20000) {
            throw std::runtime_error("Thread stack overflow");
        }

    } else {
        RiscVM::handleInvalidAddress(addr);
    }
}
