CC = gcc
CFLAGS = -Wall -g

all: server sendFile

server: server.c
	$(CC) $(CFLAGS) -o server server.c

sendFile: client.c
	$(CC) $(CFLAGS) -o sendFile client.c

clean:
	rm -f server sendFile *.o