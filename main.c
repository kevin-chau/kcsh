// ksh main.c
// Basic lifetime of a shell
// - Initialize
// - Interpret
// - Terminate
// Follow tutorial https://brennan.io/2015/01/16/write-a-shell-in-c/
#include <stdlib.h>
#include <stdio.h>


#define EXIT_SUCCESS 0
#define EXIT_FAILURE 1
#define KSH_READ_LINE_BUFFER_SIZE 1024

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
        // args = ksh_split_line(line);
        // status = ksh_execute(args);

        free(line);
        // free(args);
    } while (status);
}

int main(int argc, char **argv) {
    // Load config files

    // Interpret in a loop
    ksh_loop();

    // Shutdown
    return EXIT_SUCCESS;
}