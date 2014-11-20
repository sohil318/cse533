
CC = gcc

HOME = /users/cse533/Stevens/unpv13e
ASSGN3 = /users/cse533/Asgn3_code

LIBS = -lresolv -lnsl -lpthread\
    ${HOME}/libunp.a\

#HOME =  /home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2
CFLAGS =  ${FLAGS} -I${HOME}/lib -I${ASSGN3}/

all: odr client server

odr: odr.o get_hw_addrs.o utils.o
	${CC} -o odr odr.o get_hw_addrs.o utils.o ${LIBS}
odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

client: client.o utils.o
	${CC} -o client client.o utils.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c

server: server.o utils.o
	${CC} -o server server.o utils.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c

utils.o: utils.c
	${CC} ${CFLAGS} -c utils.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm odr client server odr.o utils.o get_hw_addrs.o client.o server.o

