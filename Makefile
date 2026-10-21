# Makefile
CC      = gcc
CFLAGS  = -Wall -Wextra -O2

all: server client

server: src/server.c
	$(CC) $(CFLAGS) -o $@ $<

client: src/client.c
	$(CC) $(CFLAGS) -o $@ $<

clean:
	rm -f server client

.PHONY: all clean
