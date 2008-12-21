#
# OpenVMS MMS/MMK
#

objects = lsp.obj name.obj mem.obj env.obj err.obj read.obj eval.obj print.obj misc.obj prim.obj prim_prog.obj prim_let.obj prim_compar.obj prim_math.obj

CFLAGS = /include="../.."
#CFLAGS = /pointer_size=long /include="../.."

aselsp.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET) *.obj
#	$(LIBR)/create $(MMS$TARGET) $(objects)

lsp.obj depends_on lsp.c
name.obj depends_on name.c
mem.obj depends_on mem.c
env.obj depends_on env.c
err.obj depends_on err.c
read.obj depends_on read.c
eval.obj depends_on eval.c
print.obj depends_on print.c
misc.obj depends_on misc.c
prim.obj depends_on prim.c
prim_prog.obj depends_on prim_prog.c
prim_let.obj depends_on prim_let.c
prim_compar.obj depends_on prim_compar.c
prim_math.obj depends_on prim_math.c

