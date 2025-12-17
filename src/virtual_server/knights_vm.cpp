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

#include "network/byte_buf.hpp"

#include <iostream>

// Debugging options
//#define LOG_ECALLS
//#define LOG_BRK_CALLS
#define LOG_UNKNOWN_CALLS
//#define LOG_TICKS

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
    // We place this BYTES_PER_PAGE below the top of memory, just to allow a little
    // bit of a buffer zone above the stack, before the addresses start
    // wrapping around.
    const uint32_t MAIN_STACK_TOP = ~(RiscVM::BYTES_PER_PAGE - 1);

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

KnightsVM::KnightsVM(std::vector<unsigned char> && random_data_)
    : RiscVM(MAIN_STACK_TOP - 16)   // Start with 16 zero bytes pushed onto the main stack.
    , hasher(0)
{
    // There is no tick data initially
    tick_data_ptr = tick_data_end = NULL;
    vm_output_data = NULL;

    // Initial timer
    timer_ms = 0;

    // RStream files enabled initially
    rstream_enabled = true;

    // The main stack area runs from MAIN_STACK_TOP - MAX_STACK_BYTES (inclusive)
    // to MAIN_STACK_TOP (exclusive).
    //
    // The buffer zone between the stack areas (allowing for a guard page) is
    // from MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE (inclusive)
    // to MAIN_STACK_TOP - MAX_STACK_BYTES (exclusive).
    //
    // The secondary stack area is
    // from MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - BYTES_PER_PAGE (inclusive)
    // to MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE (exclusive).
    // (The latter is the lowest possible guard page address for the main stack.)
    //
    // The buffer zone below the secondary stack area (allowing for a guard page) is
    // from MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - BYTES_PER_PAGE * 2 (inclusive)
    // to MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - BYTES_PER_PAGE (exclusive).
    //
    // We will pre-allocate the top-most page of the secondary stack; this will
    // automatically prevent the main stack growing into the secondary stack area.
    // Then place the secondary stack guard another 64 KB (or BYTES_PER_PAGE) below that.
    //
    allocatePage(MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE * 2);
    second_stack_guard = MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE * 3;

    // All "alternate" registers are zero initially.
    alt_ra = alt_sp = alt_gp = alt_tp = 0;
    alt_t0 = alt_t1 = alt_t2 = alt_s0 = alt_s1 = 0;
    alt_a0 = alt_a1 = alt_a2 = alt_a3 = alt_a4 = alt_a5 = alt_a6 = alt_a7 = 0;
    alt_s2 = alt_s3 = alt_s4 = alt_s5 = alt_s6 = alt_s7 = alt_s8 = alt_s9 = alt_s10 = alt_s11 = 0;
    alt_t3 = alt_t4 = alt_t5 = alt_t6 = 0;
    alt_pc = 0;

    // Random seed
    random_data = std::move(random_data_);

    // Checksumming
    next_checksum_timer_ms = 1;
    checksum_addr = 0;
}

int KnightsVM::runTicks(const unsigned char *tick_data_begin,
                        const unsigned char *tick_data_end_,
                        std::vector<unsigned char> *vm_output_data_)
{
    int sleep_time_ms = 1;

#ifdef LOG_TICKS
    const unsigned char *base = tick_data_begin;
    unsigned int prev_timer = timer_ms;
#endif

    while (tick_data_begin != tick_data_end_) {
        // Interpret the tick data. The only thing we are interested in
        // here is "onNewTick" which will advance timer_ms appropriately.
        // This is the only way in which time can advance inside the VM!
        const unsigned char *next_tick_begin = ReadTickData(tick_data_begin, tick_data_end_, *this);

#ifdef LOG_TICKS
        if (next_tick_begin - tick_data_begin > 1) {
            std::cout << "Tick: " << timer_ms - prev_timer << " ms, " << next_tick_begin - tick_data_begin << " bytes";
            if (vm_output_data_) {
                std::cout << ", initial output size: " << vm_output_data_->size();
            }
            std::cout << std::endl;
        }
#endif

        // Set up pointers so that we can handle reads and writes to the
        // "TICK:" file.
        tick_data_ptr = tick_data_begin;
        tick_data_end = next_tick_begin;
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
            sleep_time_ms = 1000;
        } else {
            sleep_time_ms = int(getA0());
        }

        // Move on to next tick (if applicable).
        tick_data_begin = next_tick_begin;
    }

#ifdef LOG_TICKS
    if (vm_output_data_ && vm_output_data_->size() > 1) {
        std::cout << "End of tick batch, " << vm_output_data_->size() << " output bytes collected" << std::endl;
    }
#endif

    return sleep_time_ms;
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
                // Writes to stdout or stderr are silently ignored.
                // Just return number of bytes written (as if the call had succeeded).
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
        alt_sp = MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE; // Top of secondary stack (see ctor for calculations)
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

    case 5004:
        // Get Random Data. Address = a0, num bytes = a1.
        {
#ifdef LOG_ECALLS
            printf("GET_RANDOM_DATA\n");
#endif
            uint32_t addr = getA0();
            uint32_t size = getA1();
            if (size != random_data.size()) {
                throw std::runtime_error("Random seed size is not correct");
            }
            for (uint32_t i = 0; i < size; ++i) {
                writeByte(addr + i, random_data[i]);
            }
            setA0(0);

            // Random data is only needed once
            std::vector<unsigned char> empty;
            random_data.swap(empty);
        }
        return ECALL_CONTINUE;

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
    // Not allowed to read RStream files after first tick
    if (!rstream_enabled) {
        return -NEWLIB_ENOENT;
    }

    // Normalize the filename
    std::string normalized_filename;
    try {
        normalized_filename = RStream::NormalizePath(resource_filename);
    } catch (RStreamError&) {
        return -NEWLIB_ENOENT;
    }

    // The VM should only be accessing server/ files
    if (!normalized_filename.starts_with("server/")) {
        return -NEWLIB_ENOENT;
    }

    // Try to open the file
    bool error = false;
    std::unique_ptr<RStream> str;
    try {
        str.reset(new RStream(normalized_filename));
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
    uint32_t page_num_mask = ~(BYTES_PER_PAGE - 1);
    if ((addr & page_num_mask) == second_stack_guard) {
        allocatePage(second_stack_guard);
        second_stack_guard -= BYTES_PER_PAGE;
        if (isPageAllocated(second_stack_guard)) {
            throw std::runtime_error("Thread stack guard page is allocated (shouldn't happen?)");
        }

        // Do not allow the secondary stack guard page to sink below its minimum allowed value.
        // (See comments in KnightsVM constructor for the calculations.)
        if (second_stack_guard < MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - BYTES_PER_PAGE * 2) {
            throw std::runtime_error("Thread stack overflow");
        }

    } else {
        RiscVM::handleInvalidAddress(addr);
    }
}


//
// Sync functions
//

uint32_t KnightsVM::getLowestAddr() const
{
    uint32_t lowest_addr = 0;
    while (!isPageAllocated(lowest_addr)) {
        lowest_addr += BYTES_PER_PAGE;
    }
    return lowest_addr;
}

std::deque<MemoryBlock> KnightsVM::getMemoryContents(uint32_t block_shift)
{
    uint32_t block_size = (1 << block_shift);

    std::deque<MemoryBlock> result;

    saveRegionStartingFrom(result, getLowestAddr(), block_size);
    saveRegionStartingFrom(result, second_stack_guard + BYTES_PER_PAGE, block_size);
    saveRegionStartingFrom(result, getGuardPageAddress() + BYTES_PER_PAGE, block_size);

    return result;
}

void KnightsVM::saveRegionStartingFrom(std::deque<MemoryBlock> &result, uint32_t addr, uint32_t block_size)
{
    // Block size must be multiple of 32.
    if ((block_size & 31) != 0) {
        throw std::logic_error("Block size must be multiple of 32");
    }

    // Save all pages from the starting address until we hit an unallocated page.
    while (isPageAllocated(addr)) {

        // Note: This calculation shouldn't wrap around, because we leave the topmost memory
        // page unallocated always.
        uint32_t top_addr = addr + block_size;

        MemoryBlock block;
        block.base_address = addr;
        block.contents.reserve(block_size >> 2);

        XXHash hasher(addr);

        // Do 32 bytes (8 words) at a time
        while (addr < top_addr) {

            uint64_t lane[4];

            for (int d = 0; d < 4; ++d) {
                uint32_t word1 = readWord(addr);
                block.contents.push_back(word1);
                addr += 4;

                uint32_t word2 = readWord(addr);
                block.contents.push_back(word2);
                addr += 4;

                uint64_t combined_word = (static_cast<uint64_t>(word1) << 32) | static_cast<uint64_t>(word2);
                lane[d] = combined_word;
            }

            hasher.updateHash(lane);
        }

        block.hash = hasher.finalHash();

        result.push_back(std::move(block));
    }
}

void KnightsVM::getVMConfig(Coercri::OutputByteBuf &buf) const
{
    buf.writeUlong(getRA());
    buf.writeUlong(getSP());
    buf.writeUlong(getGP());
    buf.writeUlong(getTP());
    buf.writeUlong(getT0());
    buf.writeUlong(getT1());
    buf.writeUlong(getT2());
    buf.writeUlong(getS0());
    buf.writeUlong(getS1());
    buf.writeUlong(getA0());
    buf.writeUlong(getA1());
    buf.writeUlong(getA2());
    buf.writeUlong(getA3());
    buf.writeUlong(getA4());
    buf.writeUlong(getA5());
    buf.writeUlong(getA6());
    buf.writeUlong(getA7());
    buf.writeUlong(getS2());
    buf.writeUlong(getS3());
    buf.writeUlong(getS4());
    buf.writeUlong(getS5());
    buf.writeUlong(getS6());
    buf.writeUlong(getS7());
    buf.writeUlong(getS8());
    buf.writeUlong(getS9());
    buf.writeUlong(getS10());
    buf.writeUlong(getS11());
    buf.writeUlong(getT3());
    buf.writeUlong(getT4());
    buf.writeUlong(getT5());
    buf.writeUlong(getT6());
    buf.writeUlong(getPC());
    buf.writeUlong(getInitialProgramBreak());
    buf.writeUlong(getProgramBreak());
    buf.writeUlong(getGuardPageAddress());
    buf.writeUlong(timer_ms);
    buf.writeUlong(second_stack_guard);
    buf.writeUlong(alt_ra);
    buf.writeUlong(alt_sp);
    buf.writeUlong(alt_gp);
    buf.writeUlong(alt_tp);
    buf.writeUlong(alt_t0);
    buf.writeUlong(alt_t1);
    buf.writeUlong(alt_t2);
    buf.writeUlong(alt_s0);
    buf.writeUlong(alt_s1);
    buf.writeUlong(alt_a0);
    buf.writeUlong(alt_a1);
    buf.writeUlong(alt_a2);
    buf.writeUlong(alt_a3);
    buf.writeUlong(alt_a4);
    buf.writeUlong(alt_a5);
    buf.writeUlong(alt_a6);
    buf.writeUlong(alt_a7);
    buf.writeUlong(alt_s2);
    buf.writeUlong(alt_s3);
    buf.writeUlong(alt_s4);
    buf.writeUlong(alt_s5);
    buf.writeUlong(alt_s6);
    buf.writeUlong(alt_s7);
    buf.writeUlong(alt_s8);
    buf.writeUlong(alt_s9);
    buf.writeUlong(alt_s10);
    buf.writeUlong(alt_s11);
    buf.writeUlong(alt_t3);
    buf.writeUlong(alt_t4);
    buf.writeUlong(alt_t5);
    buf.writeUlong(alt_t6);
    buf.writeUlong(alt_pc);
    buf.writeUlong(checksum_addr);
    buf.writeUlong(next_checksum_timer_ms);
    hasher.writeInternalState(buf);
}

void KnightsVM::putVMConfig(Coercri::InputByteBuf &buf)
{
    setRA(buf.readUlong());
    setSP(buf.readUlong());
    setGP(buf.readUlong());
    setTP(buf.readUlong());
    setT0(buf.readUlong());
    setT1(buf.readUlong());
    setT2(buf.readUlong());
    setS0(buf.readUlong());
    setS1(buf.readUlong());
    setA0(buf.readUlong());
    setA1(buf.readUlong());
    setA2(buf.readUlong());
    setA3(buf.readUlong());
    setA4(buf.readUlong());
    setA5(buf.readUlong());
    setA6(buf.readUlong());
    setA7(buf.readUlong());
    setS2(buf.readUlong());
    setS3(buf.readUlong());
    setS4(buf.readUlong());
    setS5(buf.readUlong());
    setS6(buf.readUlong());
    setS7(buf.readUlong());
    setS8(buf.readUlong());
    setS9(buf.readUlong());
    setS10(buf.readUlong());
    setS11(buf.readUlong());
    setT3(buf.readUlong());
    setT4(buf.readUlong());
    setT5(buf.readUlong());
    setT6(buf.readUlong());
    setPC(buf.readUlong());

    uint32_t initial_program_break = buf.readUlong();
    if (getInitialProgramBreak() != initial_program_break) {
        throw std::runtime_error("Sync error (program break mismatch)");
    }

    uint32_t program_break = buf.readUlong();
    if (program_break < initial_program_break
    || program_break > getInitialProgramBreak() + MAX_DATA_BYTES) {
        throw std::runtime_error("Sync error (program break out of range)");
    }
    setProgramBreak(program_break); // This will allocate or free memory for us as required

    uint32_t new_guard_page_address = buf.readUlong();
    if (new_guard_page_address < MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE
    || new_guard_page_address > MAIN_STACK_TOP - 2 * BYTES_PER_PAGE
    || (new_guard_page_address & (BYTES_PER_PAGE - 1)) != 0) {
        throw std::runtime_error("Sync error (guard page out of range or misaligned)");
    }
    adjustGuardPageAllocations(getGuardPageAddress(), new_guard_page_address);
    setGuardPageAddress(new_guard_page_address);

    timer_ms = buf.readUlong();

    uint32_t new_second_stack_guard = buf.readUlong();
    if (new_second_stack_guard < MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - BYTES_PER_PAGE * 2
    || new_second_stack_guard > MAIN_STACK_TOP - MAX_STACK_BYTES - BYTES_PER_PAGE * 3
    || (new_second_stack_guard & (BYTES_PER_PAGE - 1)) != 0) {
        throw std::runtime_error("Sync error (second stack guard out of range or misaligned)");
    }
    adjustGuardPageAllocations(second_stack_guard, new_second_stack_guard);
    second_stack_guard = new_second_stack_guard;

    alt_ra = buf.readUlong();
    alt_sp = buf.readUlong();
    alt_gp = buf.readUlong();
    alt_tp = buf.readUlong();
    alt_t0 = buf.readUlong();
    alt_t1 = buf.readUlong();
    alt_t2 = buf.readUlong();
    alt_s0 = buf.readUlong();
    alt_s1 = buf.readUlong();
    alt_a0 = buf.readUlong();
    alt_a1 = buf.readUlong();
    alt_a2 = buf.readUlong();
    alt_a3 = buf.readUlong();
    alt_a4 = buf.readUlong();
    alt_a5 = buf.readUlong();
    alt_a6 = buf.readUlong();
    alt_a7 = buf.readUlong();
    alt_s2 = buf.readUlong();
    alt_s3 = buf.readUlong();
    alt_s4 = buf.readUlong();
    alt_s5 = buf.readUlong();
    alt_s6 = buf.readUlong();
    alt_s7 = buf.readUlong();
    alt_s8 = buf.readUlong();
    alt_s9 = buf.readUlong();
    alt_s10 = buf.readUlong();
    alt_s11 = buf.readUlong();
    alt_t3 = buf.readUlong();
    alt_t4 = buf.readUlong();
    alt_t5 = buf.readUlong();
    alt_t6 = buf.readUlong();
    alt_pc = buf.readUlong();

    checksum_addr = buf.readUlong();
    next_checksum_timer_ms = buf.readUlong();
    hasher = XXHash(buf);
}

void KnightsVM::adjustGuardPageAllocations(uint32_t old_guard_page_address, uint32_t new_guard_page_address)
{
    while (old_guard_page_address < new_guard_page_address) {
        old_guard_page_address += BYTES_PER_PAGE;
        deallocatePage(old_guard_page_address);
    }
    while (old_guard_page_address > new_guard_page_address) {
        allocatePage(old_guard_page_address);
        old_guard_page_address -= BYTES_PER_PAGE;
    }
}

void KnightsVM::getMemoryHashes(Coercri::OutputByteBuf &output, uint32_t block_shift)
{
    uint32_t block_size = (1 << block_shift);
    hashRegionStartingFrom(output, getLowestAddr(), block_size);
    hashRegionStartingFrom(output, second_stack_guard + BYTES_PER_PAGE, block_size);
    hashRegionStartingFrom(output, getGuardPageAddress() + BYTES_PER_PAGE, block_size);
}

void KnightsVM::hashRegionStartingFrom(Coercri::OutputByteBuf &output, uint32_t addr, uint32_t block_size)
{
    if ((block_size & 31) != 0) {
        throw std::logic_error("Block size must be multiple of 32");
    }

    while (isPageAllocated(addr)) {
        uint32_t top_addr = addr + block_size;

        XXHash hasher(addr);

        while (addr < top_addr) {

            uint64_t lane[4];

            for (int d = 0; d < 4; ++d) {
                uint32_t word1 = readWord(addr);
                uint32_t word2 = readWord(addr + 4);
                addr += 8;

                uint64_t combined_word = (static_cast<uint64_t>(word1) << 32) | static_cast<uint64_t>(word2);
                lane[d] = combined_word;
            }

            hasher.updateHash(lane);
        }

        uint64_t hash = hasher.finalHash();

        output.writeUlong(static_cast<uint32_t>(hash));
        output.writeUlong(static_cast<uint32_t>(hash >> 32));
    }
}

void KnightsVM::compareMemoryHashes(Coercri::InputByteBuf &input,
                                    std::deque<MemoryBlock> &my_blocks,
                                    uint32_t block_shift)
{
    for (auto& block : my_blocks) {
        uint32_t hash1 = input.readUlong();
        uint32_t hash2 = input.readUlong();
        uint64_t hash = static_cast<uint64_t>(hash1)
            | (static_cast<uint64_t>(hash2) << 32);

        if (hash == block.hash) {
            // The follower already has this same block, so
            // we don't need to send it.
            // Use swap trick to free the vector's memory
            std::vector<uint32_t> empty;
            block.contents.swap(empty);
        }
    }
}

void KnightsVM::updateRollingChecksum()
{
    // Update the rolling checksum every 100ms of VM time.
    int32_t time_delta = int32_t(timer_ms - next_checksum_timer_ms);
    if (time_delta < 0) {
        return;
    }

    constexpr uint32_t CHECKSUM_INTERVAL_MS = 100;
    next_checksum_timer_ms = timer_ms + CHECKSUM_INTERVAL_MS;

    uint64_t lane[4];

    // If this is the first block to be checksummed then initialize
    // the hasher and add all the initial register contents

    if (checksum_addr == 0) {
        hasher = XXHash(timer_ms);  // Use timer_ms as seed

        lane[0] = (uint64_t(getRA()) << 32) | getSP();
        lane[1] = (uint64_t(getGP()) << 32) | getTP();
        lane[2] = (uint64_t(getT0()) << 32) | getT1();
        lane[3] = (uint64_t(getT2()) << 32) | getS0();
        hasher.updateHash(lane);

        lane[0] = (uint64_t(getS1()) << 32) | getA0();
        lane[1] = (uint64_t(getA1()) << 32) | getA2();
        lane[2] = (uint64_t(getA3()) << 32) | getA4();
        lane[3] = (uint64_t(getA5()) << 32) | getA6();
        hasher.updateHash(lane);

        lane[0] = (uint64_t(getA7()) << 32) | getS2();
        lane[1] = (uint64_t(getS3()) << 32) | getS4();
        lane[2] = (uint64_t(getS5()) << 32) | getS6();
        lane[3] = (uint64_t(getS7()) << 32) | getS8();
        hasher.updateHash(lane);

        lane[0] = (uint64_t(getS9()) << 32) | getS10();
        lane[1] = (uint64_t(getS11()) << 32) | getT3();
        lane[2] = (uint64_t(getT4()) << 32) | getT5();
        lane[3] = (uint64_t(getT6()) << 32) | getPC();
        hasher.updateHash(lane);

        lane[0] = (uint64_t(getInitialProgramBreak()) << 32) | getProgramBreak();
        lane[1] = (uint64_t(getGuardPageAddress()) << 32) | timer_ms;
        lane[2] = (uint64_t(second_stack_guard) << 32) | alt_ra;
        lane[3] = (uint64_t(alt_sp) << 32) | alt_gp;
        hasher.updateHash(lane);

        lane[0] = (uint64_t(alt_tp) << 32) | alt_t0;
        lane[1] = (uint64_t(alt_t1) << 32) | alt_t2;
        lane[2] = (uint64_t(alt_s0) << 32) | alt_s1;
        lane[3] = (uint64_t(alt_a0) << 32) | alt_a1;
        hasher.updateHash(lane);

        lane[0] = (uint64_t(alt_a2) << 32) | alt_a3;
        lane[1] = (uint64_t(alt_a4) << 32) | alt_a5;
        lane[2] = (uint64_t(alt_a6) << 32) | alt_a7;
        lane[3] = (uint64_t(alt_s2) << 32) | alt_s3;
        hasher.updateHash(lane);

        lane[0] = (uint64_t(alt_s4) << 32) | alt_s5;
        lane[1] = (uint64_t(alt_s6) << 32) | alt_s7;
        lane[2] = (uint64_t(alt_s8) << 32) | alt_s9;
        lane[3] = (uint64_t(alt_s10) << 32) | alt_s11;
        hasher.updateHash(lane);

        lane[0] = (uint64_t(alt_t3) << 32) | alt_t4;
        lane[1] = (uint64_t(alt_t5) << 32) | alt_t6;
        lane[2] = alt_pc;
        lane[3] = (rstream_enabled ? 4 : 0) | (files.empty() ? 2 : 0) | (random_data.empty() ? 1 : 0);
        hasher.updateHash(lane);
    }

    // Now advance forward until we find the next allocated page
    bool all_done = false;
    do {
        // Advance to next page
        checksum_addr += BYTES_PER_PAGE;

        // Optimization: Skip ahead because we know (in our memory layout) that
        // no page between getProgramBreak() and min_second_stack_guard will be allocated
        uint32_t min_second_stack_guard = MAIN_STACK_TOP - MAX_STACK_BYTES * 2 - BYTES_PER_PAGE * 2;
        if (checksum_addr > getProgramBreak() && checksum_addr < min_second_stack_guard) {
            checksum_addr = min_second_stack_guard;
        }

        // If the checksum addr reached zero that means we have traversed the entire
        // memory space, i.e. the checksumming is complete
        if (checksum_addr == 0) {
            all_done = true;
            break;
        }
    } while (!isPageAllocated(checksum_addr));

    if (all_done) {
        // We're done; add the new checkpoint to the queue.
        Checkpoint chk;
        chk.timer_ms = timer_ms;
        chk.checksum = hasher.finalHash();
        checkpoints.push_back(chk);

    } else {
        // We found a new page to add; add the address and contents to the hasher.
        lane[0] = checksum_addr;
        lane[1] = lane[2] = lane[3] = 0;
        hasher.updateHash(lane);
        for (int32_t offset = 0; offset < BYTES_PER_PAGE; offset += 32) {
            lane[0] = (uint64_t(readWord(checksum_addr + offset)) << 32) | readWord(checksum_addr + offset + 4);
            lane[1] = (uint64_t(readWord(checksum_addr + offset + 8)) << 32) | readWord(checksum_addr + offset + 12);
            lane[2] = (uint64_t(readWord(checksum_addr + offset + 16)) << 32) | readWord(checksum_addr + offset + 20);
            lane[3] = (uint64_t(readWord(checksum_addr + offset + 24)) << 32) | readWord(checksum_addr + offset + 28);
            hasher.updateHash(lane);
        }
    }
}

std::vector<Checkpoint> KnightsVM::getCheckpoints()
{
    std::vector<Checkpoint> result;
    result.swap(checkpoints);
    return result;
}
