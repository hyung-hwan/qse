CC = tcc
CFLAGS = -1 -O -mh -w -f87 -I..\..\.. 
LDFLAGS = -L..\..\awk
LIBS = aseawk.lib 

all: awk 

awk: awk.obj
	$(CC) $(LDFLAGS) -mh -eawk.exe awk.obj $(LIBS)

clean:
	del $(OBJS) *.obj $(OUT)

.c.obj:
	$(CC) $(CFLAGS) -c $< 

