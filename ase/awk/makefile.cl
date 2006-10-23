SRCS = \
	awk.c err.c tree.c str.c tab.c map.c parse.c \
	run.c rec.c val.c func.c misc.c extio.c rex.c
OBJS = $(SRCS:.c=.obj)
OUT = sseawk

JAVA_INC = \
	/I"C:\Program Files\IBM\Java141\Include" \
	/I"C:\Program Files\IBM\Java141\Include\Win32"

CC = cl
LD = link
CFLAGS = /nologo /O2 /MT /W3 /GR- /Za -I../.. -DSSE_CHAR_IS_WCHAR $(JAVA_INC) 

all: lib 

lib: $(OBJS)
	$(LD) /lib @<<
/nologo /out:$(OUT).lib $(OBJS)
<<

dll: $(OBJS)
	$(LD) /dll /def:awk.def /subsystem:console /version:0.1 /release @<<
/nologo /out:$(OUT).dll $(OBJS)
<<

jni: $(OBJS) jni.obj
	$(LD) /dll /def:jni.def /subsystem:console /version:0.1 /release @<<
/nologo /out:$(OUT).dll $(OBJS) jni.obj ..\bas\xpbas.lib
<<

clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) /c $<

