CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\..
LDFLAGS = /libpath:..\..\bas /libpath:..\..\lsp
LIBS = xpbas.lib xplsp.lib

all: lisp

lisp: lisp.obj
	link /nologo /out:lisp.exe $(LDFLAGS) $(LIBS) lisp.obj

clean:
	del $(OBJS) *.obj lisp.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

