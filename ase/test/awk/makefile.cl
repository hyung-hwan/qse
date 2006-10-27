CC = cl
CFLAGS = /nologo /MT /W3 /GR- -I..\..\.. -D_WIN32_WINNT=0x0400 
LDFLAGS = /libpath:..\..\awk
LIBS = aseawk.lib kernel32.lib user32.lib

all: awk #rex2 rex3

awk: awk.obj
	link /nologo /out:awk.exe $(LDFLAGS) $(LIBS) awk.obj

rex: rex.obj
	link /nologo /out:rex.exe $(LDFLAGS) $(LIBS) rex.obj

rex2: rex2.obj
	link /nologo /out:rex2.exe $(LDFLAGS) $(LIBS) rex2.obj

rex3: rex3.obj
	link /nologo /out:rex3.exe $(LDFLAGS) $(LIBS) rex3.obj

java: 
	javac -classpath ../../.. Awk.java

jrun:
	java -classpath ../../.. ase.test.awk.Awk

clean:
	del $(OBJS) *.obj awk.exe rex.exe

.SUFFIXES: .c .obj
.c.obj:
	$(CC) /c $(CFLAGS) $< 

