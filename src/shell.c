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
 * 
 * Updated in Feature 5:
 *  - Detects I/O redirection symbols: <, >, >>
 *  - Treats them as separate tokens
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
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++; // Skip spaces
        if (*cp == '\0') break;

        // Handle redirection symbols separately
        if (*cp == '<' || *cp == '>') {
            if (*cp == '>' && *(cp + 1) == '>') {  // Handle >>
                strcpy(arglist[argnum++], ">>");
                cp += 2;
            } else {                               // Handle < or >
                arglist[argnum][0] = *cp;
                arglist[argnum][1] = '\0';
                argnum++;
                cp++;
            }
            continue;
        }

        // Regular argument
        char* start = cp;
        int len = 0;

        while (*cp != '\0' && *cp != ' ' && *cp != '\t' && *cp != '<' && *cp != '>') {
            cp++;
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
