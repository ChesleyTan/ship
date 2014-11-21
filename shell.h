#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#define INPUT_BUF_SIZE 256
#define PROMPT_MAX_SIZE 1024
#define DATE_MAX_SIZE 50

const char *cmd_exit = "exit";
const char *cmd_cd = "cd";

void print_error();
char *get_time_str();
void abbreviate_home(char *, const char *, size_t);

