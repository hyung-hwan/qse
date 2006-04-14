CC = cc
CFLAGS = -Xc -a ansi -O2 -I../../..
LDFLAGS = -L../../bas -L../../awk
LIBS = -lxpawk -lm

all: awk

awk: awk.o
	$(CC) -o awk awk.o $(LDFLAGS) $(LIBS)

clean:
	rm -f *.obj awk

.SUFFIXES: .c .o
.c.o:
	$(CC) -c $(CFLAGS) $< 

