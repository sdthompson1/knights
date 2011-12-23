#!/usr/bin/python

# Convert Knights KRD file into Kconfig format.

import sys


PRE = """# Room data, imported from KRD.
# This file was automatically generated by kconvert.py.

include "tiles.txt"

knights_rooms = {

    tiles = [

# These tile numbers are mostly the same as in the original Amiga
# Knights (there are one or two changes). 
    
        t_live_pentagram    # 1
        t_wall_normal       # 2
        t_wall_pillar       # 3
        t_wall_skull_east   # 4
        t_wall_skull_west   # 5
        t_wall_cage         # 6
        [t_door_horiz, t_hdoor_background]        # 7
        [t_door_vert, t_vdoor_background]         # 8     
        [t_iron_door_horiz, t_hdoor_background]   # 9
        [t_iron_door_vert, t_vdoor_background]    # 10
        t_home_south        # 11
        t_home_west         # 12
        t_home_north        # 13
        t_home_east         # 14
        t_crystal_ball      # 15
        t_gate_horiz        # 16
        t_gate_vert         # 17
        t_switch_up         # 18
        t_switch_down       # 19
        0              # 20
        0              # 21
        0              # 22
        t_haystack          # 23
        t_barrel            # 24
        t_chest_north       # 25
        t_chest_east        # 26
        t_chest_south       # 27
        t_chest_west        # 28
        0              # 29
        0              # 30
        0              # 31
        t_small_skull       # 32
        0              # 33
        0              # 34
        0              # 35 
        0              # 36 
        0              # 37 
        0              # 38 
        t_table_small       # 39
        t_table_north       # 40
        t_table_vert        # 41
        t_table_south       # 42
        t_large_table_horiz # 43
        t_chair_south       # 44
        t_chair_north       # 45
        0              # 46
        t_open_pit_vert     # 47
        t_open_pit_wooden   # 48
        t_open_pit_normal   # 49
        [t_large_table_horiz, 0, i_basic_book]        # 50 
        [t_large_table_horiz, 0, i_necronomicon]      # 51 
        0              # 52 
        0              # 53 
        t_broken_wood_1     # 54
        t_broken_wood_2     # 55
        t_broken_wood_3     # 56
        t_broken_wood_4     # 57
        t_broken_wood_5     # 58
        [t_dead_zombie, t_floor1]       # 59
        0              # 60
        0              # 61
        0              # 62
        0              # 63
        t_open_gate_horiz   # 64
        t_open_gate_vert    # 65
        t_floor1 & {editor_label="-"}            # 66
        t_floorpp           # 67
        t_floor2            # 68
        t_floor3            # 69
        t_floor4            # 70
        t_floor5            # 71
        [t_floor6, t_floor1]            # 72
        t_floor7            # 73
        t_floor8            # 74
        t_floor9            # 75
        t_dead_pentagram    # 76
        t_floor10           # 77
        t_closed_pit_vert   # 78
        t_closed_pit_wooden # 79
        t_closed_pit_normal # 80
        t_stairs_top        # 81
        t_stairs_south      # 82
        t_stairs_west       # 83
        t_stairs_north      # 84
        t_stairs_east       # 85
        t_special_pentagram     # 86
        [t_door_horiz_locked, t_hdoor_background] # 87
        [t_door_vert_locked, t_vdoor_background] # 88
        [t_iron_door_horiz_locked, t_hdoor_background] # 89
        [t_iron_door_vert_locked, t_vdoor_background], # 90
    ]

    segments = [

"""

POST="""

]  # close 'rooms'
}  # close 'knights_rooms'
"""

facing=["'north'","'east'","'south'","'west'"]

def readbyte(f):
    return ord(f.read(1))

def gtrap(x, y, mjr, mnr):
    if mjr>=201 and mjr<=204:
        return ('shoot', x, y, facing[mjr-201] + ',i_bolt_trap')
    elif mjr == 220:
        return ('teleport_actor', x, y)
    else:
        lo = min(mjr,mnr)
        hi = max(mjr,mnr)
        if (lo>=7 and lo<=10 and hi==lo+43) or (lo==16 and hi==64) \
        or (lo==17 and hi==65) or ((lo==47 or lo==49) and hi==lo+31):
            # door or pit toggle
            return ('toggle', x, y)
        elif ((hi >= 50 and hi<=53) or hi==64 or hi==65) and lo==hi:
            # door open
            return ('open', x, y)
        elif (hi==78 or hi==80) and lo==hi:
            # pit close
            return ('close', x, y)
        elif (hi==47 or hi==49) and lo==hi:
            # pit open
            return ('open', x, y)
        elif (hi==169 and lo==76):
            # pentagram toggle
            return ('toggle_no_sound', x, y)
        elif (hi==15 and lo==3):
            # crystal ball toggle
            return ('toggle', x, y)
        elif hi==15 and lo==15:
            # crystal ball open
            return ('open', x, y)
        elif hi==3 and lo==3:
            # crystal ball close
            return ('close', x, y)
        elif hi==2 and lo==2:
            return None
        else:
            print "bad trap"
            print x, y, mjr, mnr
            sys.exit(1)

def dowrite(f, t, xb, yb):
    f.write(t[0] + '(' + str(t[1]-xb) + ',' + str(t[2]-yb))
    if len(t)>3:
        f.write(','+t[3])
    f.write(')')

def writetrap(f, tdat, xb, yb):
    t2 = []
    for t in tdat:
        if t != None:
            t2.append(t)

    if len(t2)==0: raise "Trap with no effect?"

    f.write('"')
    if len(t2)==1:
        dowrite(f, t2[0], xb, yb)
    else:
        for i in range(len(t2)):
            dowrite(f, t2[i], xb, yb)
            if i != len(t2)-1:
                f.write(' ')
    f.write('"')

def matchtrap(tdat,x,y):
    for t in tdat:
        if t != None:
            if t[1]==x and t[2]==y: return True
    return False


# Main program.

# Check arguments
if len(sys.argv) != 3:
    print "usage:", sys.argv[0], "<infile> <outfile>"
    sys.exit(1)

# Open files
f = file(sys.argv[1], 'rb')
outfile = file(sys.argv[2], 'w')
outfile.write(PRE+"\n")

# Skip AMOS header
# (we assume there is only one bank in the file)
f.read(20)


# Load numbers of rooms
na = readbyte(f)
nb = readbyte(f)
nc = readbyte(f)
nb = nb - nc
na = na - nb - nc
nx = readbyte(f)
nz = readbyte(f)
nrooms = na+nb+nc+nx+nz


# Load all rooms
for a in range(nrooms):
    mpp = a*1080+5 +20
    if a<nc:
        letter='C'
        nroom=a+1
    elif a<nb+nc:
        letter='B'
        nroom=a+1-nc
    elif a<na+nb+nc:
        letter='A'
        nroom=a+1-nc-nb
    elif a<na+nb+nc+nx:
        letter='X'
        nroom=a+1-nc-nb-na
    else:
        letter='Z'
        nroom=a+1-na-nb-nc-nx

    print letter+str(nroom)

    # Read layout information
    f.seek(mpp + 14*14*5)
    rums = readbyte(f)
    rumdata = []
    rumcheck = [[]]*14
    for i in range(14):
        rumcheck[i] = [0]*14
    f.seek(mpp + 14*14*5 + 4)
    for i in range(rums):
        x = readbyte(f)
        y = readbyte(f)
        sx = readbyte(f)
        sy = readbyte(f)
        rumdata.append([x,y,sx,sy])
        for xx in range(x, x+sx):
            for yy in range(y, y+sy):
                if (xx == x or xx == x+sx-1) and (yy == y or yy == y+sy-1): 
                    continue
                rumcheck[xx][yy] = rumcheck[xx][yy]+1
                if rumcheck[xx][yy] > 2:
                    print "ROOMS ERROR, more than 2 rooms at", xx, ",", yy
                    print rumdata
                    sys.exit(1)

    # Load traps table
    tx = []
    ty = []
    tmjr = []
    tmnr = []
    tad1 = []
    tad2 = []
    tdata = []
    
    while 1:
        trap_base = mpp + 14*14*5 + 52 + len(tx)*6
        f.seek(trap_base)
        x = readbyte(f)
        y = readbyte(f)
        mjr = readbyte(f)
        mnr = readbyte(f)
        ad1 = readbyte(f)
        ad2 = readbyte(f)

        if mjr == 0: break

        tx.append(x)
        ty.append(y)
        tmjr.append(mjr)
        tmnr.append(mnr)
        tad1.append(ad1)
        tad2.append(ad2)
        if len(tx)==8: break

    # Get traps data
    if len(tx)>0:
        for i in range(len(tx)):
            tdat=[]
            tdat.append(gtrap(tx[i], ty[i], tmjr[i], tmnr[i]))
            if tad1[i] > 0:
                j = tad1[i]-1
                tdat.append(gtrap(tx[j], ty[j], tmjr[j], tmnr[j]))
            if tad2[i] > 0: 
                j = tad2[i]-1
                tdat.append(gtrap(tx[j], ty[j], tmjr[j], tmnr[j]))
            tdata.append(tdat)

    # Read rooms themselves
    for nrot in range(4):
        bdd = nrot*14*14

        # work out type
        if letter=='A' or (letter=='X' and nrot>=2) or letter=='B' or letter=='C':
            roomtype = None
        elif (letter=='Z' and nrot==0) or (letter=='X' and nrot<=1):
            roomtype = 'guarded_exit'
        elif (letter=='Z' and nrot==1):
            roomtype = 'liche_tomb'
        elif (letter=='Z' and nrot==2):
            roomtype = 'gnome_room'
        elif (letter=='Z' and nrot==3):
            roomtype = 'special_pentagram'
        else:
            raise "Unknown room type"

        # Guarded exit room Z09.1 does not currently work because the game assumes there
        # is only one entry-point in a guarded exit segment, but this segment has two
        # entry-points. So we don't include this segment.
        if letter == 'Z' and nroom == 9 and nrot==0:
            continue

        # Write room header
        outfile.write('{\n')
        outfile.write(' width  = 12\n')
        outfile.write(' height = 12\n')
        if (roomtype != None):
            outfile.write(' category = "' + roomtype + '"\n')
            if roomtype == 'guarded_exit':
                outfile.write(' bat_placement_tile = 73\n')
        outfile.write(' name = "%s%02d.%d"\n' % (letter,nroom,nrot+1))
        outfile.write(' data = [\n')
        
        # Read the map itself
        sx = []
        sy = []
        sn = []
        f.seek(mpp+bdd)
        r = []
        
        for y in range(14):
            for x in range(14):
                t = readbyte(f)  # tile number
                tr = 0
                if t>=191 and t<=199:
                    tr = t-190
                    t = 19 # switch down
                elif t>=181 and t<=189:
                    tr = t-180
                    t = 18 # switch up
                elif t>=170 and t<=179:
                    tr = t-170
                    t = 67 # pressure plate
                elif t>=160 and t<=168:
                    tr = t-160
                    t = 76 # pentagram
                elif t==169:
                    t = 1  # Live pentagram

                if (t == 1 or t == 76) and roomtype == "special_pentagram":
                    t = 86 #  Special pentagram

                if letter == "Z" and nroom == 3 and nrot == 0 and x == 12 and y == 10:
                    # This particular tile should be replaced with 81 (stair-top)
                    # to prevent the dungeon generator creating doors into the guarded exit area.
                    if t != 59:
                        print "Unexpected tile in room Z3:1"
                        print "Found:", t, "Expected:", 59
                        sys.exit(1)
                    t = 81
                        
                if t < 1 or (t > 91):
                    print "bad tile"
                    print x, y, t
                    raise "bad tile"

                if x==0 or x==13 or y==0 or y==13:
                    if t<>2:
                        raise "non wall tile on border"
                else:
                    if t == 43:
                        # horiz table -- may need gnome book or necronomicon
                        if roomtype == 'gnome_room':
                            t = 50
                        elif roomtype == 'liche_tomb':
                            t = 51

                r.append(t)

                if tr<>0:
                    sx.append(x)
                    sy.append(y)
                    sn.append(tr)
                    
        # (end of for loops over x,y)

        # Replace inaccessible iron doors with walls:
        def isok(x,y): 
            t = r[y*14+x]
            return x==0 or x==13 or y==0 or y==13 or t==9 or t==10 or t==2 or t==3

        for y in range(1,13):
            for x in range(1,13):
                result = \
                    isok(x-1, y-1) and \
                    isok(x-1, y  ) and \
                    isok(x-1, y+1) and \
                    isok(x,   y-1) and \
                    isok(x,   y  ) and \
                    isok(x,   y+1) and \
                    isok(x+1, y-1) and \
                    isok(x+1, y  ) and \
                    isok(x+1, y+1)
                if result: r[y*14+x] = 2

        # Now write the output
        for y in range(1,13):
            for x in range(1,13):
                tile = r[y*14 + x]

                if tile >= 7 and tile <= 10:
                    # Wood or iron door.
                    # These become "traplocked" if any switch targets this tile.
                    for i in range(len(sx)):
                        if matchtrap(tdata[sn[i]-1], x, y):
                            tile = tile + 80
                            break
                
                outfile.write(str(tile))
                outfile.write(" ")
            outfile.write("\n")
        outfile.write(" ]\n")

        # Do switches
        if (len(sx)>0):
            outfile.write(' switches = [\n')
            for i in range(len(sx)):
                x=sx[i]
                y=sy[i]
                tr=sn[i]
                tile = r[y*14+x]
                outfile.write('   ['+str(x-1)+' '+str(y-1)+' ')
                writetrap(outfile, tdata[tr-1], x, y)
                outfile.write(']\n')
            outfile.write(' ]\n')

        # Rooms data
        outfile.write(' rooms = ' + str(rumdata) + '\n')

        # The end of the room
        outfile.write('}\n\n')

outfile.write(POST+"\n")
outfile.close()
print 'wrote', nrooms, '* 4 rooms'
print na,nb,nc,nx,nz

