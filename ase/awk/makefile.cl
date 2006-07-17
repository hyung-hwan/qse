SRCS = awk.c err.c tree.c tab.c map.c parse.c run.c sa.c val.c func.c misc.c extio.c rex.c
OBJS = $(SRCS:.c=.obj)
OUT = xpawk

CC = cl
#CFLAGS = /nologo /MT /W3 /GR- /D_WIN32_WINNT=0x0400 -I../..
CFLAGS = /nologo /O2 /MT /W3 /GR- /Za /D_WIN32_WINNT=0x0400 -I../.. -DXP_AWK_STAND_ALONE -DXP_CHAR_IS_WCHAR

all: lib 

lib: $(OBJS)
	link /lib @<<
/nologo /out:$(OUT).lib $(OBJS)
<<

dll: $(OBJS)
	link /dll /def:awk.def /subsystem:console /version:0.1 /release @<<
/nologo /out:$(OUT).dll $(OBJS)
<<

clean:
	del $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) /c $<

