# Makefile for "Knights virtual server"

# This will generate risc_vm-*.cpp and risc_vm.hpp.

# Note: This is only needed when building Knights for an "online
# platform" such as Steam. This open source distribution of Knights
# does not include any of the Steam-related code (due to licensing
# restrictions), and therefore, you will not need to use or care about
# this Makefile if you are just trying to build the normal open source
# version of Knights. If you do want to build an online platform
# version, you will need to port the Knights code yourself to your
# chosen platform, or else you can use the "DUMMY" online platform for
# experimentation and testing purposes. See also the
# "--online-platform" option to the makemakefile.py Python program.

# Requirements (for this Makefile):
#  1) A working RISC-V compiler, and a working copy of risc2cpp (see
#     https://github.com/sdthompson1/risc2cpp)
#      - Make sure these are both available on PATH
#      - If the RISC-V compiler is not called `riscv32-unknown-elf-g++` then
#        edit CXX below as required
#  2) Boost includes (adjust -I option below as required)
#  3) Lua
#      - You need to build liblua.a for RISC-V. First, download and unpack
#        the Lua source code from lua.org.
#      - Then, use the following command (from within the Lua main directory)
#        to build liblua.a:
#        make CC=riscv32-unknown-elf-g++ generic MYCFLAGS="-include climits"
#         - Note 1: we need to set "CC" to a C++, not C, compiler, to get a
#           C++-compatible build of liblua.a.
#         - Note 2: the "-include climits" is needed to work around a slight
#           issue with the Lua build system - without this extra flag, the
#           LLONG_MAX constant is not defined, and the build fails.
#      - Finally, adjust the -I and LIBS settings (below) to point to the
#        correct location(s) for your Lua build.

# This Makefile can be sped up by using "-j" (e.g. make -j4). This
# doesn't affect the (time-consuming) risc2cpp step, but it speeds up
# the steps before that quite a bit.

# Note that this Makefile doesn't track dependencies on header files.
# If in doubt, clean and rebuild everything!


# RISC-V compiler to use
CXX = riscv32-unknown-elf-g++

# Compile flags
# Adjust the include paths as needed.
CXXFLAGS = -Iboost_1_88_0 -Ilua-5.4.8/src \
	-I. -I../coercri -I../misc -I../rstream -I../shared -I../server -I../engine -I../protocol \
	-DNDEBUG -DVIRTUAL_SERVER \
	-O2 -ffast-math \
	-Wl,--emit-relocs \
	-include climits

# Library paths
# Adjust to point to wherever you built the RISC-V Lua library.
LIBS = lua-5.4.8/src/liblua.a

# Additional link flags ("emit-relocs" is required for risc2cpp)
LDFLAGS = -Wl,--emit-relocs

# Output directory
BUILD_DIR = build

# RISC-V binary name
RISCV_EXE = $(BUILD_DIR)/knights_virtual_server.risc

# C++ files to build.
CXXFILES = \
	coercri/core/utf8string.cpp \
	coercri/network/byte_buf.cpp \
	engine/impl/action_data.cpp \
	engine/impl/anim_lua_ctor.cpp \
	engine/impl/concrete_traps.cpp \
	engine/impl/control.cpp \
	engine/impl/control_actions.cpp \
	engine/impl/coord_transform.cpp \
	engine/impl/create_monster_type.cpp \
	engine/impl/create_tile.cpp \
	engine/impl/creature.cpp \
	engine/impl/dispel_magic.cpp \
	engine/impl/dungeon_generator.cpp \
	engine/impl/dungeon_layout.cpp \
	engine/impl/dungeon_map.cpp \
	engine/impl/entity.cpp \
	engine/impl/event_manager.cpp \
	engine/impl/gore_manager.cpp \
	engine/impl/healing_task.cpp \
	engine/impl/home_manager.cpp \
	engine/impl/item.cpp \
	engine/impl/item_check_task.cpp \
	engine/impl/item_generator.cpp \
	engine/impl/item_respawn_task.cpp \
	engine/impl/item_type.cpp \
	engine/impl/knight.cpp \
	engine/impl/knight_task.cpp \
	engine/impl/knights_config.cpp \
	engine/impl/knights_config_impl.cpp \
	engine/impl/knights_engine.cpp \
	engine/impl/legacy_action.cpp \
	engine/impl/load_segments.cpp \
	engine/impl/lockable.cpp \
	engine/impl/lua_check.cpp \
	engine/impl/lua_exec_coroutine.cpp \
	engine/impl/lua_func.cpp \
	engine/impl/lua_game_setup.cpp \
	engine/impl/lua_ingame.cpp \
	engine/impl/lua_setup.cpp \
	engine/impl/lua_userdata.cpp \
	engine/impl/magic_actions.cpp \
	engine/impl/magic_map.cpp \
	engine/impl/mediator.cpp \
	engine/impl/menu_wrapper.cpp \
	engine/impl/missile.cpp \
	engine/impl/monster.cpp \
	engine/impl/monster_definitions.cpp \
	engine/impl/monster_manager.cpp \
	engine/impl/monster_support.cpp \
	engine/impl/monster_task.cpp \
	engine/impl/monster_type.cpp \
	engine/impl/overlay_lua_ctor.cpp \
	engine/impl/player.cpp \
	engine/impl/player_task.cpp \
	engine/impl/quest_hint_manager.cpp \
	engine/impl/random_int.cpp \
	engine/impl/room_map.cpp \
	engine/impl/script_actions.cpp \
	engine/impl/segment.cpp \
	engine/impl/segment_set.cpp \
	engine/impl/special_tiles.cpp \
	engine/impl/stuff_bag.cpp \
	engine/impl/sweep.cpp \
	engine/impl/task_manager.cpp \
	engine/impl/teleport.cpp \
	engine/impl/tile.cpp \
	engine/impl/time_limit_task.cpp \
	engine/impl/user_control_lua_ctor.cpp \
	engine/impl/view_manager.cpp \
	misc/config_map.cpp \
	misc/localization.cpp \
	misc/rng.cpp \
	misc/round.cpp \
	rstream/rstream.cpp \
	rstream/rstream_error.cpp \
	rstream/rstream_find.cpp \
	server/impl/knights_game.cpp \
	server/impl/knights_server.cpp \
	server/impl/my_menu_listeners.cpp \
	server/impl/server_callbacks.cpp \
	server/impl/server_dungeon_view.cpp \
	server/impl/server_mini_map.cpp \
	server/impl/server_status_display.cpp \
	shared/impl/anim.cpp \
	shared/impl/announcement_loc.cpp \
	shared/impl/colour_change.cpp \
	shared/impl/file_info.cpp \
	shared/impl/graphic.cpp \
	shared/impl/lua_exec.cpp \
	shared/impl/lua_func_wrapper.cpp \
	shared/impl/lua_load_from_rstream.cpp \
	shared/impl/lua_module.cpp \
	shared/impl/lua_ref.cpp \
	shared/impl/lua_sandbox.cpp \
	shared/impl/lua_traceback.cpp \
	shared/impl/map_support.cpp \
	shared/impl/menu.cpp \
	shared/impl/menu_item.cpp \
	shared/impl/overlay.cpp \
	shared/impl/sound.cpp \
	shared/impl/trim.cpp \
	shared/impl/user_control.cpp \
	virtual_server/syscalls.cpp \
	virtual_server/tick_data.cpp \
	virtual_server/vm_main.cpp

# Object files and object dirs (inferred from CXXFILES).
OBJFILES = $(patsubst %.cpp,$(BUILD_DIR)/%.o,$(CXXFILES))
OBJDIRS = $(sort $(dir $(OBJFILES)))

# Default target
# Note: technically this builds several risc_vm-*.cpp files, not just risc_vm-0.cpp,
# but listing only risc_vm-0.cpp here is simpler/easier...
all: risc_vm.hpp risc_vm-0.cpp

# Rule to create build directories
$(OBJDIRS):
	mkdir -p $@

# Compile C++ files
$(BUILD_DIR)/%.o: ../%.cpp | $(OBJDIRS)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Link rule
$(RISCV_EXE): $(OBJFILES)
	$(CXX) $(LDFLAGS) -o $@ $^ $(LIBS)

# Run risc2cpp
risc_vm.hpp risc_vm-0.cpp &: $(RISCV_EXE)
	@echo "Running risc2cpp (this may take some time)..."
	risc2cpp -O2 $(RISCV_EXE) risc_vm --split-cpp 25

# Clean all built files
clean:
	rm -rf $(BUILD_DIR)
	rm -f risc_vm.hpp
	rm -f risc_vm-*.cpp risc_vm-*.o risc_vm-*.P risc_vm-*.d

# Phony targets list.
.PHONY: all clean
