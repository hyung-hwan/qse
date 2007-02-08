CC = cc
CFLAGS = -Xc -a ansi -O2 -I../../.. 
LDFLAGS = -L../../bas -L../../awk
LIBS = -lxpawk -lm

all: aseawk

aseawk: awk.o
	$(CC) -o awk awk.o $(LDFLAGS) $(LIBS)

clean:
	rm -f *.o aseawk

.SUFFIXES: .c .o
.c.o:
	$(CC) -c $(CFLAGS) $< 

