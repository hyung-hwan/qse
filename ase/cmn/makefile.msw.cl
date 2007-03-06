OUT = asecmn

C_SRCS =  mem.c str.c misc.c
C_OBJS = $(C_SRCS:.c=.obj)

CC = cl
LD = link

CFLAGS = /nologo /O2 /MT /W3 /GR- /Za -I../.. -DNDEBUG

all: lib 

lib: $(C_OBJS)
	$(LD) /lib @<<
/nologo /out:$(OUT).lib $(C_OBJS)
<<

clean:
	del $(OBJS) $(OUT).lib *.obj 

.SUFFIXES: .c .obj 
.c.obj:
	$(CC) $(CFLAGS) /c $<
