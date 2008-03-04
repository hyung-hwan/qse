#
# OpenVMS MMS/MMK
#

objects = mem.obj,str.obj,map.obj,rex.obj,misc.obj

CFLAGS = /include="../.."
#CFLAGS = /pointer_size=long /include="../.."
LIBRFLAGS = 

asecmn.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET) $(objects)
#	$(LIBR)/replace $(LIBRFLAGS) $(MMS$TARGET),$(objects)

mem.obj depends_on mem.c
str.obj depends_on str.c
map.obj depends_on map.c
rex.obj depends_on rex.c
misc.obj depends_on misc.c
