objects = awk.obj,err.obj,extio.obj,func.obj,map.obj,misc.obj,parse.obj,rex.obj,run.obj,rec.obj,sa.obj,tab.obj,tree.obj,val.obj

CFLAGS = /pointer_size=long /define=XP_AWK_STAND_ALONE
LIBRFLAGS = 

sseawk.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET)
	$(LIBR)/replace $(LIBRFLAGS) $(MMS$TARGET) $(objects)

awk.obj depends_on awk.c
err.obj depends_on err.c
extio.obj depends_on extio.c
func.obj depends_on func.c
map.obj depends_on map.c
misc.obj depends_on misc.c
parse.obj depends_on parse.c
rex.obj depends_on rex.c
run.obj depends_on run.c
rec.obj depends_on rec.c
sa.obj depends_on sa.c
tab.obj depends_on tab.c
tree.obj depends_on tree.c
val.obj depends_on val.c

