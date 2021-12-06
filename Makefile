CC=gcc
CCFLAGS=-Wall

init: 
	$(CC) $(CCFLAGS) src/cassini.c src/timing-text-io.c -o cassini -g

distclean:
	rm -f cassini *.o
