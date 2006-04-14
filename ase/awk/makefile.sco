SRCS = awk.c err.c tree.c tab.c map.c parse.c run.c sa.c val.c misc.c
OBJS = $(SRCS:.c=.o)
OUT = libxpawk.a

CC = cc
CFLAGS = -Xc -a ansi -w3 -O2 -I../.. -D__STAND_ALONE

all: $(OBJS)
	ar cr $(OUT) $(OBJS)

clean:
	rm -rf $(OBJS) $(OUT) *.obj

.SUFFIXES: .c .o
.c.o:
	$(CC) $(CFLAGS) -c $<

