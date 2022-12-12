CC = gcc
CFLAGS = -Wall -O3 -ggdb

all:
	$(CC) $(CFLAGS) main.c pp.c -o willem3

clean:
	rm -f willem3

.PHONY:	all clean
