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
#include <readline/readline.h>
#include <readline/history.h>

// Constants
#define INPUT_BUF_SIZE 256
#define PROMPT_MAX_SIZE 1024
#define DATE_MAX_SIZE 50
#define TRUE 1
#define FALSE 0
#define CMD_ERROR 0
#define CMD_OKAY 1
#define CMD_BLANK 2
#define CMD_ERROR_SIGNAL SIGUSR1
#define DIR_NAME_MAX_SIZE 768
#define GIT_BRANCH_MAX_SIZE 128
#define GIT_STATUS_MAX_SIZE 10

// Shell built-in functions
const char *cmd_exit = "exit";
const char *cmd_cd = "cd";
const char *cmd_back = "back";

// ANSI Escape codes (wrapped with \001 and \002 so readline ignores
// non-printing characters when calculating prompt size)
const char *reset = "\001\e[0m\002";
const char *bold_prefix = "\001\e[1;\002";
const char *dim_prefix = "\001\e[2;\002";
const char *underline_prefix = "\001\e[4;\002";
const char *fg_blue_39 = "\00138;5;39m\002";
const char *fg_blue_24 = "\00138;5;24m\002";
const char *fg_red_196 = "\00138;5;196m\002";
const char *fg_red_160 = "\00138;5;160m\002";
const char *fg_white = "\00138;5;15m\002";
const char *fg_bright_green = "\00138;5;118m\002";
const char *fg_green = "\00138;5;34m\002";

// UTF-8
const char *cross = "\xe2\x9c\x98";
const char *check = "\xe2\x9c\x94";
const char *delta = "\xce\x94";

// Parsing states
const char STATE_NORMAL = 0;
const char STATE_IN_QUOTES = 1;

// Function type signatures
static void sighandler(int signo);
void print_error();
void cd(const char *target);
void cd_back();
char *get_user();
char *get_uid_symbol();
char *get_time_str(char *time_str_container);
void abbreviate_home(char *full_path, size_t full_path_length);
void execute();
void free_all();
void get_prompt(char *prompt, int prompt_max_size);
void parse_input(char input[INPUT_BUF_SIZE]);
void git_branch(char *container, size_t container_size);
void git_status(char *container, size_t container_size);
char *git();
