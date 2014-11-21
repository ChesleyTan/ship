all: shell.o
	gcc -o shell shell.o
	make clean

shell.o: shell.c
	gcc -c -g shell.c

clean:
	rm *.o

run:
	./shell
