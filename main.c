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

#define DEFAULT_BUFFER_SIZE 1024
#define EXIT_SUCCESS 0
#define KCSH_READ_LINE_BUFFER_SIZE DEFAULT_BUFFER_SIZE
#define KCSH_TOKEN_BUFFER_SIZE 64
#define KCSH_TOKEN_DELIMITERS " \t\r\n\a"
#define VERSION "0.1"
char acOpen[]  = {"'\"[<{"};
char acClose[] = {"'\"]>}"};

// boolean for git dir
int GIT_DIR_EXISTS = 0;
int cwd_depth_relative_to_git_root = 0;

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

char *strmbtok ( char *input, char *delimit, char *openblock, char *closeblock) {
    static char *token = NULL;
    char *lead = NULL;
    char *block = NULL;
    int iBlock = 0;
    int iBlockIndex = 0;

    if ( input != NULL) {
        token = input;
        lead = input;
    }
    else {
        lead = token;
        if ( *token == '\0') {
            lead = NULL;
        }
    }

    while ( *token != '\0') {
        if ( iBlock) {
            if ( closeblock[iBlockIndex] == *token) {
                iBlock = 0;
            }
            token++;
            continue;
        }
        if ( ( block = strchr ( openblock, *token)) != NULL) {
            iBlock = 1;
            iBlockIndex = block - openblock;
            token++;
            continue;
        }
        if ( strchr ( delimit, *token) != NULL) {
            *token = '\0';
            token++;
            break;
        }
        token++;
    }
    return lead;
}

// Tokenizer
char **kcsh_split_line(char *line) {
    int buffer_size = KCSH_TOKEN_BUFFER_SIZE;
    int position = 0;
    char **tokens = malloc(buffer_size * sizeof(char*));
    char *token;
    handle_tokens_buffer_allocation_error(tokens);

    token = lookup_token_in_env(strmbtok(line, " ", acOpen, acClose));
    while (token) {
        tokens[position] = token;
        position++;

        if (position >= buffer_size) {
            buffer_size += KCSH_TOKEN_BUFFER_SIZE;
            tokens = realloc(tokens, buffer_size * sizeof(char*));
            handle_tokens_buffer_allocation_error(tokens);
        }

        // Get the next token
        token = lookup_token_in_env(strmbtok(NULL, " ", acOpen, acClose));
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
    // Count directory depth (i.e. number of parent folders)
    char * cwd_buffer = malloc(2048);
    getcwd(cwd_buffer, 2048);
    int i, count;
    for(i = 0, count = 0; cwd_buffer[i] != '\0'; (cwd_buffer[i] == '/')? count++: 0, i++);

    // Check this directory for a .git folder and all parents recursively
    DIR* dir;
    GIT_DIR_EXISTS = 0;
    for (i = 0; i < count - 1; i++) {
        dir = opendir(strcat(cwd_buffer, "/.git"));
        if (dir) {
            /* Directory exists. */
            closedir(dir);
            GIT_DIR_EXISTS = 1;
            cwd_depth_relative_to_git_root = i;
            break;
        }
        // chop off last folder and try parent folder
        char *lastslash = strrchr(cwd_buffer, '/');
        *lastslash = 0; // terminate / truncate the string (remove .git)
        lastslash = strrchr(cwd_buffer, '/');
        *lastslash = 0; // terminate / truncate the string (remove current directory)
    }

    free(cwd_buffer);
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

// Get the name of the current git branch
char * get_git_branch(int depth) {
    char * branch_path = malloc(DEFAULT_BUFFER_SIZE);
    char * git_head_path = malloc(DEFAULT_BUFFER_SIZE);
    int i;
    for (i = 0; i < depth; i++) {
        strcat(git_head_path, "../");
    }
    strcat(git_head_path, ".git/HEAD");
    FILE *fptr;

    fptr = fopen(git_head_path,"r");
    if (!fptr) {
        printf("Couldn't open HEAD file!\n");
    }
    fgets(branch_path, DEFAULT_BUFFER_SIZE, fptr);
    char *p = strrchr(branch_path, '/');
    p[strcspn(p, "\n")] = 0; // remove trailing newline char
    char *branch_name = malloc(DEFAULT_BUFFER_SIZE);
    // *branch_name = *(p + 1);
    strcpy(branch_name, p+1);
    fclose(fptr);
    free(git_head_path);
    free(branch_path);
    return branch_name;
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
        char * cwd_buffer = malloc(DEFAULT_BUFFER_SIZE);
        getcwd(cwd_buffer, DEFAULT_BUFFER_SIZE);
        printf("\x1b[1;36m");
        if (strcmp(cwd_buffer, getenv("HOME"))) {
            printf("%s ", basename(cwd_buffer));
        } else {
            printf("~ ");
        }

        if (GIT_DIR_EXISTS) {
            // Get the name of the branch
            char * branch_name = get_git_branch(cwd_depth_relative_to_git_root);
            printf("\x1b[1;34mgit:(\x1b[1;31m%s\x1b[1;34m) ", branch_name);
            free(branch_name);
        }


        printf("\x1b[1;32mkcsh-%s$\x1B[0m ", VERSION);
        line = kcsh_read_line();
        args = kcsh_split_line(line);

        // Pressed enter, no command
        if (!strcmp(*args, "")) {
            continue;
        }

        status = kcsh_execute(args);

        free(line);
        free(args);
        free(cwd_buffer);
    } while (status);
}

int main(int argc, char **argv) {
    // Load config files
    system("alias ls='ls -G'");

    // Initialize
    check_if_git_dir_exists();

    // Interpret in a loop
    kcsh_loop();

    // Shutdown
    return EXIT_SUCCESS;
}