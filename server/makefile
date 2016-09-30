CC = gcc -g -std=gnu99 -Wall -Wextra -pedantic -lrt -lpthread
C_SOURCES = $(wildcard *.c)
OBJECTS = $(C_SOURCES:.c=.o)
debug = -g

EXE = server

all: ${EXE}

debug: CC += -g
debug: all

${EXE}: ${OBJECTS}
	${CC} -o $@ $^

%.o: %.c
	${CC} -c -o $@ $^

clean:
	rm *.o
	rm ${EXE}

