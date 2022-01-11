CC=gcc
CCFLAGS=-Wall
.PHONY: all cassini saturnd

all: cassini saturnd

cassini: 
	$(CC) $(CCFLAGS) src/cassini.c src/timing-text-io.c -o cassini 
	
saturnd: 
	$(CC) $(CCFLAGS) src/saturnd.c src/timing-text-io.c -o saturnd 

distclean:
	rm -f cassini saturnd *.o
