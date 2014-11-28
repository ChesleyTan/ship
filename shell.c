// TODO finish simple redirection
// TODO check for memory leaks in `` command substitution
// TODO feature toggle(runtime config?)
// TODO memory allocation optimization
// TODO command tab-completion
// TODO fg, bg processes (&), jobs
// TODO wildcard?
// TODO shells vars dict?
// TODO arithmetic?
// TODO control statements?
#include "shell.h"
#include "prompt.h"
#include "state_stack.h"

char cmd_error = CMD_OKAY;
int child_pid, rl_child_pid;
const char *home;
char input[INPUT_BUF_SIZE];
char **opts;
char *tok;
int optCount, tokIndex;
char old_pwd[DIR_NAME_MAX_SIZE];
char keep_alive = 1;
char debug_output = 1;

static void sighandler(int signo) {
    if (signo == CMD_ERROR_SIGNAL) {
        cmd_error = CMD_ERROR;
    }
    else if (signo == SIGINT) {
        if (child_pid) {
            kill(child_pid, SIGINT);
        }
        // If no child process or child process killed
        if (!child_pid || kill(child_pid, 0) < 0) { // Returns -1 on error
            if (rl_child_pid) {
                // Kill readline process to refresh prompt
                kill(rl_child_pid, SIGINT);
            }
        }
    }
}

static void readline_sigint_handler() {
    // Exit gracefully when killed with SIGINT
    exit(SIGINT_EXIT_CODE);
}

void print_error() {
    if (errno) {
        fprintf(stderr, "[Error %d]: %s\n", errno, strerror(errno));
    }
}

void cd(const char *target) {
    // Duplicate the target; if the target points to old_pwd, we
    // don't want to overwrite the old_pwd
    char dup_target[DIR_NAME_MAX_SIZE];
    strncpy(dup_target, target, sizeof(dup_target));
    dup_target[sizeof(dup_target) - 1] = '\0';
    // Store the previous old_pwd in case chdir fails and we have to revert
    char prev_old_pwd[DIR_NAME_MAX_SIZE];
    strncpy(prev_old_pwd, old_pwd, sizeof(prev_old_pwd));
    prev_old_pwd[sizeof(prev_old_pwd) - 1] = '\0';
    getcwd(old_pwd, sizeof(old_pwd)); // Set the current dir as the new old_pwd
    if (chdir(dup_target) < 0) { // Returns -1 if error
        print_error();
        cmd_error = CMD_ERROR;
        strncpy(old_pwd, prev_old_pwd, sizeof(old_pwd)); // Restore previous old_pwd
        old_pwd[sizeof(old_pwd) - 1] = '\0';
    }
}

void cd_back() {
    cd(old_pwd);
}

void abbreviate_home(char *full_path, size_t full_path_length) {
    // Replace $HOME with ~ in full_path
    char *match = strstr(full_path, home);
    if (match != NULL) {
        int path_size = (strlen(match) - strlen(home) + 2);
        char *trunc_path = (char *) malloc(path_size * sizeof(char *));
        trunc_path[0] = '~';
        trunc_path[1] = '\0';
        trunc_path = strncat(trunc_path, (char *) &match[strlen(home)], path_size - 2);
        trunc_path[path_size - 1] = '\0';
        strncpy(full_path, trunc_path, full_path_length);
        full_path[full_path_length - 1] = '\0';
        free(trunc_path);
    }
}

void execute() {
    if (optCount <= 0) {
        return;
    }

    if (debug_output)
        printf("cmd: %s\n", opts[0]);
    int u = 0;
    while (u <= optCount) {
        if (debug_output)
            printf("opts[%d]: %s$\n", u, opts[u]);
        ++u;
    }
    if (debug_output)
        printf("<~~~~~~~~ Output ~~~~~~~~>\n");

    // Handle built-in commands
    if (strcmp(opts[0], cmd_exit) == 0) {
        printf("Exiting...\n");
        free_all();
        exit(0);
    }
    else if (strcmp(opts[0], cmd_cd) == 0){
        if (opts[1] == NULL) {
            // By default, cd to home if no directory specified
            cd(home);
        }
        else {
            cd(opts[1]);
        }
    }
    else if (strcmp(opts[0], cmd_back) == 0) {
        cd_back();
    }
    else {
        // Fork to execute command
        child_pid = fork();
        if (!child_pid) {
            if (execvp(opts[0], opts) < 0) { // Returns -1 if error
                print_error();
                // Notify parent of error
                kill(getppid(), CMD_ERROR_SIGNAL);
            }
            // Note: child automatically exits after successful execvp
            exit(1);
        }
        else {
            int status;
            waitpid(child_pid, &status, 0);
            if (WIFEXITED(status)) {
                if (WEXITSTATUS(status)) { // If exit status not 0
                    cmd_error = CMD_ERROR;
                }
            }
        }
    }
    if (debug_output)
        printf("<~~~~ End of Output ~~~~~>\n");
}

void reset_execute_variables() {
    // Reset tok
    tok[0] = '\0';
    // Reset tokIndex
    tokIndex = 0;
    // Reset opts
    if (optCount > 0) {
        while (optCount > 0) {
            free(opts[--optCount]);
        }
    }
    free(opts);
    // Reset optCount
    optCount = 0;
    // Reinstantiate opts
    opts = (char **) malloc(sizeof(char *));
}

void free_all() {
    // Free dynamically allocated memory
    if (optCount > 0) {
        while (optCount > 0) {
            free(opts[--optCount]);
        }
    }
    free(tok);
    free(opts);
}

void parse_input(char input[INPUT_BUF_SIZE]) {
    // Initializations
    opts = (char **) malloc(sizeof(char *));
    tok = (char *) malloc(sizeof(char));
    tok[0] = '\0';
    optCount = 0;
    tokIndex = 0;
    int i = 0;
    clear_state_stack();
    cmd_error = CMD_OKAY; // Reset the error flag
    inline char *get_next_keyword(const char *extra_delims) {
        char *keyword = (char *) malloc(sizeof(char));
        int keyword_index = 0;
        int index = i;
        char tmp = input[++index];
        while (tmp && tmp != '\n' && tmp != ' ' && strchr(extra_delims, tmp) == NULL) {
            keyword = (char *) realloc(keyword, (keyword_index + 1) * sizeof(char));
            keyword[keyword_index++] = tmp;
            tmp = input[++index];
        }
        // Add null-terminator
        keyword = (char *) realloc(keyword, (keyword_index + 1) * sizeof(char));
        keyword[keyword_index] = '\0';
        return keyword;
    }
    inline char *get_escaped(char *s) {
        char *keyword = (char *) malloc(sizeof(char));
        int keyword_index = 0;
        int s_len = strlen(s);
        int s_index = 0;
        char tmp = s[s_index];
        const char L_STATE_NORMAL = 0;
        const char L_STATE_IN_QUOTES = 1;
        int l_parsing_state = L_STATE_NORMAL;
        while (s_index < s_len && tmp && tmp != '\n' && (tmp != ' ' || l_parsing_state == L_STATE_IN_QUOTES)) {
            if (tmp == '\\' && l_parsing_state != L_STATE_IN_QUOTES) {
                keyword = (char *) realloc(keyword, (keyword_index + 1) * sizeof(char));
                keyword[keyword_index++] = s[++s_index];
            }
            else if (tmp == '"' || tmp == '\'') {
                if (l_parsing_state != L_STATE_IN_QUOTES) {
                    l_parsing_state = L_STATE_IN_QUOTES;
                }
                else {
                    l_parsing_state = L_STATE_NORMAL;
                }
            }
            else {
                keyword = (char *) realloc(keyword, (keyword_index + 1) * sizeof(char));
                keyword[keyword_index++] = tmp;
            }
            // Be careful to not iterate past the null-terminator
            if (s_index < s_len - 1) {
                tmp = s[s_index + 1];
            }
            ++s_index;
        }
        // Add null-terminator
        keyword = (char *) realloc(keyword, (keyword_index + 1) * sizeof(char));
        keyword[keyword_index] = '\0';
        return keyword;
    }
    // Iterate through each char of input
    while (input[i] && cmd_error != CMD_ERROR) {
        char current_state = get_state();
        if ((input[i] != '\n' && input[i] != ' ')
            || (current_state == STATE_IN_SINGLE_QUOTES
                || current_state == STATE_IN_DOUBLE_QUOTES
                || current_state == STATE_CMD_SUBSTITUTION)
        ) { // Ignore whitespace
            // State-specific handlers
            if (current_state == STATE_CMD_SUBSTITUTION && input[i] != '`') {
                // Copy char to var tok
                tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                tok[tokIndex] = input[i];
                tok[++tokIndex] = '\0';
            }
            // Handle escape characters
            else if (input[i] == '\\') {
                // Add char that follows the escape char to token
                tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                tok[tokIndex] = input[++i]; // Advance past next index in input
                tok[++tokIndex] = '\0';
            }
            // Handle semicolons (multiple commands separator)
            else if (input[i] == ';'
                && !(current_state == STATE_IN_SINGLE_QUOTES
                    || current_state == STATE_IN_DOUBLE_QUOTES
                    )
            ) {
                // Execute commands as we parse input
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
                opts[optCount] = NULL;

                execute();
                reset_execute_variables();
                if (cmd_error >= 0) { // If the command was successful, make the cmd_error flag CMD_FINISHED
                    cmd_error = CMD_FINISHED;
                }
            }
            // Handle command output expansion with backticks(`)
            else if (input[i] == '`') {
                if (debug_output)
                    printf("Found backtick\n");
                if (current_state != STATE_CMD_SUBSTITUTION) {
                    if (push_state(STATE_CMD_SUBSTITUTION) < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                }
                else {
                    if (pop_state() < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                    if (debug_output) {
                        printf("tok: %s\n", tok);
                        printf("cmd: %s\n", opts[0]);
                    }
                    char *l_input = tok;

                    int pipes[2];
                    if (pipe(pipes) < 0) { // Returns -1 if error
                        print_error();
                        return;
                    }

                    // Fork to execute command
                    int l_child_pid = fork();
                    if (!l_child_pid) {
                        if (dup2(pipes[1], STDOUT_FILENO) < 0) {
                            print_error();
                            exit(CMD_SUBSTITUTION_FAIL_EXIT_CODE);
                        }
                        close(pipes[0]);
                        freopen("/dev/null", "w", stderr); // Redirect stderr to /dev/null
                        parse_input(l_input);
                        free_all();
                        exit(cmd_error);
                    }
                    else {
                        int status;
                        waitpid(l_child_pid, &status, 0);
                        close(pipes[1]);
                        char *output = (char *) malloc(MAX_CMD_SUBSTITUTION_SIZE * sizeof(char));
                        int bytes = read(pipes[0], output, MAX_CMD_SUBSTITUTION_SIZE);
                        if (bytes == 0) { // Command did not write to stdout
                            output[0] = '\0';
                            return;
                        }
                        output[bytes] = '\0';
                        if (debug_output)
                            printf("output: %s\n", output);
                        tok = (char *) realloc(tok, (MAX_CMD_SUBSTITUTION_SIZE + 1) * sizeof(char));
                        strncpy(tok, output, MAX_CMD_SUBSTITUTION_SIZE);
                        tok[MAX_CMD_SUBSTITUTION_SIZE - 1] = '\0';
                        if (tok[0] != '\0') { // Make sure an argument exists to add
                            opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                            opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
                            // Copy token to opts and add null-terminator
                            strncpy(opts[optCount], tok, strlen(tok));
                            opts[optCount][strlen(tok)] = '\0';
                            // Increment optCount counter
                            ++optCount;
                            // Reset tok
                            tok[0] = '\0';
                            // Reset tokIndex
                            tokIndex = 0;
                            close(pipes[0]);
                            free(output);
                            wait(NULL);
                        }
                    }
                    cmd_error = CMD_FINISHED;
                }
            }
            // Interpret words in single quotes as a single token
            else if (input[i] == '\'') {
                if (current_state != STATE_IN_SINGLE_QUOTES) {
                    if (push_state(STATE_IN_SINGLE_QUOTES) < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                }
                else {
                    if (pop_state() < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                }
            }
            // Interpret words in double quotes as a single token
            else if (input[i] == '\"') {
                if (current_state != STATE_IN_DOUBLE_QUOTES) {
                    if (push_state(STATE_IN_DOUBLE_QUOTES) < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                }
                else {
                    if (pop_state() < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                }
            }
            // Tilde (~) expansion
            else if (input[i] == '~') {
                char *extra_delims = "/;>";
                char *username = get_next_keyword(extra_delims);
                // If no user was specified, expand $HOME
                if (strlen(username) == 0) {
                    // Allocate memory for $HOME in tok
                    tok = (char *) realloc(tok, (tokIndex + strlen(home) + 1) * sizeof(char));
                    // Add $HOME to token
                    tok = strcat(tok, home);
                    // Add null terminator
                    tok[tokIndex + strlen(home)] = '\0';
                    // Update tokIndex
                    tokIndex += strlen(home);
                }
                // A user was specified, so expand the user-specific home directory
                else {
                    char *escaped_username = get_escaped(username);
                    struct passwd *passwd_entry = getpwnam(escaped_username);
                    // If user is found, expand the user's home directory into opts
                    if (passwd_entry != NULL) {
                        // Get user-specific home directory
                        char *user_home = passwd_entry->pw_dir;
                        if (debug_output)
                            printf("User-specific home directory: %s\n", user_home);
                        // Allocate memory for user_home in tok
                        tok = (char *) realloc(tok, (tokIndex + strlen(user_home) + 1) * sizeof(char));
                        // Add user_home to token
                        tok = strcat(tok, user_home);
                        // Add null terminator
                        tok[tokIndex + strlen(user_home)] = '\0';
                        // Update tokIndex
                        tokIndex += strlen(user_home);

                        // Update input parsing pointer
                        i += strlen(username);
                    }
                    // Otherwise, print error
                    else {
                        fprintf(stderr, "[Error]: Could not find home directory for user %s\n", escaped_username);
                        cmd_error = CMD_ERROR;
                    }
                    free(escaped_username);
                }
                free(username);
            }
            // Redirection of stdout to file (>)
            // If we need to enter or exit redirection state
            else if (input[i] == '>'
                || ((current_state == STATE_REDIR_STDOUT_TO_FILE && (strchr(TERM_DELIM_STATE_REDIR_STDOUT_TO_FILE, input[i+1]) != NULL || input[i+1] == '\0')))
                || (current_state == STATE_REDIR_APPEND_STDOUT_TO_FILE && (strchr(TERM_DELIM_STATE_REDIR_APPEND_STDOUT_TO_FILE, input[i+1]) != NULL || input[i+1] == '\0'))
            ) {
                // We need to enter the redirection state
                if (input[i] == '>') {
                    char state;
                    if (input[i+1] == '>') { // If the operator is >>, then append
                        if (debug_output)
                            printf("Got >>\n");
                        state = STATE_REDIR_APPEND_STDOUT_TO_FILE;
                        ++i; // Advance pointer past second '>'
                    }
                    else {
                        if (debug_output)
                            printf("Got >\n");
                        state = STATE_REDIR_STDOUT_TO_FILE;
                    }
                    if (push_state(state) < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                    // Add last opt to opts array
                    if (tok[0] != '\0') { // Make sure an argument exists to add
                        opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                        opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
                        // Copy token to opts and add null terminator
                        strncpy(opts[optCount], tok, strlen(tok));
                        opts[optCount][strlen(tok)] = '\0';
                        // Increment optCount counter
                        ++optCount;
                        // Reset tok
                        tok[0] = '\0';
                        // Reset tokIndex
                        tokIndex = 0;

                        if (debug_output) {
                            printf("opt: %s\n", opts[0]);
                            printf("optCount: %d\n", optCount);
                        }
                    }
                    while (input[i+1] == ' ') {
                        // Advance past the optional whitespace following the redirection symbol
                        ++i;
                    }
                }
                // If currently in redirection state and we've reached the character before the end delimeter
                else {
                    int mode;
                    if (current_state == STATE_REDIR_APPEND_STDOUT_TO_FILE) { // If the operator is >>, then append
                        mode = O_APPEND;
                    }
                    else {
                        mode = O_TRUNC;
                    }
                    if (pop_state() < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                    // Copy current char to tok to complete filename
                    tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                    tok[tokIndex] = input[i];
                    tok[++tokIndex] = '\0';
                    // Reference tok as file
                    char *file = tok;
                    if (debug_output)
                        printf("Redirect to file: %s\n", file);
                    int fd = open(file, O_CREAT | O_WRONLY | mode, 0644); // Open file for redirection
                    if (fd < 0) { // fd is -1 on error
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    int stdout_dup;
                    if ((stdout_dup = dup(STDOUT_FILENO)) < 0) {
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    if (dup2(fd, STDOUT_FILENO) < 0) { // Redirect stdout to fd, returns -1 on error
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    // Add required NULL argument for exec
                    opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                    opts[optCount] = NULL;
                    execute();
                    close(fd);
                    if (dup2(stdout_dup, STDOUT_FILENO) < 0) { // Restore stdout
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    close(stdout_dup);
                    reset_execute_variables();
                    // If no error came up during execution, then set the cmd_error flag to CMD_FINISHED
                    if (cmd_error >= 0) {
                        cmd_error = CMD_FINISHED;
                    }
                }
            }
            // Redirection from file to stdin (<)
            // If we need to enter or exit redirection state
            else if (input[i] == '<'
                || ((current_state == STATE_REDIR_FILE_TO_STDIN && (strchr(TERM_DELIM_STATE_REDIR_FILE_TO_STDIN, input[i+1]) != NULL || input[i+1] == '\0')))
            ) {
                if (input[i] == '<') {
                    if (debug_output)
                        printf("Got <\n");
                    if (push_state(STATE_REDIR_FILE_TO_STDIN) < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                    // Add last opt to opts array
                    if (tok[0] != '\0') { // Make sure an argument exists to add
                        opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                        opts[optCount] = (char *) malloc((strlen(tok) + 1) * sizeof(char));
                        // Copy token to opts and add null terminator
                        strncpy(opts[optCount], tok, strlen(tok));
                        opts[optCount][strlen(tok)] = '\0';
                        // Increment optCount counter
                        ++optCount;
                        // Reset tok
                        tok[0] = '\0';
                        // Reset tokIndex
                        tokIndex = 0;

                        if (debug_output) {
                            printf("opt: %s\n", opts[0]);
                            printf("optCount: %d\n", optCount);
                        }
                    }
                    while (input[i+1] == ' ') {
                        // Advance past the optional whitespace following the redirection symbol
                        ++i;
                    }
                }
                // If currently in redirection state and we've reached the character before the end delimeter
                else {
                    if (pop_state() < 0) {
                        cmd_error = CMD_ERROR;
                        return;
                    }
                    // Copy current char to tok to complete filename
                    tok = (char *) realloc(tok, (tokIndex + 2) * sizeof(char));
                    tok[tokIndex] = input[i];
                    tok[++tokIndex] = '\0';
                    // Reference tok as file
                    char *file = tok;
                    if (debug_output)
                        printf("Redirect file to stdin: %s\n", file);
                    int fd = open(file, O_RDONLY); // Open file for redirection
                    if (fd < 0) { // fd is -1 on error
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    int stdin_dup;
                    if ((stdin_dup = dup(STDIN_FILENO)) < 0) {
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    if (dup2(fd, STDIN_FILENO) < 0) { // Redirect fd to stdin
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    // Add required NULL argument for exec
                    opts = (char **) realloc(opts, (optCount + 1) * sizeof(char *));
                    opts[optCount] = NULL;
                    if (debug_output) {
                        printf("opt: %s\n", opts[0]);
                        printf("optCount: %d\n", optCount);
                    }
                    execute();
                    close(fd);
                    if (dup2(stdin_dup, STDIN_FILENO)) { // Restore stdin
                        print_error();
                        cmd_error = CMD_ERROR;
                        reset_execute_variables();
                        return;
                    }
                    close(stdin_dup);
                    reset_execute_variables();
                    // If no error came up during execution, then set the cmd_error flag to CMD_FINISHED
                    if (cmd_error >= 0) {
                        cmd_error = CMD_FINISHED;
                    }
                }
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
            strncpy(opts[optCount], tok, strlen(tok));
            opts[optCount][strlen(tok)] = '\0';
            // Reset tok
            tok[0] = '\0';
            // Reset tokIndex to 0
            tokIndex = 0;
            // Increment optCount to keep track of num of opts
            ++optCount;
        }
        ++i;
    }
    // If a command was supplied, then try to execute it
    // !(optCount == 0 && tokIndex == 0) ensures that there was
    // at least one non-whitespace character in the input.
    // In addition, the input must be valid up to this point
    if (i >= 1 && !(optCount == 0 && tokIndex == 0) && cmd_error != CMD_ERROR) {
        if (debug_output) {
            printf("Executing standalone command\n");
            printf("optCount: %d\n", optCount);
            printf("tokIndex: %d\n", tokIndex);
        }
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
        opts[optCount] = NULL;

        execute();
    }
    else {
        if (cmd_error == CMD_OKAY) {
            cmd_error = CMD_BLANK;
        }
    }
}

void get_stdout_execute(char *container, size_t container_size) {
    int pipes[2];
    if (pipe(pipes) < 0) { // Returns -1 if error
        print_error();
        return;
    }
    // Fork to execute command
    child_pid = fork();
    if (!child_pid) {
        if (dup2(pipes[1], STDOUT_FILENO) < 0) {
            print_error();
            exit(1);
        }
        close(pipes[0]);
        freopen("/dev/null", "w", stderr); // Redirect stderr to /dev/null
        if (execvp(opts[0], opts) < 0) { // Returns -1 if error
            container[0] = '\0';
        }
        // Note: child automatically exits after successful execvp
        exit(1);
    }
    else {
        close(pipes[1]);
        int bytes = read(pipes[0], container, container_size);
        if (bytes == 0) { // Command did not write to stdout
            container[0] = '\0';
            return;
        }
        container[container_size - 1] = '\0';
        close(pipes[0]);
        wait(NULL);
    }
}

int main() {
    signal(CMD_ERROR_SIGNAL, sighandler);
    signal(SIGINT, sighandler);
    // TODO allow for possible changing home dir
    home = getenv("HOME");
    // Initialize old_pwd to the current directory
    getcwd(old_pwd, sizeof(old_pwd));
    while (keep_alive) {
        int pipes[2]; // Pipe input from child to parent process
        if (pipe(pipes) < 0) { // Returns -1 if error
            print_error();
            exit(1);
        }
        rl_child_pid = fork(); // Fork to read input
        if (!rl_child_pid) {
            signal(SIGINT, readline_sigint_handler);
            char *prompt = (char *) malloc(PROMPT_MAX_SIZE * sizeof(char));
            get_prompt(prompt, PROMPT_MAX_SIZE);
            close(pipes[0]);
            char *line = readline(prompt);
            if (line == NULL) {
                printf("\n[Reached EOF]\n");
                // Free dynamically allocated memory before exiting
                free(prompt);
                free(line);
                // Close pipe before exiting
                close(pipes[1]);
                exit(EOF_EXIT_CODE);
            }
            write(pipes[1], line, INPUT_BUF_SIZE);
            // Free dynamically allocated memory before exiting
            free(prompt);
            free(line);
            exit(0);
        }
        else {
            int status;
            waitpid(rl_child_pid, &status, 0);
            if (WIFEXITED(status)) {
                status = WEXITSTATUS(status);
                if (status == EOF_EXIT_CODE) {
                    // Close pipe before exiting
                    close(pipes[0]);
                    close(pipes[1]);
                    exit(0);
                }
                else if (status == SIGINT_EXIT_CODE) {
                    // Close pipe before relooping
                    close(pipes[0]);
                    close(pipes[1]);
                    // Go to new line before relooping
                    write(STDOUT_FILENO, "\n", 1);
                    continue;
                }
            }
            close(pipes[1]);
            int bytes = read(pipes[0], input, INPUT_BUF_SIZE);
            close(pipes[0]);
            // Add null-terminator to input
            input[bytes] = '\0';
            if (debug_output)
                printf("input: %s\n", input);
            parse_input(input);
            // Add command to history if it was successful
            if (cmd_error >= 0) {
                add_history(input);
            }
            else {
                if (debug_output)
                    printf("Could not add to history: %d\n", cmd_error);
            }
            free_all();
            wait(NULL);
        }
    }
    return 0;
}

