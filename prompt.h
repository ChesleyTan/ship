#pragma once
#include "shell.h"

// Constants
#define PROMPT_MAX_SIZE 1024
#define TIME_MAX_SIZE 50
#define GIT_BRANCH_MAX_SIZE 128
#define GIT_STATUS_MAX_SIZE 50

// ANSI Escape codes (wrapped with \001 and \002 so readline ignores
// non-printing characters when calculating prompt size)
static const char *reset = "\001\e[0m\002";
static const char *bold_prefix = "\001\e[1;\002";
static const char *dim_prefix = "\001\e[2;\002";
static const char *underline_prefix = "\001\e[4;\002";
static const char *fg_blue_39 = "\00138;5;39m\002";
static const char *fg_blue_24 = "\00138;5;24m\002";
static const char *fg_red_196 = "\00138;5;196m\002";
static const char *fg_red_160 = "\00138;5;160m\002";
static const char *fg_white = "\00138;5;15m\002";
static const char *fg_bright_green = "\00138;5;118m\002";
static const char *fg_green = "\00138;5;34m\002";

// UTF-8
static const char *cross = "\001\xe2\x9c\x98\002";
static const char *check = "\001\xe2\x9c\x94\002";
static const char *delta = "\001\xce\x94\002";

// Function type signatures
void abbreviate_home(char *full_path, size_t full_path_length);
char *get_user();
char *get_uid_symbol();
char *get_time_str(char *time_str_container);
void git_branch(char *container, size_t container_size);
void git_status(char *container, size_t container_size);
char *git();
void get_prompt(char *prompt, int prompt_max_size);

