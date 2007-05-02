#
# OpenVMS MMS/MMK
#

objects = mem.obj,str.obj,misc.obj

CFLAGS = /include="../.."
#CFLAGS = /pointer_size=long /include="../.."
LIBRFLAGS = 

asecmn.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET) $(objects)
#	$(LIBR)/replace $(LIBRFLAGS) $(MMS$TARGET),$(objects)

mem.obj depends_on mem.c
str.obj depends_on str.c
misc.obj depends_on misc.c
