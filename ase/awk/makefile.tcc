#
# makefile for turbo c 2.0 
#

SRCS = awk.c err.c tree.c str.c tab.c map.c parse.c \
	run.c rec.c val.c misc.c extio.c rex.c
OBJS = awk.obj err.obj tree.obj str.obj tab.obj map.obj parse.obj \
	run.obj rec.obj val.obj misc.obj extio.obj rex.obj
OUT = sseawk.lib

CC = tcc
AR = tlib
CFLAGS = -1 -O -mh -w -f87 -G -I..\.. -Ddos 

all: $(OBJS)
	-del $(OUT)
	$(AR) $(OUT) +awk.obj
	$(AR) $(OUT) +err.obj
	$(AR) $(OUT) +tree.obj
	$(AR) $(OUT) +str.obj
	$(AR) $(OUT) +tab.obj
	$(AR) $(OUT) +map.obj
	$(AR) $(OUT) +parse.obj
	$(AR) $(OUT) +run.obj
	$(AR) $(OUT) +rec.obj
	$(AR) $(OUT) +val.obj
	$(AR) $(OUT) +misc.obj
	$(AR) $(OUT) +extio.obj
	$(AR) $(OUT) +rex.obj

clean:
	-del *.obj
	-del $(OUT)

.c.obj:
	$(CC) $(CFLAGS) -c $<

