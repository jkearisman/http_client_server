CC = gcc -Wall -lrt
debug = -g

all: server client clean

debug: CC += -g
debug: all

server: server.o common.o
	${CC} -o server server.o common.o

client: client.o common.o
	${CC} -lm -o client client.o common.o

client.o:
	${CC} -c -o client.o client.c

server.o:
	${CC} -c -o server.o server.c

common.o:
	${CC} -c -o common.o common.c

clean:
	rm *.o

