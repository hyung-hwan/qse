CC = cl
CFLAGS = /nologo /MT /GX /W3 /GR- -I..\..\.. 
LDFLAGS = /libpath:..\..\cmn /libpath:..\..\lsp /libpath:..\..\utl
LIBS = asecmn.lib aselsp.lib aseutl.lib user32.lib

all: aselsp

aselsp: lsp.obj
	link /nologo /out:$@.exe $(LDFLAGS) $(LIBS) lsp.obj

clean:
	del $(OBJS) *.obj lsp.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

