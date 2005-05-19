SRCS = stx.c
OBJS = stx.obj
OUT = stx.exe

CC = \tc\tcc
CFLAGS = -I..\..\.. -mh -D_DOS -w
LIBS = \tc\lib\ch.lib \tc\lib\c0h.obj ..\..\..\xp\stx\xpstx.lib

all: $(OBJS)
	\tc\tlink $(OBJS),$(OUT),,$(LIBS)

clean:
	del $(OBJS) *.obj $(OUT)

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $< 

