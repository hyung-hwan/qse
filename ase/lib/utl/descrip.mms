#
# OpenVMS MMS/MMK
#

objects = main.obj,ctype.obj,stdio.obj,http.obj,getopt.obj

CFLAGS = /include="../.."
#CFLAGS = /pointer_size=long /include="../.."
LIBRFLAGS = 

aseutl.olb : $(objects)
	$(LIBR)/create $(MMS$TARGET) $(objects)

main.obj depends_on main.c
ctype.obj depends_on ctype.c
stdio.obj depends_on stdio.c
http.obj depends_on http.c
getopt.obj depends_on getopt.c
