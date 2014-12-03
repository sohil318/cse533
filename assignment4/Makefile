CC = gcc

HOME=/home/stufs1/sogandhi/cse533/Stevens
ASSGN3=/home/stufs1/sogandhi/cse533/Asgn3_code

#HOME = /users/cse533/Stevens/unpv13e
#ASSGN3 = /users/cse533/Asgn3_code

LIBS = -lresolv -lnsl -lpthread\
    ${HOME}/libunp.a\

FLAGS = -g -O2
CFLAGS =  ${FLAGS} -I${HOME}/lib -I${ASSGN3}/

all: sogaship_tour sogaship_arp

sogaship_tour: tour.o get_hw_addrs.o utils.o
	${CC} -o sogaship_tour tour.o get_hw_addrs.o utils.o ${LIBS}
tour.o: tour.c
	${CC} ${CFLAGS} -c tour.c

sogaship_arp: arp.o utils.o
	${CC} -o sogaship_arp arp.o utils.o ${LIBS}
arp.o: arp.c
	${CC} ${CFLAGS} -c arp.c

utils.o: utils.c
	${CC} ${CFLAGS} -c utils.c

get_hw_addrs.o: get_hw_addrs.c
	${CC} ${CFLAGS} -c get_hw_addrs.c

clean:
	rm sogaship_tour sogaship_arp tour.o arp.o utils.o get_hw_addrs.o

