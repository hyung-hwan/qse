CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\.. -I$(XPKIT)
LDFLAGS = /libpath:..\..\lsp /libpath:$(XPKIT)\xp\bas
LIBS = aselsp.lib xpbas.lib user32.lib

all: lsp

lsp: lsp.obj
	link /nologo /out:lsp.exe $(LDFLAGS) $(LIBS) lsp.obj

clean:
	del $(OBJS) *.obj lsp.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

