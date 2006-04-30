CC = tcc
CFLAGS = -1 -O -mh -I..\..\.. -Ddos -D__STAND_ALONE
LDFLAGS = -L..\..\awk
LIBS = xpawk.lib 

all: awk 

awk: awk.obj
	tcc $(LDFLAGS) -mh -eawk.exe awk.obj $(LIBS)

clean:
	del $(OBJS) *.obj $(OUT)

.SUFFIXES: .c .obj
.c.obj:
	$(CC) $(CFLAGS) -c $< 

