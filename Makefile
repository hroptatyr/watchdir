RM = rm -f
CC = icc -static-intel -static-libgcc -std=c99
CFLAGS = -g -O3

all: watchdir

clean:
	$(RM) watchdir *.o watchdir-clo.c watchdir-clo.h

watchdir: watchdir-clo.c watchdir.o
	$(CC) $(CFLAGS) -o $@ $@.o

.SUFFIXES: .c .o .h .ggo

.c.o:
	$(CC) $(CFLAGS) -c -o $@ $< -DVERSION='"0.0"'

.ggo.c:
	gengetopt -F $* -i $<
