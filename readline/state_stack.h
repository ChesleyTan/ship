#pragma once
#include "shell.h"

#define STATE_STACK_SIZE 256

int push_state(const char state);
const char pop_state();
const char get_state();
void clear_state_stack();
