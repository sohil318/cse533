CC = gcc

HOME=/home/stufs1/shipgupta/cse533/Stevens/
#HOME=/home/stufs1/shipgupta/cse533/Stevens
#ASSGN2=/home/stufs1/shipgupta/cse533/Asgn2_code
ASSGN2=/home/stufs1/shipgupta/cse533/Asgn2_code

LIBS = -lresolv -lpthread\
	${HOME}/libunp.a\

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I${HOME}/lib -I${ASSGN2}/

all: get_ifi_info_plus.o utils.o
	${CC} -o interfaceTest get_ifi_info_plus.o utils.o ${LIBS}

utils.o: utils.c 
	${CC} ${CFLAGS} -c utils.c

get_ifi_info_plus.o: ${ASSGN2}/get_ifi_info_plus.c
	${CC} ${CFLAGS} -c ${ASSGN2}/get_ifi_info_plus.c

clean:
	rm interfaceTest get_ifi_info_plus.o utils.o
