# Generates the Makefile for Knights.

# We now use "Advanced Auto-Dependency Generation" from
# http://make.paulandlesley.org/autodep.html
# This uses gcc to automatically generate the header file dependencies
# and store them in .P files.

# The generated Makefile will be printed on stdout.

import argparse
import os
import os.path
import re

PROJECTS_MAIN = ['Coercri', 'guichan', 'KnightsClient', 'KnightsEngine',
                 'KnightsLobby',
                 'KnightsMain', 'KnightsServer', 'KnightsShared', 
                 'Misc', 'RStream']


# add project path to a path and normalize
def add_proj_path(proj, p):
    p = re.sub(r"\\", r"/", p)
    p = "msvc/" + proj + "/" + p
    p = os.path.normpath(p)
    return p

# Scan a .vcproj file
# Returns (src_files, include_dirs)
def find_srcs(proj):
    incdirs = []
    srcfiles = []
    projfile = "msvc/" + proj + "/" + proj + ".vcxproj"
    L = open(projfile, "r").readlines()
    # find the include files
    for x in L:
        y = x.strip()
        if (y.startswith("<AdditionalIncludeDirectories")):
            idxstart = y.find('>') + 1
            idxstop = y.find('<', idxstart)
            incdirs = y[idxstart:idxstop].split(";")
            incdirs = [add_proj_path(proj,x) for x in incdirs]
            break
    # find the source files
    for x in L:
        y = x.strip()
        if (y.startswith("<ClCompile Include=")):
            idxstart = y.find('"') + 1
            idxstop = y.find('"', idxstart)
            srcfile = y[idxstart:idxstop]
            if (srcfile.endswith(".cpp") or srcfile.endswith(".c")):
                srcfiles.append(add_proj_path(proj, srcfile))
    return (srcfiles, incdirs)

# Turns a list of include dirs into a string of -I flags
def make_include_flags(dirs):
    result = ""
    for d in dirs:
        # filter out freetype, SDL include dirs:
        if not (d.startswith("SDL") or d.startswith("freetype")
                or d.find("$")>-1 or d.find("%")>-1):
            result += ("-I" + d + " ")
    return result

# Turn a source file name into an object file name
def get_obj_file(srcname):
    objname = re.sub(r"\.cpp", r".o", srcname)
    objname = re.sub(r"\.c", r".o", objname)
    return objname

# Get sources, with include dirs, for a given list of projects
def get_srcs_with_inc_dirs(projects):
    srcs_with_inc_dirs = []    # [(SourceName, ListOfIncludeDirs)]
    for proj in projects:
        (srcfiles, incdirs) = find_srcs(proj)
        for s in srcfiles:
            srcs_with_inc_dirs.append((s, incdirs))
    return sorted(srcs_with_inc_dirs)


# Main program

# Parse command line arguments
parser = argparse.ArgumentParser(description='Generate Makefile for Knights')
parser.add_argument('--online-platform', type=str, help='Set online platform (currently only DUMMY is supported)')
args = parser.parse_args()

# Set ONLINE_PLATFORM_FLAGS based on argument
# Note: if an online platform is set, then USE_VM_LOBBY is automatically enabled as well
if args.online_platform:
    online_platform_flags = f"-DONLINE_PLATFORM -DONLINE_PLATFORM_{args.online_platform.upper()} -DUSE_VM_LOBBY"
    online_platform_comment = "# Enable online platform support"

    # Extra projects when online platform enabled:
    PROJECTS_MAIN += ['OnlinePlatform', 'VirtualServer']

else:
    online_platform_flags = ""
    online_platform_comment = "# Support for online platforms (like Steam) is disabled in this version\n# of Knights, hence ONLINE_PLATFORM_FLAGS is empty."

# Calculate PROJECTS_ALL
PROJECTS_ALL = list(set(PROJECTS_MAIN))

# Start printing the Makefile.
print (f"""# Makefile for Knights
# (This file is generated automatically by makemakefile.py)
#
#
# HOW TO BUILD KNIGHTS:
#
# First, make sure you have all needed dependencies installed (see
# distribution specific notes below). Then you can just type "make"
# to build the knights executable.
#
#
# DISTRIBUTION SPECIFIC NOTES:
#
# Debian (and derivatives):
#
#   The following command (as root) will install the required dependencies
#   for building Knights:
#     apt install libboost-dev libboost-thread-dev libsdl2-dev \\
#       libfreetype6-dev libfontconfig-dev \\
#       liblua5.4-dev libenet-dev pkg-config
#   (Note: your distribution might have a newer Lua version available,
#   in which case it should be fine to use that instead.)
#
#
# Arch Linux:
#
#   The following command should install the required dependencies:
#     pacman -S base-devel boost boost-libs sdl2 freetype2 \\
#       fontconfig lua enet
#
#   For Arch you need to edit LUA_CFLAGS and LUA_LIBS below.
#   Simply change "lua-c++" to "lua++". (This is needed because Arch
#   uses a different package name for the Lua pkg-config files.)
#
#
# Other distributions:
#
#   Use whatever method is available on your distribution for installing
#   the required libraries: boost, SDL2, freetype, fontconfig,
#   enet, and lua.
#
#   Note that Knights requires a C++ (not C) version of Lua.
#   Some Linux distributions only provide a C version, which is not
#   suitable for Knights because it does not handle C++ exceptions
#   correctly.
#   To check, you can try "pkg-config --list-all" to see if a package
#   like "lua-c++" or "lua++" is available. If so, edit LUA_CFLAGS
#   and LUA_LIBS below to supply the correct package name for your
#   system. Otherwise, you can build a C++ version of Lua yourself,
#   as follows:
#    - Download Lua from https://lua.org/download.html
#    - Unpack the tgz file
#    - Run "make CC=g++" from within the lua-5.n.n directory
#      (this will build liblua.a in lua-5.n.n/src/)
#    - Now change LUA_CFLAGS and LUA_LIBS (in this Makefile, below)
#      to the following:
#        LUA_CFLAGS = -I/path/to/lua-5.n.n/src
#        LUA_LIBS = -L/path/to/lua-5.n.n/src -llua
#    - Now you should be able to build Knights.
#    - You can now safely delete your lua-5.n.n directory if desired
#      (the above method links Lua statically, so the Lua library
#      file is not required at runtime).
#
#
# INSTALLING:
#
# This makefile supports "make install" which by default installs Knights
# into /usr/local. "make uninstall" is also supported if you want to
# remove Knights again.
#
# To install into a different directory you can either just edit PREFIX
# below, or you can pass a modified PREFIX as an argument to make, as in:
# "make PREFIX=/some/dir", "make install PREFIX=/some/dir", and
# "make uninstall PREFIX=/some/dir" (note that the same PREFIX must be
# passed to all three make commands).
#
#
# TROUBLESHOOTING:
#
# If you run "knights" and it prints a "could not open file" error on
# startup, this is probably because the game cannot locate the
# "knights_data" directory. Try changing to the directory containing
# knights_data before running the game, or else run it as
# "knights -d /path/to/knights_data". (Note: if knights is installed
# using "make install", this won't be necessary because the path to
# knights_data is "baked into" the game in that case.)
#
# If (when building) you get Boost related link errors, try changing
# "BOOST_SUFFIX =" (below) to "BOOST_SUFFIX = -mt" instead. This will
# add a "-mt" suffix to the Boost library names (which is apparently
# needed on some distributions).
#
# If you get undefined references to "clock_gettime@@GLIBC_2.2.5" or
# similar, try adding "-lrt" to BOOST_LIBS below. (This is not a Boost
# library by the way, but BOOST_LIBS is a convenient place to put it!)
#
# If you want to disable audio, you can add -DDISABLE_SOUND to CPPFLAGS
# (below) - this might be useful if audio doesn't work on your system
# for some reason.
#


PREFIX = /usr/local
BIN_DIR = $(PREFIX)/bin
DOC_DIR = $(PREFIX)/share/doc/knights
DATA_DIR = $(PREFIX)/share/knights
BOOST_SUFFIX = 

KNIGHTS_BINARY_NAME = knights

CC = gcc
CXX = g++

LUA_CFLAGS = `pkg-config lua-c++ --cflags`
LUA_LIBS = `pkg-config lua-c++ --libs`

{online_platform_comment}
ONLINE_PLATFORM_FLAGS = {online_platform_flags}

CPPFLAGS = -DUSE_FONTCONFIG -DDATA_DIR=$(DATA_DIR) -DNDEBUG $(ONLINE_PLATFORM_FLAGS)
CFLAGS = -O2 -ffast-math -pthread
CXXFLAGS = $(CFLAGS) -std=c++20
LDFLAGS = -pthread

BOOST_LIBS = -lboost_thread$(BOOST_SUFFIX)

INSTALL = install


########################################################################

""")


# Get all source files / include dirs
srcs_with_inc_dirs_main = get_srcs_with_inc_dirs(PROJECTS_MAIN)
srcs_with_inc_dirs_all = get_srcs_with_inc_dirs(PROJECTS_ALL)

# Print the lists of object files for main program.
print ("OFILES_MAIN =", end=" ")
for (sfile, incdirs) in srcs_with_inc_dirs_main:
    print (get_obj_file(sfile), end=" ")
print()
print()

# Print "build" target
print ("""

build: $(KNIGHTS_BINARY_NAME)

""")

# Print targets for all source files.
for (srcfile, incdirs) in srcs_with_inc_dirs_all:
    incflags = make_include_flags(incdirs)

    pkgflags = "$(LUA_CFLAGS) `pkg-config libenet --cflags`"
    if srcfile.startswith("src/coercri/sdl/") or srcfile.startswith("src/main/") or srcfile.startswith("src/rstream/rstream_rwops"):
        pkgflags += " `pkg-config sdl2 --cflags`"
    if srcfile.startswith("src/coercri/gfx/"):
        pkgflags += " `pkg-config freetype2 --cflags`"
    if srcfile.startswith("src/lobby/") and args.online_platform:
        pkgflags += " `pkg-config zlib --cflags`"

    print (get_obj_file(srcfile) + ": " + srcfile)
    if srcfile.endswith(".cpp"):
        print ("\t$(CXX) $(CPPFLAGS) $(CXXFLAGS) " + pkgflags + " " + incflags + " -MD -c -o $@ $<")
    else:  # enet .c files
        print ("\t$(CC) $(CPPFLAGS) $(CFLAGS) " + incflags + " -MD -c -o $@ $<")
    print ("\t@cp $*.d $*.P; \\")
    print ("\t  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\\\$$//' \\")
    print ("\t      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \\")
    print ("\t  rm -f $*.d")

zlib_flag = ""
if args.online_platform:
    zlib_flag = "`pkg-config zlib --libs` "
pkg_link_flags_knights = "`pkg-config sdl2 --libs` `pkg-config freetype2 --libs` " + zlib_flag + "$(LUA_LIBS) `pkg-config libenet --libs`"

# Print target for Knights binary
print ("""
$(KNIGHTS_BINARY_NAME): $(OFILES_MAIN)
\t$(CXX) $(LDFLAGS) -o $@ $^ """ + pkg_link_flags_knights + """ -lfontconfig -lX11 $(BOOST_LIBS)
""")


# Print "clean" target

print ("""
clean:
\trm -f $(OFILES_MAIN)
\trm -f $(OFILES_MAIN:.o=.d)
\trm -f $(OFILES_MAIN:.o=.P)
\trm -f $(KNIGHTS_BINARY_NAME)
""")

# Print the 'install' target.
print ("""
install: install_knights install_docs

install_knights: $(KNIGHTS_BINARY_NAME)
\t$(INSTALL) -m 755 -d $(BIN_DIR)
\t$(INSTALL) -m 755 $(KNIGHTS_BINARY_NAME) $(BIN_DIR)
\t$(INSTALL) -m 755 -d $(DATA_DIR)""")

for root, dirs, files in os.walk('knights_data'):
    for f in files:
        f2 = os.path.join(root, f)
        print ("\t$(INSTALL) -m 644 -D " + f2 + " $(DATA_DIR)" + f2[12:])

for root, dirs, files in os.walk('knights_data/server'):
    for f in files:
        f2 = os.path.join(root, f)
        print ("\t$(INSTALL) -m 644 -D " + f2 + " $(DATA_DIR)" + f2[12:])

print()
print ("install_docs:")
print ("\t$(INSTALL) -m 755 -d $(DOC_DIR)")
print ("\t$(INSTALL) -m 755 -d $(DOC_DIR)/third_party_licences")
print ("\t$(INSTALL) -m 755 -d $(DOC_DIR)/manual")
print ("\t$(INSTALL) -m 755 -d $(DOC_DIR)/manual/images")

# delete some files that were present in older versions of knights
# but should be removed for this version:
print ("\trm -f $(DOC_DIR)/FTL.txt")
print ("\trm -f $(DOC_DIR)/GPL.txt")
print ("\trm -f $(DOC_DIR)/LGPL.txt")
print ("\trm -f $(DOC_DIR)/quests.txt")
print ("\trm -f $(DOC_DIR)/manual.html")

for root, dirs, files in os.walk('docs'):
    for f in files:
        if (f.endswith(".png")):  # slightly crude test but it works for now!
            print ("\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)/manual/images")
        elif (root.find("third_party_licences") != -1):
            print ("\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)/third_party_licences")
        elif (root.find("manual") != -1):
            print ("\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)/manual")
        else:
            print ("\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)")

print()

# Print the 'uninstall' target
print ("""
uninstall:
\trm -f $(BIN_DIR)/$(KNIGHTS_BINARY_NAME)""")

for root, dirs, files in os.walk('knights_data'):
    for f in files:
        f2 = os.path.join(root, f)
        print ("\trm -f $(DATA_DIR)" + f2[12:])

for root, dirs, files in os.walk('docs'):
    for f in files:
        if (f.endswith(".png")):  # see comment above
            print ("\trm -f $(DOC_DIR)/manual/images/" + os.path.basename(f))
        elif (root.find("third_party_licences") != -1):
            print ("\trm -f $(DOC_DIR)/third_party_licences/" + os.path.basename(f))
        elif (root.find("manual") != -1):
            print ("\trm -f $(DOC_DIR)/manual/" + os.path.basename(f))
        else:
            print ("\trm -f $(DOC_DIR)/" + os.path.basename(f))

print()
print ("-include $(OFILES_MAIN:.o=.P)")
