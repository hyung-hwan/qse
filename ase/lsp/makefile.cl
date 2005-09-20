SRCS = name.c token.c array.c mem.c env.c error.c \
	init.c read.c eval.c print.c \
	prim.c prim_prog.c prim_let.c
OBJS = $(SRCS:.c=.obj)
OUT = xplsp.lib

CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- /D_WIN32_WINNT=0x0400 -I../..

all: $(OBJS)
	link -lib @<<
/nologo /out:$(OUT) $(OBJS)
<<


clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) /c $<

