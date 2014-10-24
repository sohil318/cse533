
CC = gcc

LIBS =  -lsocket\
	/home/courses/cse533/Stevens/unpv13e_solaris2.10/libunp.a

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I/home/courses/cse533/Stevens/unpv13e_solaris2.10/lib

all: udpserv03.o 
	${CC} -o prifinfo_plus udpserv03.o ${LIBS}

udpserv03.o: udpserv03.c
	${CC} ${CFLAGS} -c udpserv03.c
clean:
	rm prifinfo_plus udpserv03.o

