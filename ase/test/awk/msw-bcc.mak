
# ilink32.exe     link.exe
# -aa             /subsystem:windows
# -ap             /subsystem:console
# -ad             /subsystem:native
#
# -Tpe            
# -Tpd            /dll

!ifndef MODE
MODE = release
!endif

CC = bcc32
CXX = bcc32
LD = ilink32
CFLAGS = -O2 -WM -WU -RT- -w -q -I..\..\.. 
CXXFLAGS = -O2 -WM -WU -RT- -w -q -I..\..\.. 
LDFLAGS = -ap -Tpe -Gn -c -q -L..\..\awk -L..\..\cmn -L..\..\utl
LIBS = import32.lib cw32mt.lib aseawk.lib asecmn.lib aseutl.lib
STARTUP = c0x32w.obj

all: aseawk 

aseawk: awk.obj
	$(LD) $(LDFLAGS) $(STARTUP) awk.obj,$@.exe,,$(LIBS),,

java: 
	javac -classpath ../../.. Awk.java AwkApplet.java

jrun:
	java -Xms1m -Xmx2m -classpath ../../.. ase.test.awk.Awk 

clean:
	del $(OBJS) *.obj *.class aseawk.exe 

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $< 

