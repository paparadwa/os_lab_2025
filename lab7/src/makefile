CC = gcc

all: tcpclient udpclient tcpserver udpserver

tcpclient: tcpclient.c
	$(CC) -o $@ $<

udpclient: udpclient.c
	$(CC) -o $@ $<

tcpserver: tcpserver.c
	$(CC) -o $@ $<

udpserver: udpserver.c
	$(CC) -o $@ $<

clean:
	rm -f tcpclient udpclient tcpserver udpserver