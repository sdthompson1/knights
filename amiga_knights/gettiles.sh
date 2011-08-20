# List of tiles (and other gfx) not generated by these scripts:
#
# hdoor_background.bmp, vdoor_background.bmp
# home_*.bmp (need to add red "securing" rectangle)
# door_??c.bmp, door_?io.bmp, door_?wo.bmp (need to black out background)
#
# menu_*.bmp
# inv_*.bmp
# health*.bmp
# skull?.bmp
# knights_sfont.bmp

convert -crop 16x16+16+0 kicons.ppm wall.bmp
convert -crop 16x16+32+0 kicons.ppm pillar.bmp
convert -crop 16x16+48+0 kicons.ppm skull_right.bmp
convert -crop 16x16+64+0 kicons.ppm skull_left.bmp
convert -crop 16x16+80+0 kicons.ppm cage.bmp
convert -crop 16x16+96+0 kicons.ppm door_hwc.bmp
convert -crop 16x16+112+0 kicons.ppm door_vwc.bmp
convert -crop 16x16+128+0 kicons.ppm door_hic.bmp
convert -crop 16x16+144+0 kicons.ppm door_vic.bmp
convert -crop 16x16+160+0 kicons.ppm home_south.bmp
convert -crop 16x16+176+0 kicons.ppm home_west.bmp
convert -crop 16x16+192+0 kicons.ppm home_north.bmp
convert -crop 16x16+208+0 kicons.ppm home_east.bmp
convert -crop 16x16+224+0 kicons.ppm crystal_ball.bmp
convert -crop 16x16+240+0 kicons.ppm door_hgc.bmp
convert -crop 16x16+256+0 kicons.ppm door_vgc.bmp
convert -crop 16x16+272+0 kicons.ppm switch_up.bmp
convert -crop 16x16+288+0 kicons.ppm switch_down.bmp

convert -crop 16x16+32+16 kicons.ppm haystack.bmp
convert -crop 16x16+48+16 kicons.ppm barrel.bmp
convert -crop 16x16+64+16 kicons.ppm chest_north.bmp
convert -crop 16x16+80+16 kicons.ppm chest_east.bmp
convert -crop 16x16+96+16 kicons.ppm chest_south.bmp
convert -crop 16x16+112+16 kicons.ppm chest_west.bmp
convert -crop 16x16+176+16 kicons.ppm small_skull.bmp
convert -crop 16x16+224+16 kicons.ppm open_chest_north.bmp
convert -crop 16x16+240+16 kicons.ppm open_chest_east.bmp
convert -crop 16x16+256+16 kicons.ppm open_chest_south.bmp
convert -crop 16x16+272+16 kicons.ppm open_chest_west.bmp
convert -crop 16x16+288+16 kicons.ppm table_small.bmp
convert -crop 16x16+304+16 kicons.ppm table_north.bmp

convert -crop 16x16+0+32 kicons.ppm table_vert.bmp
convert -crop 16x16+16+32 kicons.ppm table_south.bmp
convert -crop 16x16+32+32 kicons.ppm table_horiz.bmp
convert -crop 16x16+48+32 kicons.ppm chair_south.bmp
convert -crop 16x16+64+32 kicons.ppm chair_north.bmp
convert -crop 16x16+96+32 kicons.ppm pitv_o.bmp
convert -crop 16x16+112+32 kicons.ppm wooden_pit.bmp
convert -crop 16x16+128+32 kicons.ppm pit_o.bmp
convert -crop 16x16+144+32 kicons.ppm door_hwo.bmp
convert -crop 16x16+160+32 kicons.ppm door_vwo.bmp
convert -crop 16x16+176+32 kicons.ppm door_hio.bmp
convert -crop 16x16+192+32 kicons.ppm door_vio.bmp
convert -crop 16x16+208+32 kicons.ppm broken_wood_1.bmp
convert -crop 16x16+224+32 kicons.ppm broken_wood_2.bmp
convert -crop 16x16+240+32 kicons.ppm broken_wood_3.bmp
convert -crop 16x16+256+32 kicons.ppm broken_wood_4.bmp
convert -crop 16x16+272+32 kicons.ppm broken_wood_5.bmp
convert -crop 16x16+288+32 kicons.ppm dead_zombie.bmp
convert -crop 16x16+304+32 kicons.ppm dead_knight_1.bmp

convert -crop 16x16+0+48 kicons.ppm dead_knight_2.bmp
convert -crop 16x16+16+48 kicons.ppm dead_knight_3.bmp
convert -crop 16x16+32+48 kicons.ppm dead_knight_4.bmp
convert -crop 16x16+48+48 kicons.ppm door_hgo.bmp
convert -crop 16x16+64+48 kicons.ppm door_vgo.bmp
convert -crop 16x16+80+48 kicons.ppm floor1.bmp
convert -crop 16x16+96+48 kicons.ppm pressure_plate.bmp
convert -crop 16x16+112+48 kicons.ppm floor2.bmp
convert -crop 16x16+128+48 kicons.ppm floor3.bmp
convert -crop 16x16+144+48 kicons.ppm floor4.bmp
convert -crop 16x16+160+48 kicons.ppm floor5.bmp
convert -crop 16x16+176+48 kicons.ppm floor6.bmp
convert -crop 16x16+192+48 kicons.ppm floor7.bmp
convert -crop 16x16+208+48 kicons.ppm floor8.bmp
convert -crop 16x16+224+48 kicons.ppm floor9.bmp
convert -crop 16x16+240+48 kicons.ppm pentagram.bmp
convert -crop 16x16+256+48 kicons.ppm floor10.bmp
convert -crop 16x16+272+48 kicons.ppm pitv_c.bmp
convert -crop 16x16+288+48 kicons.ppm wooden_floor.bmp
convert -crop 16x16+304+48 kicons.ppm pit_c.bmp

convert -crop 16x16+0+64 kicons.ppm stairs_top.bmp
convert -crop 16x16+16+64 kicons.ppm stairs_south.bmp
convert -crop 16x16+32+64 kicons.ppm stairs_west.bmp
convert -crop 16x16+48+64 kicons.ppm stairs_north.bmp
convert -crop 16x16+64+64 kicons.ppm stairs_east.bmp
