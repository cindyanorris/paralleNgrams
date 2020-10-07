CFLAGS = -g -c
CC = gcc 
.C.o:
	$(CC) $(CFLAGS) $< -o $@

OBJS = serialNgram.o parseArgument.o usage.o hashTable.o parseBook.o

serialNgram: $(OBJS)
	$(CC) $(OBJS) -o serialNgram -lm	

serialNgram.o: parseArgument.h usage.h hashTable.h parseBook.h

parseArgument.o: parseArgument.h usage.h

parseBook.o: parseBook.h usage.h

usage.o: usage.h

hashTable.o: hashTable.h

clean:
	rm *.o 

