CC = gcc
CFLAGS = -Wall
LDFLAGS = -lpthread

all: client server

client: client.c MultModuloCommon.c
	$(CC) $(CFLAGS) -o client client.c MultModuloCommon.c $(LDFLAGS)

server: server.c MultModuloCommon.c
	$(CC) $(CFLAGS) -o server server.c MultModuloCommon.c $(LDFLAGS)

clean:
	rm -f client server