CC = gcc -Wall -lrt -lpthread
C_SOURCES = $(wildcard *.c)
OBJECTS = $(C_SOURCES:.c=.o)
debug = -g
EXE = client

all: ${EXE}

debug: CC += -g
debug: all

${EXE}: ${OBJECTS}
	${CC} -lm -o $@ $^

%.o: %.c
	${CC} -c -o $@ $^

clean:
	rm *.o
	rm ${EXE}

