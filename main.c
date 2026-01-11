// kcsh main.c
// Basic lifetime of a shell
// - Initialize
// - Interpret
// - Terminate
// Follows the tutorial https://brennan.io/2015/01/16/write-a-shell-in-c/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>
#include <dirent.h>

#define EXIT_SUCCESS 0
#define KCSH_READ_LINE_BUFFER_SIZE 1024
#define KCSH_TOKEN_BUFFER_SIZE 64
#define KCSH_TOKEN_DELIMITERS " \t\r\n\a"
#define VERSION "0.1"

// boolean for git dir
int GIT_DIR_EXISTS = 0;

// Handle buffer allocation error
void handle_buffer_allocation_error(char * buffer) {
    if (!buffer) {
        fprintf(stderr, "kcsh: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
}

// Reads a line of indeterminate size
// TODO: reimplement with get line to avoid manual malloc
char *kcsh_read_line(void) {
    // Setup read buffer and character holder
    int buffer_size = KCSH_READ_LINE_BUFFER_SIZE;
    int position = 0;
    int c; // input character

    // Allocate a buffer
    char *buffer = malloc(sizeof(char) * buffer_size);
    handle_buffer_allocation_error(buffer);

    // Loop indefinitely or until we hit EOF or newline
    while (1) {
        // Read a character
        c = getchar();

        // If we hit EOF, replace it with a null character and return.
        if (c == EOF || c == '\n') {
            buffer[position] = '\0';
            return buffer;
        } else {
            buffer[position] = c;
        }
        position++;

        // If we have exceeded the buffer, reallocate.
        if (position >= buffer_size) {
            buffer_size += KCSH_READ_LINE_BUFFER_SIZE;
            buffer = realloc(buffer, buffer_size);
            handle_buffer_allocation_error(buffer);
        }
    }
}


// handle tokens buffer allocation error
void handle_tokens_buffer_allocation_error(char** tokens) {
    if (!tokens) {
        fprintf(stderr, "kcsh: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
}

// lookup token in environment variables
char * lookup_token_in_env(char *token) {
    if (token) {
        // Check if it's an ENV variable
        if (token[0] == '$') {
            return getenv(token+1);
        } else {
            return token;
        }
    }
    return token;
}

// Tokenizer
char **kcsh_split_line(char *line) {
    int buffer_size = KCSH_TOKEN_BUFFER_SIZE;
    int position = 0;
    char **tokens = malloc(buffer_size * sizeof(char*));
    char *token;
    handle_tokens_buffer_allocation_error(tokens);

    token = lookup_token_in_env(strtok(line, KCSH_TOKEN_DELIMITERS));
    while (token) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += KCSH_TOKEN_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            handle_tokens_buffer_allocation_error(tokens);
        }

        // Get the next token
        token = lookup_token_in_env(strtok(NULL, KCSH_TOKEN_DELIMITERS));
    }
    tokens[position] = NULL;
    return tokens;
}


// Code for launching processes
int kcsh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork(); // Fork this process
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("kcsh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) { // error forking
        perror("kcsh");
    } else { // have this (parent) process wait
        do {
            wpid = waitpid(pid, &status, WUNTRACED);
        } while (!WIFEXITED(status) && !WIFSIGNALED(status));
    }

    return 1;
}

////////////////////////////////////////////
// Build in Shell commands
////////////////////////////////////////////
int kcsh_cd(char **args);
int kcsh_help(char **args);
int kcsh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_command_strings[] = {
  "cd",
  "help",
  "exit"
};
int (*builtin_functions[]) (char **) = {
  &kcsh_cd,
  &kcsh_help,
  &kcsh_exit
};
int kcsh_num_builtins() {
    return sizeof(builtin_command_strings) / sizeof(char *);
}

int kcsh_help(char **args) {
    int i;
    printf("Kevin Chau's KCSH\n");
    printf("Type a command and hit enter!");
    printf("The following commands are built in:\n");
    for (i = 0; i < kcsh_num_builtins(); i++) {
        printf("  %s\n", builtin_command_strings[i]);
    }
    return 1;
}

int kcsh_exit(char **args) {
    return 0;
}

void check_if_git_dir_exists() {
    DIR* dir = opendir(".git");
    if (dir) {
        /* Directory exists. */
        closedir(dir);
        GIT_DIR_EXISTS = 1;
    } else {
        GIT_DIR_EXISTS = 0;
    }
}

int kcsh_cd(char **args) {
    if (!args[1]) {
        // No args, change to home folder
        if (chdir(getenv("HOME"))) {
            perror("kcsh");
        }
    } else {
        if (chdir(args[1])) {
            perror("kcsh");
        }
    }
    // check if there is a git directory
    check_if_git_dir_exists();

    return 1;
}

int kcsh_execute(char **args) {
    int i;

    if (!args[0]) { // empty command
        return 1;
    }

    for (i = 0; i < kcsh_num_builtins(); i++) {
        if (!strcmp(args[0], builtin_command_strings[i])) {
            return (*builtin_functions[i])(args);
        }
    }

    return kcsh_launch(args);
}


//////////////////////////////////////////////////////////////////////
// Main Loop
///////////////////////////////////////////////////////////////////////

// Basic loop
// - Read
// - Parse
// - Execute
void kcsh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        char * cwd_buffer = malloc(1024);
        getcwd(cwd_buffer, 1024);
        printf("\x1b[1;36m");
        if (strcmp(cwd_buffer, getenv("HOME"))) {
            printf("%s", basename(cwd_buffer));
        } else {
            printf("~");
        }
        printf(" \x1b[1;32mkcsh-%s$\x1B[0m ", VERSION);
        line = kcsh_read_line();
        args = kcsh_split_line(line);
        status = kcsh_execute(args);

        free(line);
        free(args);
        free(cwd_buffer);
    } while (status);
}

int main(int argc, char **argv) {
    // Load config files

    // Interpret in a loop
    kcsh_loop();

    // Shutdown
    return EXIT_SUCCESS;
}