SRCS = awk.c err.c tree.c str.c tab.c map.c parse.c run.c val.c misc.c extio.c rex.c
OBJS = $(SRCS:.c=.obj)
OUT = xpawk.lib

CC = tcc
CFLAGS = -1 -O -mh -w -f87 -G -I..\.. -Ddos -DXP_AWK_STAND_ALONE

all: $(OBJS)
	tlib $(OUT) @&&!
+-$(**: = &^
+-)
!

clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $<

