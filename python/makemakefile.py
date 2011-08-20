#!/usr/bin/python

# Generates the Makefile for Knights.

# We now use "Advanced Auto-Dependency Generation" from
# http://make.paulandlesley.org/autodep.html
# This uses gcc to automatically generate the header file dependencies
# and store them in .P files.


import os
import os.path
import re

PROJECTS_MAIN = ['Coercri', 'ENet', 'guichan', 'KConfig', 'KnightsClient', 'KnightsEngine', 'KnightsMain',
                'KnightsServer', 'KnightsShared', 'lua', 'Misc', 'RStream']
PROJECTS_SERVER = ['ENet', 'KConfig', 'KnightsEngine', 'KnightsServer', 'KnightsShared',
                   'KnightsSvrMain', 'lua', 'Misc', 'RStream']
EXTRA_OBJS_SERVER = ['src/coercri/network/byte_buf.o', 
                     'src/coercri/enet/enet_network_driver.o',
                     'src/coercri/enet/enet_network_connection.o',
                     'src/coercri/enet/enet_udp_socket.o',
                     'src/coercri/sdl/core/sdl_error.o',
                     'src/coercri/sdl/core/sdl_subsystem_handle.o',
                     'src/coercri/sdl/timer/sdl_timer.o']

PROJECTS_ALL = list(set(PROJECTS_MAIN + PROJECTS_SERVER))


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
    projfile = "msvc/" + proj + "/" + proj + ".vcproj"
    L = file(projfile, "r").readlines()
    # find the include files
    for x in L:
        y = x.strip()
        if (y.startswith("AdditionalIncludeDirectories")):
            idxstart = y.find('"') + 1
            idxstop = y.find('"', idxstart)
            incdirs = y[idxstart:idxstop].split(";")
            incdirs = map(lambda x: add_proj_path(proj,x), incdirs)
            break
    # find the source files
    for x in L:
        y = x.strip()
        if (y.startswith("RelativePath=")):
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
        if not (d.startswith("SDL") or d.startswith("freetype")):
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
    return srcs_with_inc_dirs


# Main program

# Start printing the Makefile.
print """# Makefile for Knights
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

"""


# Get all source files / include dirs
srcs_with_inc_dirs_main = get_srcs_with_inc_dirs(PROJECTS_MAIN)
srcs_with_inc_dirs_server = get_srcs_with_inc_dirs(PROJECTS_SERVER)
srcs_with_inc_dirs_all = get_srcs_with_inc_dirs(PROJECTS_ALL)

# Print the lists of object files for main program & server program.
print "OFILES_MAIN =",
for (sfile, incdirs) in srcs_with_inc_dirs_main:
    print get_obj_file(sfile),
print
print

print "OFILES_SERVER =",
for (sfile, incdirs) in srcs_with_inc_dirs_server:
    print get_obj_file(sfile),
for sfile in EXTRA_OBJS_SERVER:
    print sfile,
print
print

# Print "build" target
print """

build: $(KNIGHTS_BINARY_NAME) $(SERVER_BINARY_NAME)

"""

# Print targets for all source files.
for (srcfile, incdirs) in srcs_with_inc_dirs_all:
    incflags = make_include_flags(incdirs)
    print get_obj_file(srcfile) + ": " + srcfile
    if (srcfile.endswith(".cpp")):
        print "\t$(CXX) $(CPPFLAGS) $(CXXFLAGS) `sdl-config --cflags` `curl-config --cflags` " + incflags + " -MD -c -o $@ $<"
    elif (srcfile.endswith("SDL_ttf.c")):
        print "\t$(CC) $(CPPFLAGS) $(CFLAGS) `sdl-config --cflags` `freetype-config --cflags` " + incflags + " -MD -c -o $@ $<"
    else:  # enet .c files
        print "\t$(CC) $(CPPFLAGS) $(CFLAGS) " + incflags + " -MD -c -o $@ $<"
    print "\t@cp $*.d $*.P; \\"
    print "\t  sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\\\$$//' \\"
    print "\t      -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \\"
    print "\t  rm -f $*.d"

# Print target for Knights binary
print """
$(KNIGHTS_BINARY_NAME): $(OFILES_MAIN)
\t$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ `sdl-config --libs` `freetype-config --libs` `curl-config --libs` -lfontconfig $(BOOST_LIBS)
\t$(STRIP) $@
"""

# Print target for Server binary
print """
$(SERVER_BINARY_NAME): $(OFILES_SERVER)
\t$(CXX) $(CPPFLAGS) $(CXXFLAGS) -o $@ $^ `sdl-config --libs` `curl-config --libs` $(BOOST_LIBS)
\t$(STRIP) $@
"""


# Print "clean" target

print """
clean:
\trm -f $(OFILES_MAIN)
\trm -f $(OFILES_MAIN:.o=.d)
\trm -f $(OFILES_MAIN:.o=.P)
\trm -f $(OFILES_SERVER)
\trm -f $(OFILES_SERVER:.o=.d)
\trm -f $(OFILES_SERVER:.o=.P)
\trm -f $(KNIGHTS_BINARY_NAME)
\trm -f $(SERVER_BINARY_NAME)
"""

# Print the 'install' target.
print """
install: install_knights install_server install_docs

install_knights: $(KNIGHTS_BINARY_NAME)
\t$(INSTALL) -m 755 -d $(BIN_DIR)
\t$(INSTALL) -m 755 $(KNIGHTS_BINARY_NAME) $(BIN_DIR)
\t$(INSTALL) -m 755 -d $(DATA_DIR)"""

for root, dirs, files in os.walk('knights_data'):
    if root.find(".svn") == -1:
        for f in files:
            print "\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DATA_DIR)"

print """
install_server: $(SERVER_BINARY_NAME)
\t$(INSTALL) -m 755 -d $(BIN_DIR)
\t$(INSTALL) -m 755 $(SERVER_BINARY_NAME) $(BIN_DIR)
\t$(INSTALL) -m 755 -d $(DATA_DIR)"""

for root, dirs, files in os.walk('knights_data'):
    if root.find(".svn") == -1:
        for f in files:
            if (f.endswith(".txt") or f.endswith(".lua")):
                print "\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DATA_DIR)"

print
print "install_docs:"
print "\t$(INSTALL) -m 755 -d $(DOC_DIR)"
print "\t$(INSTALL) -m 755 -d $(DOC_DIR)/third_party_licences"
print "\t$(INSTALL) -m 755 -d $(DOC_DIR)/manual"
print "\t$(INSTALL) -m 755 -d $(DOC_DIR)/manual/images"

# delete some files that were present in older versions of knights
# but should be removed for this version:
print "\trm -f $(DOC_DIR)/FTL.txt"
print "\trm -f $(DOC_DIR)/GPL.txt"
print "\trm -f $(DOC_DIR)/LGPL.txt"
print "\trm -f $(DOC_DIR)/quests.txt"
print "\trm -f $(DOC_DIR)/manual.html"

for root, dirs, files in os.walk('docs'):
    if root.find(".svn") == -1:
        for f in files:
            if (f.endswith(".png")):  # slightly crude test but it works for now!
                print "\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)/manual/images"
            elif (root.find("third_party_licences") != -1):
                print "\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)/third_party_licences"
            elif (root.find("manual") != -1):
                print "\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)/manual"
            else:
                print "\t$(INSTALL) -m 644 " + os.path.join(root, f) + " $(DOC_DIR)"

print

# Print the 'uninstall' target
print """
uninstall:
\trm -f $(BIN_DIR)/$(KNIGHTS_BINARY_NAME)
\trm -f $(BIN_DIR)/$(SERVER_BINARY_NAME)"""

for root, dirs, files in os.walk('knights_data'):
    if root.find(".svn") == -1:
        for f in files:
            print "\trm -f $(DATA_DIR)/" + os.path.basename(f)

for root, dirs, files in os.walk('docs'):
    if root.find(".svn") == -1:
        for f in files:
            if (f.endswith(".png")):  # see comment above
                print "\trm -f $(DOC_DIR)/manual/images/" + os.path.basename(f)
            elif (root.find("third_party_licences") != -1):
                print "\trm -f $(DOC_DIR)/third_party_licences/" + os.path.basename(f)
            elif (root.find("manual") != -1):
                print "\trm -f $(DOC_DIR)/manual/" + os.path.basename(f)
            else:
                print "\trm -f $(DOC_DIR)/" + os.path.basename(f)

print
print "-include $(OFILES_MAIN:.o=.P) $(OFILES_SERVER:.o=.P)"
