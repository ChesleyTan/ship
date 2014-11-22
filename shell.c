// TODO dynamically allocated cwd size?
// TODO SIGINT to kill child processes
// TODO abstraction of sections in main()
// TODO parsing of ;
// TODO implement proper use of quotation marks
// TODO command history
// TODO tab completion
#include "shell.h"

char cmd_error = FALSE;

static void sighandler(int signo) {
    if (signo == CMD_ERROR_SIGNAL) {
        cmd_error = TRUE;
    }
}

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

char *get_uid_symbol(char *uid_symbol_container) {
    const char non_root = '$';
    const char root = '#';
    if (getuid() != 0) {
        if (cmd_error == TRUE) {
            sprintf(uid_symbol_container, "%s%s%c%s", bold_prefix, fg_red_160, non_root, reset);
        }
        else {
            sprintf(uid_symbol_container, "%s%s%c%s", bold_prefix, fg_white, non_root, reset);
        }
        return uid_symbol_container;
    }
    else {
        sprintf(uid_symbol_container, "%s%s%c%s", bold_prefix, fg_red_196, root, reset);
        return uid_symbol_container;
    }
}

char *get_time_str(char *time_str_container) {
    time_t rawtime;
    time(&rawtime);
    struct tm *time = localtime(&rawtime);
    // TODO size check
    // Pad integer with zeroes to a length of 2
    if (sprintf(time_str_container, "%'02d:%'02d:%'02d", time->tm_hour, time->tm_min, time->tm_sec) < 0) {
        printf("[Error]: Error formatting time string.");
    }
    return time_str_container;
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

int main() {
    signal(CMD_ERROR_SIGNAL, sighandler);
    // TODO allow for possible changing home dir
    const char *home = getenv("HOME");
    while (!feof(stdin)) {
        // Initializations
        char cwd[768];
        cwd[767] = '\0';
        char input[INPUT_BUF_SIZE];
        char *prompt = (char *) malloc(PROMPT_MAX_SIZE * sizeof(char *));
        char **opts = (char **) malloc(sizeof(char *));
        char *tok = (char *) malloc(sizeof(char));
        tok[0] = '\0';

        // Get cwd
        if (getcwd(cwd, sizeof(cwd)) == NULL) {
            print_error();
        }
        // Generate prompt
        abbreviate_home(cwd, home, sizeof(cwd));
        char *time_str = (char *) malloc (DATE_MAX_SIZE);
        get_time_str(time_str);
        char *uid_symbol = (char *) calloc(sizeof(char), 20);
        get_uid_symbol(uid_symbol);
        snprintf(prompt, PROMPT_MAX_SIZE, "%s%s[%s]%s %s%s%s:%s%s%s%s%s %s\n%s%s>>%s ", bold_prefix, fg_red_196, time_str, reset, bold_prefix, fg_bright_green, get_user(), reset, bold_prefix, fg_blue_39, cwd, reset, uid_symbol, bold_prefix, fg_green, reset);
        free(uid_symbol);
        free(time_str);
        printf("%s", prompt);

        // Read INPUT_BUF_SIZE - 1 bytes from stdin
        if (fgets(input, INPUT_BUF_SIZE, stdin) == NULL) {
            printf("\n[Reached EOF]\n");
            exit(0);
        }
        // Parse input
        int i = 0, optCount = 0, tokIndex = 0;
        // Iterate through each char of input
        while (input[i]) {
            if (input[i] != '\n' && input[i] != ' ') { // Ignore whitespace
                // Handle escape characters
                if (input[i] == '\\') {
                    // Add char that follows the escape char to token
                    tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                    tok[tokIndex] = input[++i]; // Advance past next index in input
                    tok[++tokIndex] = '\0';
                }
                // Substitute ~ with $HOME, if applicable
                else if (input[i] == '~') {
                    // Allocate memory for $HOME in tok
                    tok = (char *) realloc(tok, (tokIndex + strlen(home) + 1) * sizeof(char));
                    // Add $HOME to token
                    tok = strcat(tok, home);
                    // Add null terminator
                    tok[tokIndex + strlen(home)] = '\0';
                    // Update tokIndex
                    tokIndex += strlen(home);
                }
                // Copy char to var tok
                else {
                    tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                    tok[tokIndex] = input[i];
                    tok[++tokIndex] = '\0';
                }
            }
            // Case when we've reached the end of a word
            // tokIndex != 0 ensures that there is non-whitespace preceding this space
            else if (input[i] == ' ' && tokIndex != 0) {
                opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                // Copy token to opts array
                opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
                strncpy(opts[optCount], tok, tokIndex + 1);
                // Clear tok
                tok[0] = '\0';
                // Reset tokIndex to 0
                tokIndex = 0;
                // Increment optCount to keep track of num of opts
                ++optCount;
            }
            ++i;
        }
        cmd_error = FALSE; // Reset the error flag
        // If a command was supplied, then try to execute it
        // !(optCount == 0 && tokIndex == 0) ensures that there was
        // at least one non-whitespace character in the input
        if (i > 1 && !(optCount == 0 && tokIndex == 0)) {
            // Add last opt to opts array
            if (tok[0] != '\0') { // Make sure an argument exists to add
                opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
                // Copy token to opts and add null terminator
                strncpy(opts[optCount], tok, strlen(tok));
                opts[optCount][strlen(tok)] = '\0';
                // Increment optCount counter
                ++optCount;
            }

            // Add required NULL argument for exec
            opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
            opts[optCount] = (char *) malloc(sizeof(char));
            opts[optCount] = NULL;

            // Debug info
            printf("cmd: %s\n", opts[0]);
            int u = 0;
            while (u <= optCount) {
                printf("opts[%d]: %s$\n", u, opts[u]);
                ++u;
            }
            printf("<~~~~~~~~ Output ~~~~~~~~>\n");

            // Handle built-in commands
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
                        cmd_error = TRUE;
                    }
                }
                else if (chdir(opts[1]) < 0) { // Returns -1 if error
                    print_error();
                    cmd_error = TRUE;
                }
            }
            else {
                // Fork to execute command
                int child_pid = fork();
                if (!child_pid) {
                    if (execvp(opts[0], opts) < 0) { // Returns -1 if error
                        print_error();
                        // Notify parent of error
                        kill(getppid(), CMD_ERROR_SIGNAL);
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

