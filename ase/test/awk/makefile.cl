CC = cl
#CFLAGS = /nologo /MT /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\..
CFLAGS = /nologo /MT /W3 /GR- /D_WIN32_WINNT=0x0400 -I..\..\.. -D__STAND_ALONE
LDFLAGS = /libpath:..\..\bas /libpath:..\..\awk
LIBS = xpawk.lib user32.lib

all: awk rex2 rex3

awk: awk.obj
	link /nologo /out:awk.exe $(LDFLAGS) $(LIBS) awk.obj

rex: rex.obj
	link /nologo /out:rex.exe $(LDFLAGS) $(LIBS) rex.obj

rex2: rex2.obj
	link /nologo /out:rex2.exe $(LDFLAGS) $(LIBS) rex2.obj

rex3: rex3.obj
	link /nologo /out:rex3.exe $(LDFLAGS) $(LIBS) rex3.obj


clean:
	del $(OBJS) *.obj awk.exe rex.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

