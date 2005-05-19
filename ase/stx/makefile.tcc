SRCS = \
	stx.c memory.c object.c symbol.c hash.c misc.c context.c
OBJS = $(SRCS:.c=.obj)
OUT = xpstx.lib

CC = c:\tc\tcc
CFLAGS = -I..\.. -D_DOS -mh -w

all: $(OBJS)
	c:\tc\tlib $(OUT) @&&!
+-$(**: = &^
+-)
!

clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $<

