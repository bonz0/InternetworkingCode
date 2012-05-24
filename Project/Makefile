CC = gcc
CFLAGS = -o
FUNC = functions.c
HEADER = header.h
all:
	$(CC) server.c $(FUNC) $(CFLAGS) ftps
	$(CC) client.c $(FUNC) $(CFLAGS) ftpc
	$(CC) tcpd_client.c $(FUNC) $(CFLAGS) tcpd_client
	$(CC) tcpd_server.c $(FUNC) $(CFLAGS) tcpd_server
clean:
	rm -rf ftp*
	rm -rf tcpd_client
	rm -rf tcpd_server
	rm -rf n3WF1l3*
