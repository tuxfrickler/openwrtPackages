CFLAGS=-std=c99 -Wall

picburnhid: main.o
	$(CC) $(LDFLAGS) -o tmc main.o
picburnhid.o: main.c
	$(CC) $(CFLAGS) -c main.c

# remove object files and executable when user executes "make clean" 
clean:
	rm *.o tmc
