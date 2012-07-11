Source code taken from other places. At the moment we have:

enet -- A cross-platform networking library for games, by Lee Salzman
(enet.bespin.org). Currently I am using enet 1.2.5, with custom
modifications to enet.h and protocol.c (to add an "averagePing" field
to ENetPeer).

guichan -- A GUI library by Olof Naessén and Per Larsson
(guichan.sourceforge.net).

Lua -- A scripting language (www.lua.org). I have included a subset of
the files from the Lua 5.2.1 release. I have also modified Lua to
accept '&' as a table merging operator. The following files were
changed:
  lcode.c
  lcode.h
  lopcodes.c
  lopcodes.h
  lparser.c
  luac.c
  lvm.c
