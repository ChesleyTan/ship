#include "prompt.h"

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

char *get_user() {
    uid_t uid = geteuid();
    struct passwd *passwd = getpwuid(uid);
    if (passwd) {
        return passwd->pw_name;
    }
    return "Anon";
}

char *get_uid_symbol() {
    char *uid_symbol;
    const char non_root = '$';
    const char root = '#';
    if (getuid() != 0) {
        if (cmd_error == CMD_ERROR) {
            int required_size = sizeof(char) * (strlen(bold_prefix) + strlen(fg_red_160) + 1 + strlen(reset) + 1);
            uid_symbol = (char *) malloc(required_size);
            sprintf(uid_symbol, "%s%s%c%s", bold_prefix, fg_red_160, non_root, reset);
        }
        else {
            int required_size = sizeof(char) * (strlen(bold_prefix) + strlen(fg_white) + 1 + strlen(reset) + 1);
            uid_symbol = (char *) malloc(required_size);
            sprintf(uid_symbol, "%s%s%c%s", bold_prefix, fg_white, non_root, reset);
        }
        return uid_symbol;
    }
    else {
        int required_size = sizeof(char) * (strlen(bold_prefix) + strlen(fg_red_196) + 1 + strlen(reset) + 1);
        uid_symbol = (char *) malloc(required_size);
        sprintf(uid_symbol, "%s%s%c%s", bold_prefix, fg_red_196, root, reset);
        return uid_symbol;
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

void git_branch(char *container, size_t container_size) {
    char *l_opts[3] = {"git", "branch", NULL};
    int pipes[2];
    if (pipe(pipes) < 0) { // Returns -1 if error
        print_error();
        return;
    }
    // Fork to execute command
    child_pid = fork();
    if (!child_pid) {
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[0]);
        freopen("/dev/null", "w", stderr); // Redirect stderr to /dev/null
        if (execvp(l_opts[0], l_opts) < 0) { // Returns -1 if error
            container[0] = '\0';
        }
        // Note: child automatically exits after successful execvp
        exit(1);
    }
    else {
        close(pipes[1]);
        char output[1024];
        int bytes = read(pipes[0], output, sizeof(output));
        if (bytes == 0) { // Git error (e.g. not in git repo)
            container[0] = '\0';
            return;
        }
        output[bytes] = '\0';
        int i = 0;
        int container_index = 0;
        for (;output[i];++i) {
            if (output[i] == '*' && output[i + 1] == ' ') {
                i += 2; // Advance past * and space
                while (output[i] && output[i] != '\n' && (container_index < container_size - 2)) {
                    container[container_index++] = output[i];
                    ++i;
                }
                break;
            }
        }
        container[container_index] = '\0';
        close(pipes[0]);
        wait(NULL);
    }
}

void git_status(char *container, size_t container_size) {
    if (container_size < GIT_STATUS_MAX_SIZE) {
        fprintf(stderr, "[Error]: Incorrect size for git status container.");
        return;
    }
    char *l_opts[3] = {"git", "status", NULL};
    int pipes[2];
    if (pipe(pipes) < 0) { // Returns -1 if error
        print_error();
        return;
    }
    // Fork to execute command
    child_pid = fork();
    if (!child_pid) {
        dup2(pipes[1], STDOUT_FILENO);
        close(pipes[0]);
        freopen("/dev/null", "w", stderr); // Redirect stderr to /dev/null
        if (execvp(l_opts[0], l_opts) < 0) { // Returns -1 if error
            container[0] = '\0';
        }
        // Note: child automatically exits after successful execvp
        exit(0);
    }
    else {
        close(pipes[1]);
        char output[4096];
        int bytes = read(pipes[0], output, sizeof(output));
        if (bytes == 0) { // Git error (e.g. not in git repo)
            container[0] = '\0';
            return;
        }
        output[bytes] = '\0';
        int container_index = 0;
        if (strstr(output, "Changes to be committed") != NULL) {
            sprintf(container, " %s", cross);
            container_index += strlen(cross) + 1;
        }
        else {
            sprintf(container, " %s", check);
            container_index += strlen(check) + 1;
        }
        if (strstr(output, "Changes not staged for commit") != NULL) {
            sprintf(container, "%s %s", container, delta);
            container_index += strlen(delta) + 1;
        }
        container[container_index] = '\0';
        close(pipes[0]);
        wait(NULL);
    }
}

char *git() {
    char *git_branch_container = (char *) malloc(GIT_BRANCH_MAX_SIZE * sizeof(char));
    int container_size = GIT_BRANCH_MAX_SIZE + GIT_STATUS_MAX_SIZE;
    char *git_container = (char *) malloc(container_size * sizeof(char));
    git_container[0] = '\0';
    git_branch(git_branch_container, GIT_BRANCH_MAX_SIZE);
    if (strlen(git_branch_container) != 0) { // If in valid git repo
        char *git_status_container = (char *) malloc(GIT_STATUS_MAX_SIZE * sizeof(char));
        git_status(git_status_container, GIT_STATUS_MAX_SIZE);
        snprintf(git_container, container_size, "(%s%s)", git_branch_container, git_status_container);
        git_container[container_size - 1] = '\0';
        free(git_status_container);
    }
    free(git_branch_container);
    return git_container;
}

void get_prompt(char *prompt, int prompt_max_size) {
    char cwd[DIR_NAME_MAX_SIZE];
    cwd[DIR_NAME_MAX_SIZE - 1] = '\0';
    // Get cwd
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        print_error();
    }
    // Generate prompt
    abbreviate_home(cwd, sizeof(cwd));
    char *time_str = (char *) malloc (TIME_MAX_SIZE * sizeof(char));
    get_time_str(time_str);
    char *uid_symbol = get_uid_symbol();
    char hostname[256];
    hostname[255] = '\0';
    gethostname(hostname, sizeof(hostname));
    char *git_container = git();
    snprintf(prompt, prompt_max_size, "%s%s[%s]%s %s%s%s%s%s%s@%s%s%s%s:%s%s%s%s%s %s%s%s%s %s\n%s%s>>%s ", bold_prefix, fg_red_196, time_str, reset, bold_prefix, fg_bright_green, get_user(), reset, bold_prefix, fg_blue_24, hostname, reset, bold_prefix, fg_bright_green, reset, bold_prefix, fg_blue_39, cwd, reset, bold_prefix, fg_green, git_container, reset, uid_symbol, bold_prefix, fg_green, reset);
    free(git_container);
    free(uid_symbol);
    free(time_str);
}

