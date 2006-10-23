SRCS = lsp.c name.c token.c array.c mem.c env.c err.c \
	read.c eval.c print.c \
	prim.c prim_prog.c prim_let.c prim_compar.c prim_math.c
OBJS = $(SRCS:.c=.obj)
OUT = xplsp.lib

CC = cl
LD = link
CFLAGS = /nologo /O2 /MT /W3 /GR- /Za -I../.. -DSSE_CHAR_IS_WCHAR

all: $(OBJS)
	$(LD) -lib @<<
/nologo /out:$(OUT) $(OBJS)
<<

clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) /c $<

