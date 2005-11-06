SRCS = awk.c lex.c parse.c 
OBJS = $(SRCS:.c=.obj)
OUT = xpawk.lib

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

