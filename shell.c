// TODO dynamically allocated cwd size?
// TODO expand ~ in command line to $HOME
// TODO implement parsing of \ escape characters
// TODO implement proper use of quotation marks
// TODO command history
// TODO tab completion
#include "shell.h"

void print_error() {
    if (errno) {
        printf("[Error %d]: %s\n", errno, strerror(errno));
    }
}

char *get_user() {
    uid_t uid = geteuid();
    struct passwd *passwd = getpwuid(uid);
    if (passwd) {
        return passwd->pw_name;
    }
    return "Anon";
}

// TODO take dynamically allocated string as input
char *get_uid_symbol() {
    const char non_root = '$';
    const char root = '#';
    char *symbol = (char *) calloc(sizeof(char), 20);
    if (getuid() != 0) {
        sprintf(symbol, "%s%s%c%s", bold_prefix, fg_white, non_root, reset);
        return symbol;
    }
    else {
        sprintf(symbol, "%s%s%c%s", bold_prefix, fg_red_9, root, reset);
        return symbol;
    }
}

// TODO take dynamically allocated string as input
char *get_time_str() {
    time_t rawtime;
    time(&rawtime);
    struct tm *time = localtime(&rawtime);
    char *time_str = (char *) malloc(DATE_MAX_SIZE);
    // TODO size check
    // Pad integer with zeroes to a length of 2
    if (sprintf(time_str, "%'02d:%'02d:%'02d", time->tm_hour, time->tm_min, time->tm_sec) < 0) {
        printf("[Error]: Error formatting time string.");
    }
    return time_str;
}

void abbreviate_home(char *full_path, const char *home_dir, size_t full_path_length) {
    // Replace $HOME with ~ in full_path
    char *match = strstr(full_path, home_dir);
    if (match != NULL) {
        int path_size = (strlen(match) - strlen(home_dir) + 2);
        char *trunc_path = (char *) malloc(path_size * sizeof(char *));
        trunc_path[0] = '~';
        trunc_path[1] = '\0';
        trunc_path = strncat(trunc_path, (char *) &match[strlen(home_dir)], path_size - 2);
        trunc_path[path_size - 1] = '\0';
        strncpy(full_path, trunc_path, full_path_length);
        free(trunc_path);
    }
}

void trim_whitespace(char *input) {
    // TODO trim trailing whitespace
    int i = 0;
    // Count number of preceding whitespace
    int num_preceding_whitespace = 0;
    while (input[i] && input[i] == ' ') {
        ++num_preceding_whitespace;
        ++i;
    }
    // Trim preceding whitespace
    i = 0;
    if (num_preceding_whitespace > 0) {
        while(input[i]) {
            input[i] = input[i + num_preceding_whitespace];
            ++i;
        }
    }
}

int main() {
    // TODO allow for possible changing home dir
    const char *home = getenv("HOME");
    while (1) {
        // Initializations
        char cwd[768];
        cwd[768] = '\0';
        char input[INPUT_BUF_SIZE];
        char *prompt = (char *) malloc(PROMPT_MAX_SIZE * sizeof(char *));
        char **opts = (char **) malloc(sizeof(char *));
        char *tok = (char *) malloc(sizeof(char));

        // Get cwd
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            print_error();
        }
        // Generate prompt
        abbreviate_home(cwd, home, sizeof(cwd));
        char *time_str = get_time_str();
        char *uid_symbol = get_uid_symbol();
        snprintf(prompt, PROMPT_MAX_SIZE, "%s%s[%s]%s %s%s%s:%s%s%s%s%s %s\n%s%s>>%s ", bold_prefix, fg_red_9, time_str, reset, bold_prefix, fg_bright_green, get_user(), reset, bold_prefix, fg_blue_39, cwd, reset, uid_symbol, bold_prefix, fg_green, reset);
        free(uid_symbol);
        free(time_str);
        printf("%s", prompt);

        // Read INPUT_BUF_SIZE - 1 bytes from stdin
        if (fgets(input, INPUT_BUF_SIZE, stdin) == NULL) {
            printf("[Error]: fgets error");
        }
        // Parse input
        int i = 0, optCount = 0, tokIndex = 0;
        // Trim whitespace
        trim_whitespace(input);
        // Iterate through each char of input
        while (input[i]) {
            if (input[i] != '\n') {
                // Copy char to var tok
                tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                tok[tokIndex] = input[i];
                tok[tokIndex + 1] = '\0';
            }
            // Case when we've reached the end of a word
            if (input[i] == ' ') {
                // Add null terminator, replacing the space
                tok[tokIndex] = '\0';
                opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                // Copy token to opts array
                opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
                strncpy(opts[optCount], tok, tokIndex + 1);
                // Reset tokIndex to 0
                tokIndex = 0;
                // Increment optCount to keep track of num of opts
                ++optCount;
            }
            else {
                ++tokIndex;
            }
            ++i;
        }
        if (i > 1) {
            // Add last opt to opts array
            opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
            opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
            // Copy token to opts and add null terminator
            strncpy(opts[optCount], tok, strlen(tok));
            opts[optCount][strlen(tok)] = '\0';
            // Increment optCount counter
            ++optCount;
            // Add required NULL argument for exec
            opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
            opts[optCount] = (char *) malloc(sizeof(char));
            opts[optCount] = NULL;
            printf("cmd: %s\n", opts[0]);
            int u = 0;
            while (u <= optCount) {
                printf("opts[%d]: %s$\n", u, opts[u]);
                ++u;
            }
            printf("<~~~~~~~~ Output ~~~~~~~~>\n");
            if (strcmp(opts[0], cmd_exit) == 0) {
                printf("Exiting...\n");
                // Free dynamically allocated memory before exiting
                free(prompt);
                for (;optCount >= 0;--optCount) {
                    free(opts[optCount]);
                }
                free(opts);
                free(tok);
                exit(0);
            }
            else if (strcmp(opts[0], cmd_cd) == 0){
	      if (opts[1] == NULL) {
		if (chdir(home) < 0) { // Returns -1 if error
		    print_error();
		}
	      }
	      else if (chdir(opts[1]) < 0) { // Returns -1 if error
		print_error();
	      }
	    }
            else {
                // Execution
                int child_pid = fork();
                if (!child_pid) {
                    if (execvp(opts[0], opts) < 0) { // Returns -1 if error
                        print_error();
                    }
                    // Note: child automatically exits after successful execvp
                    exit(0);
                }
                else {
                    int status;
                    wait(&status);
                }
            }
        }
        // Free dynamically allocated memory
        free(prompt);
        if (optCount > 0) {
            for (;optCount >= 0;--optCount) {
                free(opts[optCount]);
            }
        }
        free(opts);
        free(tok);
    }
    return 0;
}

