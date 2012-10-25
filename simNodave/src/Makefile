CFLAGS=-std=c99 -Wall -Winline -DLINUX -DDAVE_LITTLE_ENDIAN

PROGRAMS=simNodave

simNodave: simNodave.o openSocket.o nodave.o nodave.h
	$(CC) $(LDFLAGS) simNodave.o openSocket.o nodave.o -o simNodave -lpthread

nodave.o: nodave.h log2.h
openSocket.o: openSocket.h nodave.h log2.h

clean: 
	rm -f $(PROGRAMS)
	rm -f *.o
	rm -f *.so
