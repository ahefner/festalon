CC	= gcc
%.h:	%.coef
	cat $< | ./toh > $@ || true

all:            floogie ntsc.h ntsc-hi.h pal.h pal-hi.h

floogie:	toh.o
		gcc -o toh toh.o
