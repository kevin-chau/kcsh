// ksh main.c
// Basic lifetime of a shell
// - Initialize
// - Interpret
// - Terminate
// Follow tutorial https://brennan.io/2015/01/16/write-a-shell-in-c/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
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

// Tokenizer
char **ksh_split_line(char *line) {
    int buffer_size = KSH_TOKEN_BUFFER_SIZE;
    int position = 0;
    char **tokens = malloc(buffer_size * sizeof(char*));
    char *token;
    handle_tokens_buffer_allocation_error(tokens);

    token = strtok(line, KSH_TOKEN_DELIMITERS);
    while (token) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += KSH_TOKEN_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            handle_tokens_buffer_allocation_error(tokens);
        }

        // Get the next token
        token = strtok(NULL, KSH_TOKEN_DELIMITERS);
    }
    tokens[position] = NULL;
    return tokens;
}


// Basic loop
// - Read
// - Parse
// - Execute
void ksh_loop(void) {
    char *line;
    char **args;
    int status;

    do {
        printf("ksh$ ");
        line = ksh_read_line();
        args = ksh_split_line(line);
        // status = ksh_execute(args);
        status = 1;

        free(line);
        free(args);
    } while (status);
}

int main(int argc, char **argv) {
    // Load config files

    // Interpret in a loop
    ksh_loop();

    // Shutdown
    return EXIT_SUCCESS;
}