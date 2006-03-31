CC = cl
#CFLAGS = /nologo /MT /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\..
CFLAGS = /nologo /MT /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\..
LDFLAGS = /libpath:..\..\bas /libpath:..\..\awk
LIBS = xpbas.lib xpawk.lib
#LIBS = xpawk.lib

all: awk

awk: awk.obj
	link /nologo /out:awk.exe $(LDFLAGS) $(LIBS) awk.obj

clean:
	del $(OBJS) *.obj awk.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

