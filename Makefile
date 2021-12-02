CC=gcc
CCFLAGS=-Wall

init: 
	$(CC) $(CCFLAGS) src/cassini.c src/timing-text-io.c -o cassini 

distclean:
	rm -f cassini *.o
