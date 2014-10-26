CC = gcc

#### Params for sohil sbrocks #### 
#HOME=/home/stufs1/sogandhi/cse533/Stevens
#ASSGN2=/home/stufs1/sogandhi/cse533/Asgn2_code

#### Params for compserv #### 
#HOME=/home/courses/cse533/Stevens/unpv13e_solaris2.10
#ASSGN2=/home/courses/cse533/Asgn2_code

#### Params for shipra sbrocks #### 
HOME=/home/stufs1/shipgupta/cse533/Stevens
ASSGN2=/home/stufs1/shipgupta/cse533/Asgn2_code

#LIBS = -lresolv -lnsl -lsocket -lpthread
LIBS = -lresolv -lnsl -lpthread\
	${HOME}/libunp.a\

FLAGS =  -g -O2
CFLAGS = ${FLAGS} -I${HOME}/lib -I${ASSGN2}/

all: server client

server: server.o utils.o get_ifi_info_plus.o
	${CC} -o server server.o get_ifi_info_plus.o utils.o ${LIBS}
server.o: server.c
	${CC} ${CFLAGS} -c server.c

client: client.o utils.o get_ifi_info_plus.o
	${CC} -o client client.o get_ifi_info_plus.o utils.o ${LIBS}
client.o: client.c
	${CC} ${CFLAGS} -c client.c

utils.o: utils.c 
	${CC} ${CFLAGS} -c utils.c

get_ifi_info_plus.o: ${ASSGN2}/get_ifi_info_plus.c
	${CC} ${CFLAGS} -c ${ASSGN2}/get_ifi_info_plus.c

clean:
	rm client server server.o get_ifi_info_plus.o utils.o client.o

