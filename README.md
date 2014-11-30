SHIP
---

A shell written by Chesley Tan and Dmitriy Kagno

## Compilation:
**NOTE:** This projects depends on the GNU Readline Library<br/>
(See http://cnswww.cns.cwru.edu/php/chet/readline/rltop.html)<br/>
### To Compile:
    $ make
### To Run:
    $ make run
## Features:
- Color (256) Prompt
    - Shows current time
    - Shows current user
    - Shows current working directory
    - Shows git branch and git status
    - Shows uid symbol `$` for normal user, `#` for root
    - Shows success/failure of last command in uid symbol color
- Built-in commands
    - cd, back, and exit
- Tilde expansion
    - `~` expands to the current user's home
    - `~user` expands to user's home
- Parses escape characters `\`
- Parses quotes `""` and `''`
    - Supports nested quotes
- Supports multiple commands separated by `;`
- File redirection using `<`, `>`, and `>>`
- Piping using `|`
    - Supports chained piping
- Command substitution using backticks `` ` `` or `$()`
    - Supports nested command substitutions
- Elegant SIGINT handler
- Tab completion and command history (Requires GNU Readline Library)

## TODO - Stuff we didn't have time to finish
- TODO Chaining <, >, and >>
- TODO feature toggle(runtime configuration?)
- TODO memory allocation optimization
- TODO command tab-completion
- TODO fg, bg processes (&), jobs
- TODO wildcard expansion
- TODO shell variables
- TODO arithmetic
- TODO control statements

## Files & Function Headers
### shell.c - Handles input, parsing of input, and execution
##### static void sighandler(int signo);
Handles general signals and IPC
##### static void readline_sigint_handler();
Handles SIGINT
##### void print_error();
Utility function to print errno
##### int save_stdin();
Duplicate stdin if necessary<br/>
Returns -1 on failure, 0 on success
##### int save_stdout();
Duplicate stdout if necessary<br/>
Returns -1 on failure, 0 on success
##### int save_stderr();
Duplicate stderr if necessary<br/>
Returns -1 on failure, 0 on success
##### int save_default_fds();
Calls save_stdin(), save_stdout(), and save_stderr()<br/>
Returns -1 on failure, 0 on success
##### int restore_stdin();
Restores stdin<br/>
Returns -1 on failure, 0 on success
##### int restore_stdout();
Restores stdout<br/>
Returns -1 on failure, 0 on success
##### int restore_stderr();
Restores stderr<br/>
Returns -1 on failure, 0 on success
##### int restore_default_fds();
Calls restore_stdin(), restore_stdout(), and restore_stderr()<br/>
Returns -1 on failure, 0 on success
##### void cd(const char *target);
Changes directory to target
##### void cd_back();
Changes directory to last directory
##### void execute();
Executes the current command
##### void reset_global_pipes();
Resets the global pipes used for chained piping
##### void reset_execute_variables();
Resets variables used in parsing
##### void free_all();
Frees all dynamically allocated memory
##### void parse_input(char input[INPUT_BUF_SIZE]);
Parses the input
##### void get_stdout_execute(char *container, size_t container_size);
Executes a command and stores its stdout output to container<br/>
(Currently unused)

### state_stack.c - Handles the parsing state stack
##### int push_state(const char state);
Adds the state to the state stack
##### const char pop_state();
Removes the state from the state stack<br/>
Returns the state that was removed
##### const char get_state();
Returns the current state
##### void clear_state_stack();
Clears the state stack

### prompt.c - Handles prompt generation
##### void abbreviate_home(char *full_path, size_t full_path_length);
Utility function to abbreviate `$HOME` in the current path to `~`
##### char *get_user();
Returns the current user or "Anon" on error
##### char *get_uid_symbol();
Returns a uid symbol for the user (`$` for nonroot, `#` for root)<br/>
Color of uid symbol for nonroot user is based on the exit status of the last command
##### char *get_time_str(char *time_str_container);
Places the current time in the time_str_container and returns it
##### void git_branch(char *container, size_t container_size);
Places the current git branch in the container
##### void git_status(char *container, size_t container_size);
Places a symbol representing the current git status in the container char *git();<br/>
Generates the git portion of the prompt using git_branch() and git_status()
##### void get_prompt(char *prompt, int prompt_max_size);
Generates the prompt using abbreviate_home(), get_user(), get_uid_symbol(), get_time_str(), and git()<br/>
Places the generated prompt inside the prompt parameter

