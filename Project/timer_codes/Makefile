# Makefile for project with timer

CC = gcc
OBJCLI = ftpc.c
OBJSRV = ftps.c
OBJTCPD_CLI = tcpd_client.c
OBJTCPD_SRV = tcpd_server.c
OBJTIMER = timerprocess.c
CFLAGS = -Wall -c
LFLAGS = -Wall

all : ftpc ftps tcpd_client tcpd_server functions.o timerfunctions.o timerprocess

ftpc : $(OBJCLI) functions.o
	$(CC) -o $@ $(LFLAGS) $(OBJCLI) functions.o

ftps : $(OBJSRV) functions.o
	$(CC) -o $@ $(LFLAGS) $(OBJSRV) functions.o

tcpd_client : $(OBJTCPD_CLI) functions.o
	$(CC) -o $@ $(LFLAGS) $(OBJTCPD_CLI) functions.o

tcpd_server : $(OBJTCPD_SRV) functions.o
	$(CC) -o $@ $(LFLAGS) $(OBJTCPD_SRV) functions.o

functions.o : functions.c tcpd.h
	$(CC) $(CFLAGS) functions.c

timerfunctions.o : timerfunctions.c timer.h
	$(CC) $(CFLAGS) timerfunctions.c

timerprocess : $(OBJTIMER) timerfunctions.o
	$(CC) -o $@ $(LFLAGS) $(OBJTIMER) timerfunctions.o

clean :
	\rm *.o ftpc ftps tcpd_client tcpd_server timerprocess


