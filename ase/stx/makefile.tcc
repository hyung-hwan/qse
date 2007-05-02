SRCS = \
	stx.c memory.c object.c symbol.c dict.c misc.c context.c
OBJS = $(SRCS:.c=.obj)
OUT = xpstx.lib

TC = \dos\tcc
CC = $(TC)\tcc
CFLAGS = -I..\.. -D_DOS -ml -w

all: $(OBJS)
	$(TC)\tlib $(OUT) @&&!
+-$(**: = &^
+-)
!

clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $<

