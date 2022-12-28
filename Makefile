CC=gcc
CFLAGS=-O
OBJS=main.o smallsh.o
TARGET=smallsh

$(TARGET) : $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

main.o: smallsh.h main.c
smallsh.o: smallsh.h smallsh.c

clean:
	rm -f *.o smallsh
