LIBS=-lreadline
C_FILES=shell.c state_stack.c prompt.c
O_FILES=shell.o state_stack.o prompt.o
WARNINGS_QUIET=-Wall -Wno-unused-variable -Wno-unused-function
WARNINGS_ALL=-Wall

WARNINGS=$(WARNINGS_QUIET)

DEBUG=-g
all: shell.o state_stack.o prompt.o
	@gcc -o shell $(O_FILES) $(LIBS)
	@make clean

shell.o: shell.c shell.h state_stack.h prompt.h
	@gcc -c $(DEBUG) $(WARNINGS) shell.c

state_stack.o: state_stack.c state_stack.h shell.h
	@gcc -c $(DEBUG) $(WARNINGS) state_stack.c

prompt.o: prompt.c prompt.h shell.h
	@gcc -c $(DEBUG) $(WARNINGS) prompt.c

clean:
	@rm *.o

run:
	@./shell

