all: shell.o
	gcc -o shell shell.o
	make clean

shell.o: shell.c shell.h
	gcc -c -g shell.c

clean:
	rm *.o

run:
	./shell

new: new_shell.o
	gcc -o new_shell new_shell.o
	make clean

new_shell.o: new_shell.c new_shell.h
	gcc -c -g new_shell.c


