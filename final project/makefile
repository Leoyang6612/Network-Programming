SHELL = /bin/bash
CC = gcc
CFLAGS = -g
LDFLAGS = -lcrypto
SRC = $(wildcard *.c)
EXE = server client

all: ${EXE}

server: server.o user_mngmt.o map_func.o
	${CC} $^ ${LDFLAGS} -o $@

server.o: server.c
	${CC} ${CFLAGS} -c $< -o $@

client: client.o user_mngmt.o map_func.o
	${CC} $^ ${LDFLAGS} -o $@

client.o: client.c
	${CC} ${CFLAGS} -c $< -o $@

user_mngmt.o: user_mngmt.c
	${CC} ${CFLAGS} -c $< -o $@

map_func.o: map_func.c
	${CC} ${CFLAGS} -c $< -o $@

clean:
	rm *.o ${EXE}

