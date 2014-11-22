#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <pwd.h>
#include <signal.h>

// Constants
#define INPUT_BUF_SIZE 256
#define PROMPT_MAX_SIZE 1024
#define DATE_MAX_SIZE 50
#define TRUE 1
#define FALSE 0
#define CMD_ERROR_SIGNAL SIGUSR1

// Shell built-in functions
const char *cmd_exit = "exit";
const char *cmd_cd = "cd";

// ANSI Escape codes
const char *reset = "\e[0m";
const char *bold_prefix = "\e[1;";
const char *dim_prefix = "\e[2;";
const char *underline_prefix = "\e[4;";
const char *fg_blue_39 = "38;5;39m";
const char *fg_red_196 = "38;5;196m";
const char *fg_red_160 = "38;5;160m";
const char *fg_white = "38;5;15m";
const char *fg_bright_green = "38;5;118m";
const char *fg_green = "38;5;34m";

// Function type signatures
static void sighandler(int signo);
void print_error();
char *get_user();
char *get_uid_symbol(char *uid_symbol_container);
char *get_time_str(char *time_str_container);
void abbreviate_home(char *full_path, size_t full_path_length);
void execute();
void free_all();
void get_prompt(char *prompt, int prompt_max_size);
void parse_input(char input[INPUT_BUF_SIZE]);

