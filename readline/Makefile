LIBS=-lreadline
all: shell.o
	gcc -o shell shell.o $(LIBS)
	make clean

shell.o: shell.c shell.h
	gcc -c -g -Wall shell.c

clean:
	rm *.o

run:
	./shell

