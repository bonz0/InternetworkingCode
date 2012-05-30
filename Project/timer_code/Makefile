# Makefile for project with timer

CC = gcc
OBJTIMER = timerprocess.c
CFLAGS = -Wall -c
LFLAGS = -Wall

all : timerfunctions.o timerprocess

functions.o : functions.c tcpd.h
	$(CC) $(CFLAGS) functions.c

timerfunctions.o : timerfunctions.c timer.h
	$(CC) $(CFLAGS) timerfunctions.c

timerprocess : $(OBJTIMER) timerfunctions.o
	$(CC) -o $@ $(LFLAGS) $(OBJTIMER) timerfunctions.o

clean :
	rm -rf timerprocess
