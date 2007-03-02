OUT = aseawk

C_SRCS = awk.c err.c tree.c str.c tab.c map.c parse.c \
	run.c rec.c val.c func.c misc.c extio.c rex.c
JNI_SRCS = jni.c
JAVA_SRCS = Exception.java Extio.java Awk.java StdAwk.java

C_OBJS = $(C_SRCS:.c=.obj)
JNI_OBJS = $(JNI_SRCS:.c=.obj)
JAVA_OBJS = $(JAVA_SRCS:.java=.class)

JNI_INC = \
	/I"$(JAVA_HOME)/include" \
	/I"$(JAVA_HOME)/include\win32" 

CC = cl
LD = link
JAVAC = javac

#CFLAGS = /nologo /O2 /MT /W3 /GR- /Za -I../.. $(JNI_INC) 
CFLAGS = /nologo /O2 /MT /W3 /GR- -I../.. $(JNI_INC) 
JAVACFLAGS = -classpath ../.. -Xlint:unchecked

all: lib 

lib: $(C_OBJS)
	$(LD) /lib @<<
/nologo /out:$(OUT).lib $(C_OBJS)
<<

jni: lib $(JNI_OBJS) $(JAVA_OBJS) 
	$(LD) /dll /def:jni.def /subsystem:windows /version:0.1 /release @<<
/nologo /out:$(OUT)_jni.dll $(JNI_OBJS) /implib:tmp.lib user32.lib $(OUT).lib
<<
	del tmp.lib tmp.exp

clean:
	del $(OBJS) $(OUT).lib $(OUT)_jni.dll *.obj *.class

.SUFFIXES: .c .obj .java .class
.c.obj:
	$(CC) $(CFLAGS) /c $<

.java.class:
	$(JAVAC) $(JAVACFLAGS) $<
