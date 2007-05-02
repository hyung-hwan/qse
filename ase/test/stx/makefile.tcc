SRCS = stx.c
OBJS = stx.obj
OUT = stx.exe

TC = \dos\tc
CC = $(TC)\tcc
CFLAGS = -I..\..\.. -ml -D_DOS -w
LIBS = $(TC)\lib\cl.lib $(TC)\lib\c0l.obj ..\..\..\xp\stx\xpstx.lib

all: $(OBJS)
	$(TC)\tlink $(OBJS),$(OUT),,$(LIBS)

clean:
	del $(OBJS) *.obj $(OUT)

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $< 

