// ksh main.c
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

#define EXIT_SUCCESS 0
#define KSH_READ_LINE_BUFFER_SIZE 1024
#define KSH_TOKEN_BUFFER_SIZE 64
#define KSH_TOKEN_DELIMITERS " \t\r\n\a"

// Handle buffer allocation error
void handle_buffer_allocation_error(char * buffer) {
    if (!buffer) {
        fprintf(stderr, "ksh: memory allocation error\n");
        exit(EXIT_FAILURE);
    }
}

// Reads a line of indeterminate size
// TODO: reimplement with get line to avoid manual malloc
char *ksh_read_line(void) {
    // Setup read buffer and character holder
    int buffer_size = KSH_READ_LINE_BUFFER_SIZE;
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
            buffer_size += KSH_READ_LINE_BUFFER_SIZE;
            buffer = realloc(buffer, buffer_size);
            handle_buffer_allocation_error(buffer);
        }
    }
}


// handle tokens buffer allocation error
void handle_tokens_buffer_allocation_error(char** tokens) {
    if (!tokens) {
        fprintf(stderr, "ksh: memory allocation error\n");
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
char **ksh_split_line(char *line) {
    int buffer_size = KSH_TOKEN_BUFFER_SIZE;
    int position = 0;
    char **tokens = malloc(buffer_size * sizeof(char*));
    char *token;
    handle_tokens_buffer_allocation_error(tokens);

    token = lookup_token_in_env(strtok(line, KSH_TOKEN_DELIMITERS));
    while (token) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += KSH_TOKEN_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            handle_tokens_buffer_allocation_error(tokens);
        }

        // Get the next token
        token = lookup_token_in_env(strtok(NULL, KSH_TOKEN_DELIMITERS));
    }
    tokens[position] = NULL;
    return tokens;
}


// Code for launching processes
int ksh_launch(char **args) {
    pid_t pid, wpid;
    int status;

    pid = fork(); // Fork this process
    if (pid == 0) {
        if (execvp(args[0], args) == -1) {
            perror("ksh");
        }
        exit(EXIT_FAILURE);
    } else if (pid < 0) { // error forking
        perror("ksh");
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
int ksh_cd(char **args);
int ksh_help(char **args);
int ksh_exit(char **args);

/*
  List of builtin commands, followed by their corresponding functions.
 */
char *builtin_command_strings[] = {
  "cd",
  "help",
  "exit"
};
int (*builtin_functions[]) (char **) = {
  &ksh_cd,
  &ksh_help,
  &ksh_exit
};
int ksh_num_builtins() {
    return sizeof(builtin_command_strings) / sizeof(char *);
}

int ksh_help(char **args) {
    int i;
    printf("Kevin Chau's KSH\n");
    printf("Type a command and hit enter!");
    printf("The following commands are built in:\n");
    for (i = 0; i < ksh_num_builtins(); i++) {
        printf("  %s\n", builtin_command_strings[i]);
    }
    return 1;
}

int ksh_exit(char **args) {
    return 0;
}

int ksh_cd(char **args) {
    if (!args[1]) {
        // No args, change to home folder
        if (chdir(getenv("HOME"))) {
            perror("ksh");
        }
    } else {
        if (chdir(args[1])) {
            perror("ksh");
        }
    }
    return 1;
}

int ksh_execute(char **args) {
    int i;

    if (!args[0]) { // empty command
        return 1;
    }

    for (i = 0; i < ksh_num_builtins(); i++) {
        if (!strcmp(args[0], builtin_command_strings[i])) {
            return (*builtin_functions[i])(args);
        }
    }

    return ksh_launch(args);
}


//////////////////////////////////////////////////////////////////////
// Main Loop
///////////////////////////////////////////////////////////////////////

// Basic loop
// - Read
// - Parse
// - Execute
void ksh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        char * cwd_buffer = malloc(1024);
        getcwd(cwd_buffer, 1024);
        if (strcmp(cwd_buffer, getenv("HOME"))) {
            printf("\x1b[1;36m%s\x1B[0m \x1b[1;31mksh$\x1B[0m ", basename(cwd_buffer));
        } else {
            printf("\x1b[1;36m~\x1B[0m \x1b[1;31mksh$\x1B[0m ");
        }
        line = ksh_read_line();
        args = ksh_split_line(line);
        status = ksh_execute(args);

        free(line);
        free(args);
        free(cwd_buffer);
    } while (status);
}

int main(int argc, char **argv) {
    // Load config files

    // Interpret in a loop
    ksh_loop();

    // Shutdown
    return EXIT_SUCCESS;
}