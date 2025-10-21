#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

/*
 * read_cmd()
 * Uses GNU Readline instead of manual input.
 * Supports command-line editing, history navigation (↑/↓), and tab completion.
 */
char* read_cmd(char* prompt, FILE* fp) {
    // Read user input using readline()
    char* cmdline = readline(prompt);

    // Handle Ctrl+D (EOF)
    if (cmdline == NULL)
        return NULL;

    // If command is not empty, add to history
    if (strlen(cmdline) > 0) {
        add_history(cmdline);
    }

    return cmdline;
}

/*
 * tokenize()
 * Converts the command string into an array of arguments for execvp().
 */
char** tokenize(char* cmdline) {
    if (cmdline == NULL || cmdline[0] == '\0' || cmdline[0] == '\n') {
        return NULL;
    }

    char** arglist = (char**) malloc(sizeof(char*) * (MAXARGS + 1));
    for (int i = 0; i < MAXARGS + 1; i++) {
        arglist[i] = (char*) malloc(sizeof(char) * ARGLEN);
        bzero(arglist[i], ARGLEN);
    }

    char* cp = cmdline;
    char* start;
    int len;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip spaces

        if (*cp == '\0') break;

        start = cp;
        len = 1;
        while (*++cp != '\0' && !(*cp == ' ' || *cp == '\t')) {
            len++;
        }

        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; i++) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
