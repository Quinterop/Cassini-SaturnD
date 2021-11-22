CC=gcc
CCFLAGS=-Wall

init:
	$(CC) $(CCFLAGS) src/cassini.c -o cassini 

distclean:
	rm -f cassini.o
