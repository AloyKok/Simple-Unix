shell: shell.o parser.o
	gcc shell.o parser.o -o shell

shell.o: shell.c shell.h parser.h
	gcc -c shell.c

parser.o: parser.c parser.h
	gcc -c parser.c

clean: 
	 rm *.o