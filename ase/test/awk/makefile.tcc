CC = tcc
CFLAGS = -1 -O -mh -w -f87 -I..\..\.. -Ddos -D__STAND_ALONE
LDFLAGS = -L..\..\awk
LIBS = xpawk.lib 

all: awk 

awk: awk.obj
	$(CC) $(LDFLAGS) -mh -eawk.exe awk.obj $(LIBS)

clean:
	del $(OBJS) *.obj $(OUT)

.c.obj:
	$(CC) $(CFLAGS) -c $< 

