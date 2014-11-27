#include "state_stack.h"

char state_stack[STATE_STACK_SIZE];
int stack_pointer = 0;

int push_state(const char state) {
    if (stack_pointer > STATE_STACK_SIZE - 1) {
        fprintf(stderr, "[Error]: Too many states on the stack.\n");
        return -1;
    }
    state_stack[stack_pointer++] = state;
    return 0;
}

const char pop_state() {
    if (stack_pointer < 1) {
        fprintf(stderr, "[Error]: Pop: Negative stack pointer.\n");
        return -1;
    }
    return state_stack[--stack_pointer];
}

const char get_state() {
    if (stack_pointer < 1) {
        fprintf(stderr, "[Error]: Get: Negative stack pointer.\n");
        return -1;
    }
    return state_stack[stack_pointer - 1];
}

void clear_state_stack() {
    stack_pointer = 0;
    state_stack[stack_pointer++] = STATE_NORMAL;
}

