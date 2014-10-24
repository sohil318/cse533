CC = gcc

HOME=/home/courses/cse533/Stevens/unpv13e_solaris2.10
#HOME=/home/courses/cse533/Stevens/unpv13e_solaris2.10
ASSGN2=/home/courses/cse533/Asgn2_code

LIBS = -lresolv -lnsl -lsocket -lpthread\
	${HOME}/libunp.a\

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I${HOME}/lib -I${ASSGN2}/

all: get_ifi_info_plus.o udpserv03.o
	${CC} -o interfaceTest get_ifi_info_plus.o udpserv03.o ${LIBS}

udpserv03.o: udpserv03.c 
	${CC} ${CFLAGS} -c udpserv03.c

get_ifi_info_plus.o: ${ASSGN2}/get_ifi_info_plus.c
	${CC} ${CFLAGS} -c ${ASSGN2}/get_ifi_info_plus.c

#prifinfo_plus.o: ${ASSGN2}/prifinfo_plus.c
#	${CC} ${CFLAGS} -c ${ASSGN2}/prifinfo_plus.c

clean:
	rm interfaceTest get_ifi_info_plus.o udpserv03.o
