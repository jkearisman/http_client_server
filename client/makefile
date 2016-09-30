CC = gcc -Wall -lrt -lpthread
C_SOURCES = $(wildcard *.c)
OBJECTS = $(C_SOURCES:.c=.o)
debug = -g

all: client

debug: CC += -g
debug: all

client: ${OBJECTS}
	${CC} -lm -o $@ $^

%.o: %.c
	${CC} -c -o $@ $^

clean:
	rm *.o

