CC = cl
CFLAGS = /nologo /MT /W3 /GR- -I..\..\.. -D_WIN32_WINNT=0x0400 
LDFLAGS = /libpath:..\..\cmn /libpath:..\..\awk /libpath:..\..\utl
LIBS = asecmn.lib aseawk.lib aseutl.lib kernel32.lib user32.lib

!if !defined(CPU) || "$(CPU)" == ""
CPU = $(PROCESSOR_ARCHITECTURE)
!endif 

!if "$(CPU)" == ""
CPU = i386
!endif

!if "$(CPU)" == "IA64" || "$(CPU)" == "AMD64"
LIBS = $(LIBS) bufferoverflowu.lib
!endif

all: aseawk

aseawk: awk.obj
	link /nologo /out:$@.exe $(LDFLAGS) $(LIBS) awk.obj

mini: mini.obj
	link /nologo /out:$@.exe $(LDFLAGS) $(LIBS) mini.obj

java: 
	javac -classpath ../../.. Awk.java

jrun:
	java -classpath ../../.. ase.test.awk.Awk

clean:
	del $(OBJS) *.obj aseawk.exe mini.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

