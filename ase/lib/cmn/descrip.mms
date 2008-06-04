#
# OpenVMS MMS/MMK
#

objects = mem.obj,str_bas.obj,str_cnv.obj,str_dyn.obj,map.obj,rex.obj,misc.obj

CFLAGS = /include="../.."
#CFLAGS = /pointer_size=long /include="../.."
LIBRFLAGS = 

asecmn.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET) $(objects)
#	$(LIBR)/replace $(LIBRFLAGS) $(MMS$TARGET),$(objects)

mem.obj depends_on mem.c
str_bas.obj depends_on str_bas.c
str_cnv.obj depends_on str_cnv.c
str_dyn.obj depends_on str_dyn.c
map.obj depends_on map.c
rex.obj depends_on rex.c
misc.obj depends_on misc.c
