#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

#define PROMPT_MAX_SIZE 1024

const char *cmd_exit = "exit";
char *prompt;

int main() {
    const char *home = getenv("HOME");
    char cwd[768];
    prompt = (char *) malloc(PROMPT_MAX_SIZE * sizeof(char *));
    char *input = (char *)malloc(sizeof(char *));
    char **opts = (char **)malloc(sizeof(char **));
    char *tok = (char *)malloc(sizeof(char *));
    while (1) {
        // TODO error checking for cwd
        getcwd(cwd, sizeof(cwd));
        strncpy(prompt, cwd, sizeof(cwd));
        // Read input
        int i = 0, optCount = 0, tokIndex = 0;
        char *match = strstr(prompt, home);
        // Replace $HOME with ~
        if (match != NULL) {
            char *trunc_path = (char *) malloc((strlen(match) + 1) * sizeof(char *));
            trunc_path[0] = '~';
            trunc_path = strcat(trunc_path, (char *) &match[strlen(home)]);
            strcpy(prompt, trunc_path);
        }
        printf("%s: ", prompt);
        // Read 255 bytes from stdin
        fgets(input, 256, stdin);
        // Iterate through each char of input
        while (input[i]) {
            // Copy char to var tok
            tok[tokIndex] = input[i];
            // Case when we've reached the end of a word
            if (input[i] == ' ') {
                opts[optCount] = (char *) malloc(sizeof(char *));
                // Copy token to opts array
                strncpy(opts[optCount], tok, tokIndex);
                // Reset tokIndex to 0
                tokIndex = 0; 
                // Increment optCount to keep track of num of opts 
                optCount++;
            }
            else {
                tokIndex++;
            }
            i++;
        }
        // Add last opt to opts array
        opts[optCount] = (char *) malloc(sizeof(char *));
        strncpy(opts[optCount], tok, tokIndex - 1);
        // Increment optCount counter
        optCount++;
        // Add required NULL argument for exec
        opts[optCount] = NULL;
        printf("cmd: %s\n", opts[0]);
        int u = 0;
        while (u <= optCount) {
            printf("opts[%d]: %s\n", u, opts[u]);
            u++;
        }
        printf("~~~~~~~~~~~~~~~~\n");
        if (strcmp(opts[0], cmd_exit) == 0) {
            exit(0); 
        }
        // Execution
        int child_pid = fork();
        if (!child_pid) {
            printf("I'm here! Woohoo!");
            execvp(opts[0], opts);
            // Note: child automatically exits after execvp, but we'll
            // explicitly call exit just in case
            exit(0);
        }
        else {
            int status;
            wait(&status);
        }
        if (errno) {
            printf("[Error %d]: %s\n", errno, strerror(errno));
        }
    }
    free(input);
    free(opts);
    free(tok);
    return 0;
}
