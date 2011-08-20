# Convert from "getitems.sh" script to a Kconfig file for the items.

input = file("getitems.sh", "r").readlines()

for i in input:
	s = i.split()

	width = int((s[4].split('x'))[0])
	height = int((s[4].split('x'))[1])
	filename = s[6]
	rootname = (filename.split('.'))[0]

	print 'g_'+rootname+' = ["' + filename + '",' + str(-(32-width)/2) + ',' + str(-(32-height)/2) + ',0,0,0]'

	
