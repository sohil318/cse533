
CC = gcc

HOME = /users/cse533/Stevens/unpv13e
ASSGN3 = /users/cse533/Asgn3_code

LIBS = -lresolv -lnsl -lpthread\
    ${HOME}/libunp.a\

#HOME =  /home/courses/cse533/Stevens/unpv13e/libunp.a

FLAGS = -g -O2
CFLAGS =  ${FLAGS} -I${HOME}/lib -I${ASSGN3}/

all: odr

odr: odr.o get_hw_addrs.o 
	${CC} -o odr odr.o get_hw_addrs.o ${LIBS}
odr.o: odr.c
	${CC} ${CFLAGS} -c odr.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${FLAGS} -c get_hw_addrs.c

clean:
	rm odr odr.o get_hw_addrs.o

