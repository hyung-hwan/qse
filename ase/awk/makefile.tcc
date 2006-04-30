SRCS = awk.c err.c tree.c tab.c map.c parse.c run.c sa.c val.c misc.c
OBJS = $(SRCS:.c=.obj)
OUT = xpawk.lib

CC = tcc
CFLAGS = -1 -O -mh -I..\.. -Ddos -DXP_AWK_STAND_ALONE

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

