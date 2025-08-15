# Makefile for "Knights virtual server"

# This will regenerate risc_vm.cpp and risc_vm.hpp.

# Note: You only need to do this if you have changed any of the
# Knights C++ files listed in CXXFILES below. If you are just building
# an unmodified copy of Knights from its original source code, then
# running this Makefile won't be necessary.

# Requirements:
#  1) A working RISC-V compiler, and a working copy of risc2cpp (see
#     https://github.com/sdthompson1/risc2cpp)
#      - Make sure these are both available on PATH
#      - If the RISC-V compiler is not called `riscv32-unknown-elf-g++` then
#        edit CXX below as required
#  2) Boost includes (adjust -I option below as required)
#  3) Lua
#      - You need to build liblua.a for RISC-V. First, download and unpack
#        the Lua source code from lua.org.
#      - Next, we need to configure Lua to build in 32 bit mode. Edit
#        src/luaconf.h (in the Lua distribution), search for where LUA_32BITS
#        is defined, and set it to 1 (instead of 0).
#      - Then, use the following command (from within the Lua main directory)
#        to build liblua.a:
#            make CC=riscv32-unknown-elf-g++ generic
#        Note that you need to set "CC" to a C++, not C, compiler, to get a
#        C++-compatible build of liblua.a.
#        Note: you also need to edit luaconf.h and set LUA_32BITS to 1 (not 0)
#        as the 32-bit RISC-V compiler doesn't support 64-bit integers.

# This Makefile can be sped up by using "-j" (e.g. make -j4). This
# doesn't affect the (time-consuming) risc2cpp step, but it speeds up
# the steps before that quite a bit.

# Note that this Makefile doesn't track dependencies on header files.
# If in doubt, clean and rebuild everything!


# RISC-V compiler to use
CXX = riscv32-unknown-elf-g++

# Compile flags
# Adjust the include paths as needed.
CXXFLAGS = -Iboost_1_88_0 -Ilua-5.4.7/src \
	-I. -I../coercri -I../misc -I../rstream -I../shared -I../server -I../engine -I../protocol \
	-DNDEBUG -DVIRTUAL_SERVER \
	-O2 -ffast-math \
	-Wl,--emit-relocs

# Library paths
# Adjust to point to wherever you built the RISC-V Lua library.
LIBS = lua-5.4.7/src/liblua.a

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
	engine/impl/tutorial_manager.cpp \
	engine/impl/user_control_lua_ctor.cpp \
	engine/impl/view_manager.cpp \
	misc/config_map.cpp \
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
all: risc_vm.hpp risc_vm.cpp

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
risc_vm.hpp risc_vm.cpp &: $(RISCV_EXE)
	@echo "Running risc2cpp (this may take some time)..."
	risc2cpp -O2 $(RISCV_EXE) risc_vm.hpp risc_vm.cpp

# "clean" - clean everything except risc_vm.hpp and risc_vm.cpp.
clean:
	rm -rf $(BUILD_DIR)

# "cleanall" - gets rid of risc_vm.hpp and risc_vm.cpp as well.
cleanall: clean
	rm -f risc_vm.hpp risc_vm.cpp

# Phony targets list.
.PHONY: all clean cleanall
