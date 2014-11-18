CC = gcc -g -Wall -lrt -lpthread
C_SOURCES = $(wildcard *.c)
OBJECTS = $(C_SOURCES:.c=.o)
debug = -g

all: server

debug: CC += -g
debug: all

server: ${OBJECTS}
	${CC} -o $@ $^

%.o: %.c
	${CC} -c -o $@ $^

clean:
	rm *.o

