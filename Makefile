CC=gcc
CFLAGS=-c
RM=rm -f

.PHONY: all clean

all: shell

shell: shell.o parser.o
	$(CC) shell.o parser.o -o shell

shell.o: shell.c shell.h parser.h
	$(CC) $(CFLAGS) shell.c

parser.o: parser.c parser.h
	$(CC) $(CFLAGS) parser.c

clean: 
	$(RM) *.o shell
