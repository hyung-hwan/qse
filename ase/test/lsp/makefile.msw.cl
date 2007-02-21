CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\.. 
LDFLAGS = /libpath:..\..\lsp /libpath:..\..\utl
LIBS = aselsp.lib aseutl.lib user32.lib

all: aselsp

aselsp: lsp.obj
	link /nologo /out:$@.exe $(LDFLAGS) $(LIBS) lsp.obj

clean:
	del $(OBJS) *.obj lsp.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

