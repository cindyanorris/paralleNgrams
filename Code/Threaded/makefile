CFLAGS = -g -c
CC = gcc 
.C.o:
	$(CC) $(CFLAGS) $< -o $@

OBJS = threadedNgram.o parseArgument.o usage.o hashTable.o handleBook.o threadPool.o

threadedNgram: $(OBJS)
	$(CC) $(OBJS) -o threadedNgram -lm -pthread

threadPool.o: threadPool.h

threadedNgram.o: parseArgument.h usage.h hashTable.h handleBook.h threadPool.h

parseArgument.o: parseArgument.h usage.h

handleBook.o: handleBook.h usage.h

usage.o: usage.h

hashTable.o: hashTable.h

clean:
	rm *.o 

