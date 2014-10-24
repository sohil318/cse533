
CC = gcc

LIBS = /home/stufs1/sogandhi/cse533/Stevens/libunp.a

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I/home/stufs1/sogandhi/cse533/Stevens/lib

all: udpserv03.o 
	${CC} -o prifinfo_plus udpserv03.o ${LIBS}

udpserv03.o: udpserv03.c
	${CC} ${CFLAGS} -c udpserv03.c
clean:
	rm prifinfo_plus udpserv03.o

