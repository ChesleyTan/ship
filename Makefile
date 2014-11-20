all: shell.o
	gcc -o shell shell.o
	make clean
	make run

shell.o: shell.c
	gcc -c shell.c

clean:
	rm *.o

run:
	./shell
