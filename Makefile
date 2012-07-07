# Makefile for Knights
# This file is generated automatically by makemakefile.py

# NOTE: Audio can be disabled by adding -DDISABLE_SOUND to the CPPFLAGS.
# This is useful if audio doesn't work on your system for some reason.

# NOTE: If you have boost in a non standard location then you may need
# to add some -I flags to the CPPFLAGS, and/or edit the BOOST_LIBS.

PREFIX = /usr/local
BIN_DIR = $(PREFIX)/bin
DOC_DIR = $(PREFIX)/share/doc/knights
DATA_DIR = $(PREFIX)/share/knights

KNIGHTS_BINARY_NAME = knights
SERVER_BINARY_NAME = knights_server

CC = gcc
CXX = g++
STRIP = strip

CPPFLAGS = -DUSE_FONTCONFIG -DDATA_DIR=$(DATA_DIR) -DNDEBUG
CFLAGS = -O3 -ffast-math
CXXFLAGS = $(CFLAGS)
BOOST_LIBS = -lboost_thread-mt

INSTALL = install


########################################################################


OFILES_MAIN = src/coercri/gfx/bitmap_font.o src/coercri/network/byte_buf.o src/coercri/gcn/cg_font.o src/coercri/gcn/cg_graphics.o src/coercri/gcn/cg_image.o src/coercri/gcn/cg_input.o src/coercri/gcn/cg_listener.o src/coercri/enet/enet_network_connection.o src/coercri/enet/enet_network_driver.o src/coercri/enet/enet_udp_socket.o src/coercri/gfx/freetype_ttf_loader.o src/coercri/timer/generic_timer.o src/coercri/gfx/gfx_context.o src/coercri/sdl/core/istream_rwops.o src/coercri/gfx/key_name.o src/coercri/gfx/load_bmp.o src/coercri/gfx/load_system_ttf.o src/coercri/gfx/region.o src/coercri/sdl/core/sdl_error.o src/coercri/sdl/gfx/sdl_gfx_context.o src/coercri/sdl/gfx/sdl_gfx_driver.o src/coercri/sdl/gfx/sdl_graphic.o src/coercri/sdl/sound/sdl_sound_driver.o src/coercri/sdl/core/sdl_subsystem_handle.o src/coercri/sdl/timer/sdl_timer.o src/coercri/sdl/gfx/SDL_ttf.o src/coercri/sdl/gfx/sdl_ttf_font.o src/coercri/sdl/gfx/sdl_ttf_loader.o src/coercri/sdl/gfx/sdl_window.o src/coercri/gfx/window.o src/external/enet/callbacks.o src/external/enet/host.o src/external/enet/list.o src/external/enet/packet.o src/external/enet/peer.o src/external/enet/protocol.o src/external/enet/unix.o src/external/enet/win32.o src/external/guichan/src/actionevent.o src/external/guichan/src/basiccontainer.o src/external/guichan/src/widgets/button.o src/external/guichan/src/widgets/checkbox.o src/external/guichan/src/cliprectangle.o src/external/guichan/src/color.o src/external/guichan/src/widgets/container.o src/external/guichan/src/defaultfont.o src/external/guichan/src/widgets/dropdown.o src/external/guichan/src/event.o src/external/guichan/src/exception.o src/external/guichan/src/focushandler.o src/external/guichan/src/font.o src/external/guichan/src/genericinput.o src/external/guichan/src/graphics.o src/external/guichan/src/gui.o src/external/guichan/src/guichan.o src/external/guichan/src/widgets/icon.o src/external/guichan/src/image.o src/external/guichan/src/widgets/imagebutton.o src/external/guichan/src/imagefont.o src/external/guichan/src/inputevent.o src/external/guichan/src/key.o src/external/guichan/src/keyevent.o src/external/guichan/src/keyinput.o src/external/guichan/src/widgets/label.o src/external/guichan/src/widgets/listbox.o src/external/guichan/src/mouseevent.o src/external/guichan/src/mouseinput.o src/external/guichan/src/widgets/radiobutton.o src/external/guichan/src/rectangle.o src/external/guichan/src/widgets/scrollarea.o src/external/guichan/src/selectionevent.o src/external/guichan/src/widgets/slider.o src/external/guichan/src/widgets/tab.o src/external/guichan/src/widgets/tabbedarea.o src/external/guichan/src/widgets/textbox.o src/external/guichan/src/widgets/textfield.o src/external/guichan/src/widget.o src/external/guichan/src/widgets/window.o src/kconfig/gold_parser.o src/kconfig/kconfig_loader.o src/kconfig/kfile.o src/kconfig/random_int.o src/client/client_config.o src/client/knights_client.o src/engine/impl/action.o src/engine/impl/concrete_quests.o src/engine/impl/concrete_traps.o src/engine/impl/control_actions.o src/engine/impl/coord_transform.o src/engine/impl/create_quest.o src/engine/impl/creature.o src/engine/impl/dispel_magic.o src/engine/impl/dungeon_generator.o src/engine/impl/dungeon_layout.o src/engine/impl/dungeon_map.o src/engine/impl/entity.o src/engine/impl/event_manager.o src/engine/impl/gore_manager.o src/engine/impl/healing_task.o src/engine/impl/home_manager.o src/engine/impl/item.o src/engine/impl/item_check_task.o src/engine/impl/item_generator.o src/engine/impl/item_respawn_task.o src/engine/impl/item_type.o src/engine/impl/knight.o src/engine/impl/knight_task.o src/engine/impl/knights_config.o src/engine/impl/knights_config_functions.o src/engine/impl/knights_config_impl.o src/engine/impl/knights_engine.o src/engine/impl/lockable.o src/engine/impl/lua_ingame.o src/engine/impl/lua_setup.o src/engine/impl/lua_userdata.o src/engine/impl/magic_actions.o src/engine/impl/magic_map.o src/engine/impl/mediator.o src/engine/impl/menu_constraints.o src/engine/impl/missile.o src/engine/impl/monster.o src/engine/impl/monster_definitions.o src/engine/impl/monster_manager.o src/engine/impl/monster_support.o src/engine/impl/monster_task.o src/engine/impl/player.o src/engine/impl/player_task.o src/engine/impl/room_map.o src/engine/impl/script_actions.o src/engine/impl/segment.o src/engine/impl/segment_set.o src/engine/impl/special_tiles.o src/engine/impl/stuff_bag.o src/engine/impl/sweep.o src/engine/impl/task_manager.o src/engine/impl/teleport.o src/engine/impl/tile.o src/engine/impl/time_limit_task.o src/engine/impl/tutorial_manager.o src/engine/impl/view_manager.o src/main/action_bar.o src/main/adjust_list_box_size.o src/main/connecting_screen.o src/main/credits_screen.o src/main/draw.o src/main/entity_map.o src/main/error_screen.o src/main/find_server_screen.o src/main/game_manager.o src/main/gfx_manager.o src/main/gfx_resizer_compose.o src/main/gfx_resizer_nearest_nbr.o src/main/gfx_resizer_scale2x.o src/main/graphic_transform.o src/main/gui_button.o src/main/gui_centre.o src/main/gui_draw_box.o src/main/gui_numeric_field.o src/main/gui_panel.o src/main/gui_simple_container.o src/main/gui_text_wrap.o src/main/host_lan_screen.o src/main/house_colour_font.o src/main/in_game_screen.o src/main/keyboard_controller.o src/main/knights_app.o src/main/load_font.o src/main/loading_screen.o src/main/lobby_screen.o src/main/local_display.o src/main/local_dungeon_view.o src/main/local_mini_map.o src/main/local_status_display.o src/main/main.o src/main/make_scroll_area.o src/main/menu_screen.o src/main/options.o src/main/options_screen.o src/main/password_screen.o src/main/potion_renderer.o src/main/skull_renderer.o src/main/sound_manager.o src/main/start_game_screen.o src/main/tab_font.o src/main/text_formatter.o src/main/title_block.o src/main/title_screen.o src/main/x_centre.o src/server/impl/knights_game.o src/server/impl/knights_server.o src/server/impl/server_callbacks.o src/server/impl/server_dungeon_view.o src/server/impl/server_mini_map.o src/server/impl/server_status_display.o src/shared/impl/anim.o src/shared/impl/colour_change.o src/shared/impl/graphic.o src/shared/impl/lua_exec.o src/shared/impl/lua_load_from_rstream.o src/shared/lua_sandbox.o src/shared/impl/menu.o src/shared/impl/menu_item.o src/shared/impl/menu_selections.o src/shared/impl/overlay.o src/shared/impl/sound.o src/shared/impl/user_control.o src/external/lua/lapi.o src/external/lua/lauxlib.o src/external/lua/lbaselib.o src/external/lua/lcode.o src/external/lua/ldblib.o src/external/lua/ldebug.o src/external/lua/ldo.o src/external/lua/ldump.o src/external/lua/lfunc.o src/external/lua/lgc.o src/external/lua/linit.o src/external/lua/liolib.o src/external/lua/llex.o src/external/lua/lmathlib.o src/external/lua/lmem.o src/external/lua/loadlib.o src/external/lua/lobject.o src/external/lua/lopcodes.o src/external/lua/loslib.o src/external/lua/lparser.o src/external/lua/lstate.o src/external/lua/lstring.o src/external/lua/lstrlib.o src/external/lua/ltable.o src/external/lua/ltablib.o src/external/lua/ltm.o src/external/lua/lundump.o src/external/lua/lvm.o src/external/lua/lzio.o src/external/lua/print.o src/misc/config_map.o src/misc/krandom.o src/misc/metaserver_urls.o src/misc/rng.o src/misc/round.o src/rstream/rstream.o src/rstream/rstream_rwops.o

OFILES_SERVER = src/external/enet/callbacks.o src/external/enet/host.o src/external/enet/list.o src/external/enet/packet.o src/external/enet/peer.o src/external/enet/protocol.o src/external/enet/unix.o src/external/enet/win32.o src/kconfig/gold_parser.o src/kconfig/kconfig_loader.o src/kconfig/kfile.o src/kconfig/random_int.o src/engine/impl/action.o src/engine/impl/concrete_quests.o src/engine/impl/concrete_traps.o src/engine/impl/control_actions.o src/engine/impl/coord_transform.o src/engine/impl/create_quest.o src/engine/impl/creature.o src/engine/impl/dispel_magic.o src/engine/impl/dungeon_generator.o src/engine/impl/dungeon_layout.o src/engine/impl/dungeon_map.o src/engine/impl/entity.o src/engine/impl/event_manager.o src/engine/impl/gore_manager.o src/engine/impl/healing_task.o src/engine/impl/home_manager.o src/engine/impl/item.o src/engine/impl/item_check_task.o src/engine/impl/item_generator.o src/engine/impl/item_respawn_task.o src/engine/impl/item_type.o src/engine/impl/knight.o src/engine/impl/knight_task.o src/engine/impl/knights_config.o src/engine/impl/knights_config_functions.o src/engine/impl/knights_config_impl.o src/engine/impl/knights_engine.o src/engine/impl/lockable.o src/engine/impl/lua_ingame.o src/engine/impl/lua_setup.o src/engine/impl/lua_userdata.o src/engine/impl/magic_actions.o src/engine/impl/magic_map.o src/engine/impl/mediator.o src/engine/impl/menu_constraints.o src/engine/impl/missile.o src/engine/impl/monster.o src/engine/impl/monster_definitions.o src/engine/impl/monster_manager.o src/engine/impl/monster_support.o src/engine/impl/monster_task.o src/engine/impl/player.o src/engine/impl/player_task.o src/engine/impl/room_map.o src/engine/impl/script_actions.o src/engine/impl/segment.o src/engine/impl/segment_set.o src/engine/impl/special_tiles.o src/engine/impl/stuff_bag.o src/engine/impl/sweep.o src/engine/impl/task_manager.o src/engine/impl/teleport.o src/engine/impl/tile.o src/engine/impl/time_limit_task.o src/engine/impl/tutorial_manager.o src/engine/impl/view_manager.o src/server/impl/knights_game.o src/server/impl/knights_server.o src/server/impl/server_callbacks.o src/server/impl/server_dungeon_view.o src/server/impl/server_mini_map.o src/server/impl/server_status_display.o src/shared/impl/anim.o src/shared/impl/colour_change.o src/shared/impl/graphic.o src/shared/impl/lua_exec.o src/shared/impl/lua_load_from_rstream.o src/shared/lua_sandbox.o src/shared/impl/menu.o src/shared/impl/menu_item.o src/shared/impl/menu_selections.o src/shared/impl/overlay.o src/shared/impl/sound.o src/shared/impl/user_control.o src/svr_main/config.o src/svr_main/replay_file.o src/svr_main/server_main.o src/external/lua/lapi.o src/external/lua/lauxlib.o src/external/lua/lbaselib.o src/external/lua/lcode.o src/external/lua/ldblib.o src/external/lua/ldebug.o src/external/lua/ldo.o src/external/lua/ldump.o src/external/lua/lfunc.o src/external/lua/lgc.o src/external/lua/linit.o src/external/lua/liolib.o src/external/lua/llex.o src/external/lua/lmathlib.o src/external/lua/lmem.o src/external/lua/loadlib.o src/external/lua/lobject.o src/external/lua/lopcodes.o src/external/lua/loslib.o src/external/lua/lparser.o src/external/lua/lstate.o src/external/lua/lstring.o src/external/lua/lstrlib.o src/external/lua/ltable.o src/external/lua/ltablib.o src/external/lua/ltm.o src/external/lua/lundump.o src/external/lua/lvm.o src/external/lua/lzio.o src/external/lua/print.o src/misc/config_map.o src/misc/krandom.o src/misc/metaserver_urls.o src/misc/rng.o src/misc/round.o src/rstream/rstream.o src/rstream/rstream_rwops.o src/coercri/network/byte_buf.o src/coercri/enet/enet_network_driver.o src/coercri/enet/enet_network_connection.o src/coercri/enet/enet_udp_socket.o src/coercri/timer/generic_timer.o



build: $(KNIGHTS_BINARY_NAME) $(SERVER_BINARY_NAME)


src/external/enet/callbacks.o: src/external/enet/callbacks.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/host.o: src/external/enet/host.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/list.o: src/external/enet/list.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/packet.o: src/external/enet/packet.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/peer.o: src/external/enet/peer.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/protocol.o: src/external/enet/protocol.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/unix.o: src/external/enet/unix.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/enet/win32.o: src/external/enet/win32.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/svr_main/config.o: src/svr_main/config.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/misc -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/rstream -Isrc/server -Isrc/shared -Icurl/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/svr_main/replay_file.o: src/svr_main/replay_file.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/misc -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/rstream -Isrc/server -Isrc/shared -Icurl/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/svr_main/server_main.o: src/svr_main/server_main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/misc -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/rstream -Isrc/server -Isrc/shared -Icurl/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/rstream/rstream.o: src/rstream/rstream.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags`  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/rstream/rstream_rwops.o: src/rstream/rstream_rwops.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags`  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/kconfig/gold_parser.o: src/kconfig/gold_parser.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -I. -Isrc/misc -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/kconfig/kconfig_loader.o: src/kconfig/kconfig_loader.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -I. -Isrc/misc -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/kconfig/kfile.o: src/kconfig/kfile.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -I. -Isrc/misc -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/kconfig/random_int.o: src/kconfig/random_int.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -I. -Isrc/misc -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/anim.o: src/shared/impl/anim.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/colour_change.o: src/shared/impl/colour_change.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/graphic.o: src/shared/impl/graphic.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/lua_exec.o: src/shared/impl/lua_exec.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/lua_load_from_rstream.o: src/shared/impl/lua_load_from_rstream.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/lua_sandbox.o: src/shared/lua_sandbox.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/menu.o: src/shared/impl/menu.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/menu_item.o: src/shared/impl/menu_item.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/menu_selections.o: src/shared/impl/menu_selections.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/overlay.o: src/shared/impl/overlay.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/sound.o: src/shared/impl/sound.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/shared/impl/user_control.o: src/shared/impl/user_control.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared -Isrc/external/lua -Isrc/rstream  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/actionevent.o: src/external/guichan/src/actionevent.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/basiccontainer.o: src/external/guichan/src/basiccontainer.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/button.o: src/external/guichan/src/widgets/button.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/checkbox.o: src/external/guichan/src/widgets/checkbox.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/cliprectangle.o: src/external/guichan/src/cliprectangle.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/color.o: src/external/guichan/src/color.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/container.o: src/external/guichan/src/widgets/container.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/defaultfont.o: src/external/guichan/src/defaultfont.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/dropdown.o: src/external/guichan/src/widgets/dropdown.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/event.o: src/external/guichan/src/event.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/exception.o: src/external/guichan/src/exception.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/focushandler.o: src/external/guichan/src/focushandler.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/font.o: src/external/guichan/src/font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/genericinput.o: src/external/guichan/src/genericinput.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/graphics.o: src/external/guichan/src/graphics.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/gui.o: src/external/guichan/src/gui.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/guichan.o: src/external/guichan/src/guichan.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/icon.o: src/external/guichan/src/widgets/icon.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/image.o: src/external/guichan/src/image.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/imagebutton.o: src/external/guichan/src/widgets/imagebutton.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/imagefont.o: src/external/guichan/src/imagefont.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/inputevent.o: src/external/guichan/src/inputevent.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/key.o: src/external/guichan/src/key.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/keyevent.o: src/external/guichan/src/keyevent.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/keyinput.o: src/external/guichan/src/keyinput.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/label.o: src/external/guichan/src/widgets/label.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/listbox.o: src/external/guichan/src/widgets/listbox.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/mouseevent.o: src/external/guichan/src/mouseevent.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/mouseinput.o: src/external/guichan/src/mouseinput.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/radiobutton.o: src/external/guichan/src/widgets/radiobutton.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/rectangle.o: src/external/guichan/src/rectangle.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/scrollarea.o: src/external/guichan/src/widgets/scrollarea.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/selectionevent.o: src/external/guichan/src/selectionevent.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/slider.o: src/external/guichan/src/widgets/slider.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/tab.o: src/external/guichan/src/widgets/tab.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/tabbedarea.o: src/external/guichan/src/widgets/tabbedarea.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/textbox.o: src/external/guichan/src/widgets/textbox.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/textfield.o: src/external/guichan/src/widgets/textfield.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widget.o: src/external/guichan/src/widget.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/guichan/src/widgets/window.o: src/external/guichan/src/widgets/window.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external/guichan/include  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/misc/config_map.o: src/misc/config_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/misc/krandom.o: src/misc/krandom.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/misc/metaserver_urls.o: src/misc/metaserver_urls.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/misc/rng.o: src/misc/rng.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/misc/round.o: src/misc/round.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/action.o: src/engine/impl/action.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/concrete_quests.o: src/engine/impl/concrete_quests.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/concrete_traps.o: src/engine/impl/concrete_traps.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/control_actions.o: src/engine/impl/control_actions.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/coord_transform.o: src/engine/impl/coord_transform.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/create_quest.o: src/engine/impl/create_quest.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/creature.o: src/engine/impl/creature.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/dispel_magic.o: src/engine/impl/dispel_magic.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/dungeon_generator.o: src/engine/impl/dungeon_generator.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/dungeon_layout.o: src/engine/impl/dungeon_layout.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/dungeon_map.o: src/engine/impl/dungeon_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/entity.o: src/engine/impl/entity.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/event_manager.o: src/engine/impl/event_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/gore_manager.o: src/engine/impl/gore_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/healing_task.o: src/engine/impl/healing_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/home_manager.o: src/engine/impl/home_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/item.o: src/engine/impl/item.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/item_check_task.o: src/engine/impl/item_check_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/item_generator.o: src/engine/impl/item_generator.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/item_respawn_task.o: src/engine/impl/item_respawn_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/item_type.o: src/engine/impl/item_type.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/knight.o: src/engine/impl/knight.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/knight_task.o: src/engine/impl/knight_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/knights_config.o: src/engine/impl/knights_config.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/knights_config_functions.o: src/engine/impl/knights_config_functions.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/knights_config_impl.o: src/engine/impl/knights_config_impl.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/knights_engine.o: src/engine/impl/knights_engine.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/lockable.o: src/engine/impl/lockable.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/lua_ingame.o: src/engine/impl/lua_ingame.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/lua_setup.o: src/engine/impl/lua_setup.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/lua_userdata.o: src/engine/impl/lua_userdata.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/magic_actions.o: src/engine/impl/magic_actions.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/magic_map.o: src/engine/impl/magic_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/mediator.o: src/engine/impl/mediator.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/menu_constraints.o: src/engine/impl/menu_constraints.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/missile.o: src/engine/impl/missile.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/monster.o: src/engine/impl/monster.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/monster_definitions.o: src/engine/impl/monster_definitions.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/monster_manager.o: src/engine/impl/monster_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/monster_support.o: src/engine/impl/monster_support.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/monster_task.o: src/engine/impl/monster_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/player.o: src/engine/impl/player.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/player_task.o: src/engine/impl/player_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/room_map.o: src/engine/impl/room_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/script_actions.o: src/engine/impl/script_actions.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/segment.o: src/engine/impl/segment.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/segment_set.o: src/engine/impl/segment_set.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/special_tiles.o: src/engine/impl/special_tiles.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/stuff_bag.o: src/engine/impl/stuff_bag.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/sweep.o: src/engine/impl/sweep.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/task_manager.o: src/engine/impl/task_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/teleport.o: src/engine/impl/teleport.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/tile.o: src/engine/impl/tile.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/time_limit_task.o: src/engine/impl/time_limit_task.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/tutorial_manager.o: src/engine/impl/tutorial_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/engine/impl/view_manager.o: src/engine/impl/view_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/shared -I. -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/client/client_config.o: src/client/client_config.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/client/knights_client.o: src/client/knights_client.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lapi.o: src/external/lua/lapi.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lauxlib.o: src/external/lua/lauxlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lbaselib.o: src/external/lua/lbaselib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lcode.o: src/external/lua/lcode.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ldblib.o: src/external/lua/ldblib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ldebug.o: src/external/lua/ldebug.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ldo.o: src/external/lua/ldo.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ldump.o: src/external/lua/ldump.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lfunc.o: src/external/lua/lfunc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lgc.o: src/external/lua/lgc.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/linit.o: src/external/lua/linit.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/liolib.o: src/external/lua/liolib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/llex.o: src/external/lua/llex.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lmathlib.o: src/external/lua/lmathlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lmem.o: src/external/lua/lmem.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/loadlib.o: src/external/lua/loadlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lobject.o: src/external/lua/lobject.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lopcodes.o: src/external/lua/lopcodes.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/loslib.o: src/external/lua/loslib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lparser.o: src/external/lua/lparser.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lstate.o: src/external/lua/lstate.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lstring.o: src/external/lua/lstring.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lstrlib.o: src/external/lua/lstrlib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ltable.o: src/external/lua/ltable.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ltablib.o: src/external/lua/ltablib.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/ltm.o: src/external/lua/ltm.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lundump.o: src/external/lua/lundump.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lvm.o: src/external/lua/lvm.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/lzio.o: src/external/lua/lzio.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/external/lua/print.o: src/external/lua/print.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/server/impl/knights_game.o: src/server/impl/knights_game.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/server -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/server/impl/knights_server.o: src/server/impl/knights_server.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/server -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/server/impl/server_callbacks.o: src/server/impl/server_callbacks.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/server -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/server/impl/server_dungeon_view.o: src/server/impl/server_dungeon_view.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/server -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/server/impl/server_mini_map.o: src/server/impl/server_mini_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/server -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/server/impl/server_status_display.o: src/server/impl/server_status_display.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/coercri -Isrc/engine -Isrc/kconfig -Isrc/misc -Isrc/protocol -Isrc/server -Isrc/shared  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/bitmap_font.o: src/coercri/gfx/bitmap_font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/network/byte_buf.o: src/coercri/network/byte_buf.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gcn/cg_font.o: src/coercri/gcn/cg_font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gcn/cg_graphics.o: src/coercri/gcn/cg_graphics.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gcn/cg_image.o: src/coercri/gcn/cg_image.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gcn/cg_input.o: src/coercri/gcn/cg_input.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gcn/cg_listener.o: src/coercri/gcn/cg_listener.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/enet/enet_network_connection.o: src/coercri/enet/enet_network_connection.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/enet/enet_network_driver.o: src/coercri/enet/enet_network_driver.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/enet/enet_udp_socket.o: src/coercri/enet/enet_udp_socket.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/freetype_ttf_loader.o: src/coercri/gfx/freetype_ttf_loader.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/timer/generic_timer.o: src/coercri/timer/generic_timer.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/gfx_context.o: src/coercri/gfx/gfx_context.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/core/istream_rwops.o: src/coercri/sdl/core/istream_rwops.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/key_name.o: src/coercri/gfx/key_name.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/load_bmp.o: src/coercri/gfx/load_bmp.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/load_system_ttf.o: src/coercri/gfx/load_system_ttf.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/region.o: src/coercri/gfx/region.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/core/sdl_error.o: src/coercri/sdl/core/sdl_error.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/sdl_gfx_context.o: src/coercri/sdl/gfx/sdl_gfx_context.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/sdl_gfx_driver.o: src/coercri/sdl/gfx/sdl_gfx_driver.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/sdl_graphic.o: src/coercri/sdl/gfx/sdl_graphic.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/sound/sdl_sound_driver.o: src/coercri/sdl/sound/sdl_sound_driver.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/core/sdl_subsystem_handle.o: src/coercri/sdl/core/sdl_subsystem_handle.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/timer/sdl_timer.o: src/coercri/sdl/timer/sdl_timer.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/SDL_ttf.o: src/coercri/sdl/gfx/SDL_ttf.c
	$(CC) $(CPPFLAGS) $(CFLAGS) `sdl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/sdl_ttf_font.o: src/coercri/sdl/gfx/sdl_ttf_font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/sdl_ttf_loader.o: src/coercri/sdl/gfx/sdl_ttf_loader.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/sdl/gfx/sdl_window.o: src/coercri/sdl/gfx/sdl_window.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/coercri/gfx/window.o: src/coercri/gfx/window.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/external -Isrc/external/guichan/include -I.  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/action_bar.o: src/main/action_bar.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/adjust_list_box_size.o: src/main/adjust_list_box_size.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/connecting_screen.o: src/main/connecting_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/credits_screen.o: src/main/credits_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/draw.o: src/main/draw.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/entity_map.o: src/main/entity_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/error_screen.o: src/main/error_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/find_server_screen.o: src/main/find_server_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/game_manager.o: src/main/game_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gfx_manager.o: src/main/gfx_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gfx_resizer_compose.o: src/main/gfx_resizer_compose.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gfx_resizer_nearest_nbr.o: src/main/gfx_resizer_nearest_nbr.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gfx_resizer_scale2x.o: src/main/gfx_resizer_scale2x.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/graphic_transform.o: src/main/graphic_transform.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_button.o: src/main/gui_button.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_centre.o: src/main/gui_centre.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_draw_box.o: src/main/gui_draw_box.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_numeric_field.o: src/main/gui_numeric_field.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_panel.o: src/main/gui_panel.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_simple_container.o: src/main/gui_simple_container.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/gui_text_wrap.o: src/main/gui_text_wrap.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/host_lan_screen.o: src/main/host_lan_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/house_colour_font.o: src/main/house_colour_font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/in_game_screen.o: src/main/in_game_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/keyboard_controller.o: src/main/keyboard_controller.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/knights_app.o: src/main/knights_app.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/load_font.o: src/main/load_font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/loading_screen.o: src/main/loading_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/lobby_screen.o: src/main/lobby_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/local_display.o: src/main/local_display.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/local_dungeon_view.o: src/main/local_dungeon_view.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/local_mini_map.o: src/main/local_mini_map.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/local_status_display.o: src/main/local_status_display.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/main.o: src/main/main.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/make_scroll_area.o: src/main/make_scroll_area.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/menu_screen.o: src/main/menu_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/options.o: src/main/options.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/options_screen.o: src/main/options_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/password_screen.o: src/main/password_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/potion_renderer.o: src/main/potion_renderer.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/skull_renderer.o: src/main/skull_renderer.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/sound_manager.o: src/main/sound_manager.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/start_game_screen.o: src/main/start_game_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/tab_font.o: src/main/tab_font.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/text_formatter.o: src/main/text_formatter.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/title_block.o: src/main/title_block.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/title_screen.o: src/main/title_screen.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d
src/main/x_centre.o: src/main/x_centre.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` `freetype-config --cflags` -Isrc/client -Isrc/coercri -Isrc/engine -Isrc/external -Isrc/external/guichan/include -Isrc/kconfig -Isrc/misc -Isrc/rstream -Isrc/server -Isrc/shared -I. -Icurl/include -Isrc/external/lua  -MD -c -o $@ $<
	@cp $*.d $*.P; \
	  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	  rm -f $*.d

$(KNIGHTS_BINARY_NAME): $(OFILES_MAIN)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ `sdl-config --libs` `freetype-config --libs` `curl-config --libs` -lfontconfig $(BOOST_LIBS)
	$(STRIP) $@


$(SERVER_BINARY_NAME): $(OFILES_SERVER)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ `curl-config --libs` $(BOOST_LIBS)
	$(STRIP) $@


clean:
	rm -f $(OFILES_MAIN)
	rm -f $(OFILES_MAIN:.o=.d)
	rm -f $(OFILES_MAIN:.o=.P)
	rm -f $(OFILES_SERVER)
	rm -f $(OFILES_SERVER:.o=.d)
	rm -f $(OFILES_SERVER:.o=.P)
	rm -f $(KNIGHTS_BINARY_NAME)
	rm -f $(SERVER_BINARY_NAME)


install: install_knights install_server install_docs

install_knights: $(KNIGHTS_BINARY_NAME)
	$(INSTALL) -m 755 -d $(BIN_DIR)
	$(INSTALL) -m 755 $(KNIGHTS_BINARY_NAME) $(BIN_DIR)
	$(INSTALL) -m 755 -d $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/anims.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/axe.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/axe_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/axe_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/axe_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/axe_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/barrel.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/beartrap_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/beartrap_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/beartrap_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/beartrap_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/blade_trap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/blood_1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/blood_2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/blood_3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/blood_icon.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/bolts.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/bolt_horiz.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/bolt_vert.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/book.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/book_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/book_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/book_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/book_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/broken_wood_1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/broken_wood_2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/broken_wood_3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/broken_wood_4.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/broken_wood_5.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/cage.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chair_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chair_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chair_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chair_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chest_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chest_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chest_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/chest_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/click.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/client_config.lua $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/closed_bear_trap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/controls.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/credits.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/crossbow.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/crystal_ball.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dagger.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/daggers.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dagger_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dagger_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dagger_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dagger_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_knight_1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_knight_2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_knight_3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_knight_4.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_vbat_1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_vbat_2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_vbat_3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dead_zombie.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_hgc.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_hgo.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_hic.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_hio.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_hwc.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_hwo.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_vgc.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_vgo.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_vic.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_vio.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_vwc.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/door_vwo.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/drink.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dungeon_layouts.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/editor.lua $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/first_time_message.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor10.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor4.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor5.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor6.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor7.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor8.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/floor9.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/fonts.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/gem.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/graphics.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/hammer.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/hammer_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/hammer_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/hammer_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/hammer_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/haystack.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/hdoor_background.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/health0.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/health1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/health2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/health3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/health4.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/home_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/home_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/home_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/home_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_bolt.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_dagger.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_key1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_key2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_key3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_lockpicks.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/inv_overdraw.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/items.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/item_generation.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/key.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/knights_rooms.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/knights_sfont.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp1e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp1n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp1s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp1w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp2e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp2n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp2s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp2w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp3e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp3n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp3s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp3w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp4e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp4n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp4s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp4w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp5e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp5n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp5s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp5w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp6e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp6n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp6s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp6w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp7e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp7n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp7s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp7w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp8e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp8n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp8s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ktp8w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/large_table_horiz.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/large_table_vert.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/loser.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/lua_test.lua $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/main_config.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menus.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_axe.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_beartrap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_blade_trap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_centre.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_crossbow.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_dagger.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_diagonal_arrow.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_drop.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_drop_gem.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_empty.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_fist.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_highlight.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_lockpicks.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_open_close.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_pickup.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_poison_trap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menu_suicide.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/monsters.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_stand_2e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_stand_2n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_stand_2s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_stand_2w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_1e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_1n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_1s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_1w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_3e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_3n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_3s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_strike_3w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_walk_1e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_walk_1n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_walk_1s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ogre_walk_1w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/open_bear_trap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/open_chest_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/open_chest_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/open_chest_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/open_chest_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/parry.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pentagram.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pillar.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pith_c.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pith_o.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pitv_c.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pitv_o.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pit_c.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pit_o.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/poison_trap.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/potion.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/pressure_plate.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/quests.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/screech.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/scroll.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/server_config.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull4.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull_down.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull_left.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull_right.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/skull_up.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/small_skull.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/sounds.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/speech_bubble.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/squelch.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/staff.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/staff_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/staff_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/staff_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/staff_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/stairs_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/stairs_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/stairs_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/stairs_top.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/stairs_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/stuff_bag.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/switch_down.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/switch_up.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/sword_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/sword_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/sword_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/sword_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_horiz.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_small.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_vert.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/table_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/tiles.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/tutorial.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/ugh.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/vbat1.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/vbat2.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/vbat3.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/vbatbite.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/vdoor_background.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wall.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wand.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wand_east.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wand_north.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wand_south.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wand_west.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/winner.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wooden_floor.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/wooden_pit.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom1e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom1n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom1s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom1w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom2e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom2n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom2s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom2w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom3e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom3n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom3s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom3w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom4e.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom4n.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom4s.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zom4w.bmp $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zombie2.wav $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/zombie3.wav $(DATA_DIR)

install_server: $(SERVER_BINARY_NAME)
	$(INSTALL) -m 755 -d $(BIN_DIR)
	$(INSTALL) -m 755 $(SERVER_BINARY_NAME) $(BIN_DIR)
	$(INSTALL) -m 755 -d $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/anims.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/client_config.lua $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/controls.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/credits.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/dungeon_layouts.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/editor.lua $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/first_time_message.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/fonts.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/graphics.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/items.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/item_generation.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/knights_rooms.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/lua_test.lua $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/main_config.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/menus.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/monsters.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/quests.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/server_config.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/sounds.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/tiles.txt $(DATA_DIR)
	$(INSTALL) -m 644 knights_data/tutorial.txt $(DATA_DIR)

install_docs:
	$(INSTALL) -m 755 -d $(DOC_DIR)
	$(INSTALL) -m 755 -d $(DOC_DIR)/third_party_licences
	$(INSTALL) -m 755 -d $(DOC_DIR)/manual
	$(INSTALL) -m 755 -d $(DOC_DIR)/manual/images
	rm -f $(DOC_DIR)/FTL.txt
	rm -f $(DOC_DIR)/GPL.txt
	rm -f $(DOC_DIR)/LGPL.txt
	rm -f $(DOC_DIR)/quests.txt
	rm -f $(DOC_DIR)/manual.html
	$(INSTALL) -m 644 docs/ACKNOWLEDGMENTS.txt $(DOC_DIR)
	$(INSTALL) -m 644 docs/CHANGELOG.txt $(DOC_DIR)
	$(INSTALL) -m 644 docs/COPYRIGHT.txt $(DOC_DIR)
	$(INSTALL) -m 644 docs/GPL.txt $(DOC_DIR)
	$(INSTALL) -m 644 docs/README.txt $(DOC_DIR)
	$(INSTALL) -m 644 docs/style_new.css $(DOC_DIR)
	$(INSTALL) -m 644 docs/manual/building.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/controls.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/copyright.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/dungeon_features.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/index.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/intro.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/items.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/monsters.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/options.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/quest_selection.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/screen_layout.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/server.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/starting.html $(DOC_DIR)/manual
	$(INSTALL) -m 644 docs/manual/images/action_menu.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/axe.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/barrel.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/bear_trap_closed.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/bear_trap_open.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/blade_trap.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/bolts.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/book.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/chair.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/chest.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/crossbow.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/crystal_ball.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/daggers.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/entry_point.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/find_server_screen.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/game_options_1.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/gem.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/hammer.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/in_game_screen_1.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/iron_door.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/key.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/lobby.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/main_menu.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_axe.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_beartrap.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_blade_trap.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_crossbow.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_dagger.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_drop.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_drop_gem.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_fist.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_lockpicks.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_open_close.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_pickup.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_poison_trap.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/menu_suicide.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/needle_trap.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/pentagram.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/pit.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/portcullis.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/potion.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/quest_selection_1.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/scroll.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/staff.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/switch.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/vampire_bat.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/wand.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/wooden_door.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/manual/images/zombie.png $(DOC_DIR)/manual/images
	$(INSTALL) -m 644 docs/third_party_licences/FTL.txt $(DOC_DIR)/third_party_licences
	$(INSTALL) -m 644 docs/third_party_licences/LGPL.txt $(DOC_DIR)/third_party_licences
	$(INSTALL) -m 644 docs/third_party_licences/README.txt $(DOC_DIR)/third_party_licences


uninstall:
	rm -f $(BIN_DIR)/$(KNIGHTS_BINARY_NAME)
	rm -f $(BIN_DIR)/$(SERVER_BINARY_NAME)
	rm -f $(DATA_DIR)/anims.txt
	rm -f $(DATA_DIR)/axe.bmp
	rm -f $(DATA_DIR)/axe_east.bmp
	rm -f $(DATA_DIR)/axe_north.bmp
	rm -f $(DATA_DIR)/axe_south.bmp
	rm -f $(DATA_DIR)/axe_west.bmp
	rm -f $(DATA_DIR)/barrel.bmp
	rm -f $(DATA_DIR)/beartrap_east.bmp
	rm -f $(DATA_DIR)/beartrap_north.bmp
	rm -f $(DATA_DIR)/beartrap_south.bmp
	rm -f $(DATA_DIR)/beartrap_west.bmp
	rm -f $(DATA_DIR)/blade_trap.bmp
	rm -f $(DATA_DIR)/blood_1.bmp
	rm -f $(DATA_DIR)/blood_2.bmp
	rm -f $(DATA_DIR)/blood_3.bmp
	rm -f $(DATA_DIR)/blood_icon.bmp
	rm -f $(DATA_DIR)/bolts.bmp
	rm -f $(DATA_DIR)/bolt_horiz.bmp
	rm -f $(DATA_DIR)/bolt_vert.bmp
	rm -f $(DATA_DIR)/book.bmp
	rm -f $(DATA_DIR)/book_east.bmp
	rm -f $(DATA_DIR)/book_north.bmp
	rm -f $(DATA_DIR)/book_south.bmp
	rm -f $(DATA_DIR)/book_west.bmp
	rm -f $(DATA_DIR)/broken_wood_1.bmp
	rm -f $(DATA_DIR)/broken_wood_2.bmp
	rm -f $(DATA_DIR)/broken_wood_3.bmp
	rm -f $(DATA_DIR)/broken_wood_4.bmp
	rm -f $(DATA_DIR)/broken_wood_5.bmp
	rm -f $(DATA_DIR)/cage.bmp
	rm -f $(DATA_DIR)/chair_east.bmp
	rm -f $(DATA_DIR)/chair_north.bmp
	rm -f $(DATA_DIR)/chair_south.bmp
	rm -f $(DATA_DIR)/chair_west.bmp
	rm -f $(DATA_DIR)/chest_east.bmp
	rm -f $(DATA_DIR)/chest_north.bmp
	rm -f $(DATA_DIR)/chest_south.bmp
	rm -f $(DATA_DIR)/chest_west.bmp
	rm -f $(DATA_DIR)/click.wav
	rm -f $(DATA_DIR)/client_config.lua
	rm -f $(DATA_DIR)/closed_bear_trap.bmp
	rm -f $(DATA_DIR)/controls.txt
	rm -f $(DATA_DIR)/credits.txt
	rm -f $(DATA_DIR)/crossbow.bmp
	rm -f $(DATA_DIR)/crystal_ball.bmp
	rm -f $(DATA_DIR)/dagger.bmp
	rm -f $(DATA_DIR)/daggers.bmp
	rm -f $(DATA_DIR)/dagger_east.bmp
	rm -f $(DATA_DIR)/dagger_north.bmp
	rm -f $(DATA_DIR)/dagger_south.bmp
	rm -f $(DATA_DIR)/dagger_west.bmp
	rm -f $(DATA_DIR)/dead_knight_1.bmp
	rm -f $(DATA_DIR)/dead_knight_2.bmp
	rm -f $(DATA_DIR)/dead_knight_3.bmp
	rm -f $(DATA_DIR)/dead_knight_4.bmp
	rm -f $(DATA_DIR)/dead_vbat_1.bmp
	rm -f $(DATA_DIR)/dead_vbat_2.bmp
	rm -f $(DATA_DIR)/dead_vbat_3.bmp
	rm -f $(DATA_DIR)/dead_zombie.bmp
	rm -f $(DATA_DIR)/door.wav
	rm -f $(DATA_DIR)/door_hgc.bmp
	rm -f $(DATA_DIR)/door_hgo.bmp
	rm -f $(DATA_DIR)/door_hic.bmp
	rm -f $(DATA_DIR)/door_hio.bmp
	rm -f $(DATA_DIR)/door_hwc.bmp
	rm -f $(DATA_DIR)/door_hwo.bmp
	rm -f $(DATA_DIR)/door_vgc.bmp
	rm -f $(DATA_DIR)/door_vgo.bmp
	rm -f $(DATA_DIR)/door_vic.bmp
	rm -f $(DATA_DIR)/door_vio.bmp
	rm -f $(DATA_DIR)/door_vwc.bmp
	rm -f $(DATA_DIR)/door_vwo.bmp
	rm -f $(DATA_DIR)/drink.wav
	rm -f $(DATA_DIR)/dungeon_layouts.txt
	rm -f $(DATA_DIR)/editor.lua
	rm -f $(DATA_DIR)/first_time_message.txt
	rm -f $(DATA_DIR)/floor1.bmp
	rm -f $(DATA_DIR)/floor10.bmp
	rm -f $(DATA_DIR)/floor2.bmp
	rm -f $(DATA_DIR)/floor3.bmp
	rm -f $(DATA_DIR)/floor4.bmp
	rm -f $(DATA_DIR)/floor5.bmp
	rm -f $(DATA_DIR)/floor6.bmp
	rm -f $(DATA_DIR)/floor7.bmp
	rm -f $(DATA_DIR)/floor8.bmp
	rm -f $(DATA_DIR)/floor9.bmp
	rm -f $(DATA_DIR)/fonts.txt
	rm -f $(DATA_DIR)/gem.bmp
	rm -f $(DATA_DIR)/graphics.txt
	rm -f $(DATA_DIR)/hammer.bmp
	rm -f $(DATA_DIR)/hammer_east.bmp
	rm -f $(DATA_DIR)/hammer_north.bmp
	rm -f $(DATA_DIR)/hammer_south.bmp
	rm -f $(DATA_DIR)/hammer_west.bmp
	rm -f $(DATA_DIR)/haystack.bmp
	rm -f $(DATA_DIR)/hdoor_background.bmp
	rm -f $(DATA_DIR)/health0.bmp
	rm -f $(DATA_DIR)/health1.bmp
	rm -f $(DATA_DIR)/health2.bmp
	rm -f $(DATA_DIR)/health3.bmp
	rm -f $(DATA_DIR)/health4.bmp
	rm -f $(DATA_DIR)/home_east.bmp
	rm -f $(DATA_DIR)/home_north.bmp
	rm -f $(DATA_DIR)/home_south.bmp
	rm -f $(DATA_DIR)/home_west.bmp
	rm -f $(DATA_DIR)/inv_bolt.bmp
	rm -f $(DATA_DIR)/inv_dagger.bmp
	rm -f $(DATA_DIR)/inv_key1.bmp
	rm -f $(DATA_DIR)/inv_key2.bmp
	rm -f $(DATA_DIR)/inv_key3.bmp
	rm -f $(DATA_DIR)/inv_lockpicks.bmp
	rm -f $(DATA_DIR)/inv_overdraw.bmp
	rm -f $(DATA_DIR)/items.txt
	rm -f $(DATA_DIR)/item_generation.txt
	rm -f $(DATA_DIR)/key.bmp
	rm -f $(DATA_DIR)/knights_rooms.txt
	rm -f $(DATA_DIR)/knights_sfont.bmp
	rm -f $(DATA_DIR)/ktp1e.bmp
	rm -f $(DATA_DIR)/ktp1n.bmp
	rm -f $(DATA_DIR)/ktp1s.bmp
	rm -f $(DATA_DIR)/ktp1w.bmp
	rm -f $(DATA_DIR)/ktp2e.bmp
	rm -f $(DATA_DIR)/ktp2n.bmp
	rm -f $(DATA_DIR)/ktp2s.bmp
	rm -f $(DATA_DIR)/ktp2w.bmp
	rm -f $(DATA_DIR)/ktp3e.bmp
	rm -f $(DATA_DIR)/ktp3n.bmp
	rm -f $(DATA_DIR)/ktp3s.bmp
	rm -f $(DATA_DIR)/ktp3w.bmp
	rm -f $(DATA_DIR)/ktp4e.bmp
	rm -f $(DATA_DIR)/ktp4n.bmp
	rm -f $(DATA_DIR)/ktp4s.bmp
	rm -f $(DATA_DIR)/ktp4w.bmp
	rm -f $(DATA_DIR)/ktp5e.bmp
	rm -f $(DATA_DIR)/ktp5n.bmp
	rm -f $(DATA_DIR)/ktp5s.bmp
	rm -f $(DATA_DIR)/ktp5w.bmp
	rm -f $(DATA_DIR)/ktp6e.bmp
	rm -f $(DATA_DIR)/ktp6n.bmp
	rm -f $(DATA_DIR)/ktp6s.bmp
	rm -f $(DATA_DIR)/ktp6w.bmp
	rm -f $(DATA_DIR)/ktp7e.bmp
	rm -f $(DATA_DIR)/ktp7n.bmp
	rm -f $(DATA_DIR)/ktp7s.bmp
	rm -f $(DATA_DIR)/ktp7w.bmp
	rm -f $(DATA_DIR)/ktp8e.bmp
	rm -f $(DATA_DIR)/ktp8n.bmp
	rm -f $(DATA_DIR)/ktp8s.bmp
	rm -f $(DATA_DIR)/ktp8w.bmp
	rm -f $(DATA_DIR)/large_table_horiz.bmp
	rm -f $(DATA_DIR)/large_table_vert.bmp
	rm -f $(DATA_DIR)/loser.bmp
	rm -f $(DATA_DIR)/lua_test.lua
	rm -f $(DATA_DIR)/main_config.txt
	rm -f $(DATA_DIR)/menus.txt
	rm -f $(DATA_DIR)/menu_axe.bmp
	rm -f $(DATA_DIR)/menu_beartrap.bmp
	rm -f $(DATA_DIR)/menu_blade_trap.bmp
	rm -f $(DATA_DIR)/menu_centre.bmp
	rm -f $(DATA_DIR)/menu_crossbow.bmp
	rm -f $(DATA_DIR)/menu_dagger.bmp
	rm -f $(DATA_DIR)/menu_diagonal_arrow.bmp
	rm -f $(DATA_DIR)/menu_drop.bmp
	rm -f $(DATA_DIR)/menu_drop_gem.bmp
	rm -f $(DATA_DIR)/menu_empty.bmp
	rm -f $(DATA_DIR)/menu_fist.bmp
	rm -f $(DATA_DIR)/menu_highlight.bmp
	rm -f $(DATA_DIR)/menu_lockpicks.bmp
	rm -f $(DATA_DIR)/menu_open_close.bmp
	rm -f $(DATA_DIR)/menu_pickup.bmp
	rm -f $(DATA_DIR)/menu_poison_trap.bmp
	rm -f $(DATA_DIR)/menu_suicide.bmp
	rm -f $(DATA_DIR)/monsters.txt
	rm -f $(DATA_DIR)/ogre_stand_2e.bmp
	rm -f $(DATA_DIR)/ogre_stand_2n.bmp
	rm -f $(DATA_DIR)/ogre_stand_2s.bmp
	rm -f $(DATA_DIR)/ogre_stand_2w.bmp
	rm -f $(DATA_DIR)/ogre_strike_1e.bmp
	rm -f $(DATA_DIR)/ogre_strike_1n.bmp
	rm -f $(DATA_DIR)/ogre_strike_1s.bmp
	rm -f $(DATA_DIR)/ogre_strike_1w.bmp
	rm -f $(DATA_DIR)/ogre_strike_3e.bmp
	rm -f $(DATA_DIR)/ogre_strike_3n.bmp
	rm -f $(DATA_DIR)/ogre_strike_3s.bmp
	rm -f $(DATA_DIR)/ogre_strike_3w.bmp
	rm -f $(DATA_DIR)/ogre_walk_1e.bmp
	rm -f $(DATA_DIR)/ogre_walk_1n.bmp
	rm -f $(DATA_DIR)/ogre_walk_1s.bmp
	rm -f $(DATA_DIR)/ogre_walk_1w.bmp
	rm -f $(DATA_DIR)/open_bear_trap.bmp
	rm -f $(DATA_DIR)/open_chest_east.bmp
	rm -f $(DATA_DIR)/open_chest_north.bmp
	rm -f $(DATA_DIR)/open_chest_south.bmp
	rm -f $(DATA_DIR)/open_chest_west.bmp
	rm -f $(DATA_DIR)/parry.wav
	rm -f $(DATA_DIR)/pentagram.bmp
	rm -f $(DATA_DIR)/pillar.bmp
	rm -f $(DATA_DIR)/pith_c.bmp
	rm -f $(DATA_DIR)/pith_o.bmp
	rm -f $(DATA_DIR)/pitv_c.bmp
	rm -f $(DATA_DIR)/pitv_o.bmp
	rm -f $(DATA_DIR)/pit_c.bmp
	rm -f $(DATA_DIR)/pit_o.bmp
	rm -f $(DATA_DIR)/poison_trap.bmp
	rm -f $(DATA_DIR)/potion.bmp
	rm -f $(DATA_DIR)/pressure_plate.bmp
	rm -f $(DATA_DIR)/quests.txt
	rm -f $(DATA_DIR)/screech.wav
	rm -f $(DATA_DIR)/scroll.bmp
	rm -f $(DATA_DIR)/server_config.txt
	rm -f $(DATA_DIR)/skull1.bmp
	rm -f $(DATA_DIR)/skull2.bmp
	rm -f $(DATA_DIR)/skull3.bmp
	rm -f $(DATA_DIR)/skull4.bmp
	rm -f $(DATA_DIR)/skull_down.bmp
	rm -f $(DATA_DIR)/skull_left.bmp
	rm -f $(DATA_DIR)/skull_right.bmp
	rm -f $(DATA_DIR)/skull_up.bmp
	rm -f $(DATA_DIR)/small_skull.bmp
	rm -f $(DATA_DIR)/sounds.txt
	rm -f $(DATA_DIR)/speech_bubble.bmp
	rm -f $(DATA_DIR)/squelch.wav
	rm -f $(DATA_DIR)/staff.bmp
	rm -f $(DATA_DIR)/staff_east.bmp
	rm -f $(DATA_DIR)/staff_north.bmp
	rm -f $(DATA_DIR)/staff_south.bmp
	rm -f $(DATA_DIR)/staff_west.bmp
	rm -f $(DATA_DIR)/stairs_east.bmp
	rm -f $(DATA_DIR)/stairs_north.bmp
	rm -f $(DATA_DIR)/stairs_south.bmp
	rm -f $(DATA_DIR)/stairs_top.bmp
	rm -f $(DATA_DIR)/stairs_west.bmp
	rm -f $(DATA_DIR)/stuff_bag.bmp
	rm -f $(DATA_DIR)/switch_down.bmp
	rm -f $(DATA_DIR)/switch_up.bmp
	rm -f $(DATA_DIR)/sword_east.bmp
	rm -f $(DATA_DIR)/sword_north.bmp
	rm -f $(DATA_DIR)/sword_south.bmp
	rm -f $(DATA_DIR)/sword_west.bmp
	rm -f $(DATA_DIR)/table_east.bmp
	rm -f $(DATA_DIR)/table_horiz.bmp
	rm -f $(DATA_DIR)/table_north.bmp
	rm -f $(DATA_DIR)/table_small.bmp
	rm -f $(DATA_DIR)/table_south.bmp
	rm -f $(DATA_DIR)/table_vert.bmp
	rm -f $(DATA_DIR)/table_west.bmp
	rm -f $(DATA_DIR)/tiles.txt
	rm -f $(DATA_DIR)/tutorial.txt
	rm -f $(DATA_DIR)/ugh.wav
	rm -f $(DATA_DIR)/vbat1.bmp
	rm -f $(DATA_DIR)/vbat2.bmp
	rm -f $(DATA_DIR)/vbat3.bmp
	rm -f $(DATA_DIR)/vbatbite.bmp
	rm -f $(DATA_DIR)/vdoor_background.bmp
	rm -f $(DATA_DIR)/wall.bmp
	rm -f $(DATA_DIR)/wand.bmp
	rm -f $(DATA_DIR)/wand_east.bmp
	rm -f $(DATA_DIR)/wand_north.bmp
	rm -f $(DATA_DIR)/wand_south.bmp
	rm -f $(DATA_DIR)/wand_west.bmp
	rm -f $(DATA_DIR)/winner.bmp
	rm -f $(DATA_DIR)/wooden_floor.bmp
	rm -f $(DATA_DIR)/wooden_pit.bmp
	rm -f $(DATA_DIR)/zom1e.bmp
	rm -f $(DATA_DIR)/zom1n.bmp
	rm -f $(DATA_DIR)/zom1s.bmp
	rm -f $(DATA_DIR)/zom1w.bmp
	rm -f $(DATA_DIR)/zom2e.bmp
	rm -f $(DATA_DIR)/zom2n.bmp
	rm -f $(DATA_DIR)/zom2s.bmp
	rm -f $(DATA_DIR)/zom2w.bmp
	rm -f $(DATA_DIR)/zom3e.bmp
	rm -f $(DATA_DIR)/zom3n.bmp
	rm -f $(DATA_DIR)/zom3s.bmp
	rm -f $(DATA_DIR)/zom3w.bmp
	rm -f $(DATA_DIR)/zom4e.bmp
	rm -f $(DATA_DIR)/zom4n.bmp
	rm -f $(DATA_DIR)/zom4s.bmp
	rm -f $(DATA_DIR)/zom4w.bmp
	rm -f $(DATA_DIR)/zombie2.wav
	rm -f $(DATA_DIR)/zombie3.wav
	rm -f $(DOC_DIR)/ACKNOWLEDGMENTS.txt
	rm -f $(DOC_DIR)/CHANGELOG.txt
	rm -f $(DOC_DIR)/COPYRIGHT.txt
	rm -f $(DOC_DIR)/GPL.txt
	rm -f $(DOC_DIR)/README.txt
	rm -f $(DOC_DIR)/style_new.css
	rm -f $(DOC_DIR)/manual/building.html
	rm -f $(DOC_DIR)/manual/controls.html
	rm -f $(DOC_DIR)/manual/copyright.html
	rm -f $(DOC_DIR)/manual/dungeon_features.html
	rm -f $(DOC_DIR)/manual/index.html
	rm -f $(DOC_DIR)/manual/intro.html
	rm -f $(DOC_DIR)/manual/items.html
	rm -f $(DOC_DIR)/manual/monsters.html
	rm -f $(DOC_DIR)/manual/options.html
	rm -f $(DOC_DIR)/manual/quest_selection.html
	rm -f $(DOC_DIR)/manual/screen_layout.html
	rm -f $(DOC_DIR)/manual/server.html
	rm -f $(DOC_DIR)/manual/starting.html
	rm -f $(DOC_DIR)/manual/images/action_menu.png
	rm -f $(DOC_DIR)/manual/images/axe.png
	rm -f $(DOC_DIR)/manual/images/barrel.png
	rm -f $(DOC_DIR)/manual/images/bear_trap_closed.png
	rm -f $(DOC_DIR)/manual/images/bear_trap_open.png
	rm -f $(DOC_DIR)/manual/images/blade_trap.png
	rm -f $(DOC_DIR)/manual/images/bolts.png
	rm -f $(DOC_DIR)/manual/images/book.png
	rm -f $(DOC_DIR)/manual/images/chair.png
	rm -f $(DOC_DIR)/manual/images/chest.png
	rm -f $(DOC_DIR)/manual/images/crossbow.png
	rm -f $(DOC_DIR)/manual/images/crystal_ball.png
	rm -f $(DOC_DIR)/manual/images/daggers.png
	rm -f $(DOC_DIR)/manual/images/entry_point.png
	rm -f $(DOC_DIR)/manual/images/find_server_screen.png
	rm -f $(DOC_DIR)/manual/images/game_options_1.png
	rm -f $(DOC_DIR)/manual/images/gem.png
	rm -f $(DOC_DIR)/manual/images/hammer.png
	rm -f $(DOC_DIR)/manual/images/in_game_screen_1.png
	rm -f $(DOC_DIR)/manual/images/iron_door.png
	rm -f $(DOC_DIR)/manual/images/key.png
	rm -f $(DOC_DIR)/manual/images/lobby.png
	rm -f $(DOC_DIR)/manual/images/main_menu.png
	rm -f $(DOC_DIR)/manual/images/menu_axe.png
	rm -f $(DOC_DIR)/manual/images/menu_beartrap.png
	rm -f $(DOC_DIR)/manual/images/menu_blade_trap.png
	rm -f $(DOC_DIR)/manual/images/menu_crossbow.png
	rm -f $(DOC_DIR)/manual/images/menu_dagger.png
	rm -f $(DOC_DIR)/manual/images/menu_drop.png
	rm -f $(DOC_DIR)/manual/images/menu_drop_gem.png
	rm -f $(DOC_DIR)/manual/images/menu_fist.png
	rm -f $(DOC_DIR)/manual/images/menu_lockpicks.png
	rm -f $(DOC_DIR)/manual/images/menu_open_close.png
	rm -f $(DOC_DIR)/manual/images/menu_pickup.png
	rm -f $(DOC_DIR)/manual/images/menu_poison_trap.png
	rm -f $(DOC_DIR)/manual/images/menu_suicide.png
	rm -f $(DOC_DIR)/manual/images/needle_trap.png
	rm -f $(DOC_DIR)/manual/images/pentagram.png
	rm -f $(DOC_DIR)/manual/images/pit.png
	rm -f $(DOC_DIR)/manual/images/portcullis.png
	rm -f $(DOC_DIR)/manual/images/potion.png
	rm -f $(DOC_DIR)/manual/images/quest_selection_1.png
	rm -f $(DOC_DIR)/manual/images/scroll.png
	rm -f $(DOC_DIR)/manual/images/staff.png
	rm -f $(DOC_DIR)/manual/images/switch.png
	rm -f $(DOC_DIR)/manual/images/vampire_bat.png
	rm -f $(DOC_DIR)/manual/images/wand.png
	rm -f $(DOC_DIR)/manual/images/wooden_door.png
	rm -f $(DOC_DIR)/manual/images/zombie.png
	rm -f $(DOC_DIR)/third_party_licences/FTL.txt
	rm -f $(DOC_DIR)/third_party_licences/LGPL.txt
	rm -f $(DOC_DIR)/third_party_licences/README.txt

-include $(OFILES_MAIN:.o=.P) $(OFILES_SERVER:.o=.P)
