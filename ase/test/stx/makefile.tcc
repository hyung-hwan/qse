SRCS = stx.c
OBJS = stx.obj
OUT = stx.exe

CC = \tc\tcc
CFLAGS = -I..\..\.. -ml -D_DOS -w
LIBS = \tc\lib\cl.lib \tc\lib\c0l.obj ..\..\..\xp\stx\xpstx.lib

all: $(OBJS)
	\tc\tlink $(OBJS),$(OUT),,$(LIBS)

clean:
	del $(OBJS) *.obj $(OUT)

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $< 

