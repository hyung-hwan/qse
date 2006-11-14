OUT = aseawk

C_SRCS = awk.c err.c tree.c str.c tab.c map.c parse.c \
	run.c rec.c val.c func.c misc.c extio.c rex.c
JNI_SRCS = $(C_SRCS) jni.c
JAVA_SRCS = Awk.java Exception.java Extio.java

C_OBJS = $(C_SRCS:.c=.obj)
JNI_OBJS = $(JNI_SRCS:.c=.obj)
JAVA_OBJS = $(JAVA_SRCS:.java=.class)

JNI_INC = \
	/I"C:\Program Files\Java\jdk1.5.0_09\include" \
	/I"C:\Program Files\Java\jdk1.5.0_09\include\win32" 

CC = cl
LD = link
JAVAC = javac

CFLAGS = /nologo /O2 /MT /W3 /GR- /Za -I../.. $(JNI_INC) 
JAVACFLAGS = -classpath ../..

all: lib jni

lib: $(C_OBJS)
	$(LD) /lib @<<
/nologo /out:$(OUT).lib $(C_OBJS)
<<

jni: $(JNI_OBJS) $(JAVA_OBJS) 
	$(LD) /dll /def:jni.def /subsystem:windows /version:0.1 /release @<<
/nologo /out:$(OUT).dll $(JNI_OBJS)
<<

clean:
	del $(OBJS) $(OUT).lib $(OUT).dll *.obj *.class

.SUFFIXES: .c .obj .java .class
.c.obj:
	$(CC) $(CFLAGS) /c $<

.java.class:
	$(JAVAC) $(JAVACFLAGS) $<
