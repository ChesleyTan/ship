#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <string.h>
#include <errno.h>

const char *cmd_exit = "exit";
char *prompt;

int main() {
    const char *home = getenv("HOME");
    char cwd[1024];
    prompt = (char *) malloc(sizeof(cwd) * sizeof(char *));
    // TODO error checking
    getcwd(cwd, sizeof(cwd));
    strncpy(prompt, cwd, sizeof(cwd));
    char *input = (char *)malloc(sizeof(char *));
    char **opts = (char **)malloc(sizeof(char **));
    char *tok = (char *)malloc(sizeof(char *));
    while (1) {
        int i = 0, optCount = 0, tokIndex = 0;
        printf("%s: \n", prompt);
        printf("%s: \n", home);
        strsep(&prompt, home);
        // TODO extract local directory
        printf("Rest of cwd: %s\n", prompt);
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
