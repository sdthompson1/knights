#!/bin/bash

# Command to build "knights_virtual_server", which is a test app that
# implements a standalone Knights server using the KnightsVM
# infrastructure. This might be useful for testing or debugging.

# Pass option "-O2" for an optimized build (if you are brave!).

# Please note building is very slow. An unoptimized build takes around
# 30 seconds to 1 minute; an optimized build takes around 8 hours (!).

# After building, you can run "knights_virtual_server", passing the
# path to "knights_data" as a parameter. Alternatively, if you run it
# from the directory that contains knights_data, then you don't need
# to pass a parameter.

g++ -I../coercri -I../rstream -I../misc \
    ../coercri/enet/enet_network_connection.cpp \
    ../coercri/enet/enet_network_driver.cpp \
    ../coercri/enet/enet_udp_socket.cpp \
    ../coercri/network/byte_buf.cpp \
    ../coercri/timer/generic_timer.cpp \
    ../rstream/rstream.cpp \
    ../rstream/rstream_error.cpp \
    knights_virtual_server.cpp \
    knights_vm.cpp \
    tick_data.cpp \
    risc_vm.cpp \
    -g $1 \
    -lenet -lboost_filesystem \
    -o knights_virtual_server
