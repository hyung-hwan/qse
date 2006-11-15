#
# OpenVMS MMS/MMK
#

objects = awk.obj,err.obj,tree.obj,str.obj,tab.obj,map.obj,parse.obj run.obj rec.obj val.obj func.obj misc.obj extio.obj rex.obj

CFLAGS = /pointer_size=long /define=XP_AWK_STAND_ALONE
LIBRFLAGS = 

aseawk.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET)
	$(LIBR)/replace $(LIBRFLAGS) $(MMS$TARGET) $(objects)

awk.obj depends_on awk.c
err.obj depends_on err.c
tree.obj depends_on tree.c
str.obj depends_on str.c
tab.obj depends_on tab.c
map.obj depends_on map.c
parse.obj depends_on parse.c
run.obj depends_on run.c
rec.obj depends_on rec.c
val.obj depends_on val.c
func.obj depends_on func.c
misc.obj depends_on misc.c
extio.obj depends_on extio.c
rex.obj depends_on rex.c

