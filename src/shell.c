#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

/*
 * read_cmd()
 * Uses GNU Readline for prompt, history navigation, and tab-completion default behavior.
 */
char* read_cmd(char* prompt, FILE* fp) {
    (void)fp; /* not used but kept for compatibility */

    char* cmdline = readline(prompt);
    if (cmdline == NULL) return NULL; /* Ctrl+D */

    /* add non-empty, non-whitespace commands to history */
    int only_ws = 1;
    for (char *p = cmdline; *p; ++p) {
        if (*p != ' ' && *p != '\t' && *p != '\n') { only_ws = 0; break; }
    }
    if (!only_ws && strlen(cmdline) > 0) add_history(cmdline);
    return cmdline;
}

/*
 * tokenize()
 * Tokenize one command string (no ';' splitting).
 * Produces an argv-like array. Caller must free each arg and the array.
 * Recognizes <, >, >> and & as separate tokens.
 */
char** tokenize(char* cmdline) {
    if (cmdline == NULL) return NULL;

    char** arglist = malloc(sizeof(char*) * (MAXARGS + 1));
    if (!arglist) return NULL;
    for (int i = 0; i < MAXARGS + 1; ++i) {
        arglist[i] = malloc(ARGLEN);
        if (arglist[i]) memset(arglist[i], 0, ARGLEN);
    }

    char* cp = cmdline;
    int argnum = 0;

    while (*cp != '\0' && argnum < MAXARGS) {
        while (*cp == ' ' || *cp == '\t') cp++;
        if (*cp == '\0' || *cp == '\n') break;

        if (*cp == '<') {
            strncpy(arglist[argnum++], "<", ARGLEN-1);
            cp++;
            continue;
        }
        if (*cp == '>') {
            if (*(cp+1) == '>') { strncpy(arglist[argnum++], ">>", ARGLEN-1); cp += 2; }
            else { strncpy(arglist[argnum++], ">", ARGLEN-1); cp++; }
            continue;
        }
        if (*cp == '&') {
            strncpy(arglist[argnum++], "&", ARGLEN-1);
            cp++;
            continue;
        }

        /* normal token */
        char* start = cp;
        int len = 0;
        while (*cp != '\0' && *cp != ' ' && *cp != '\t' && *cp != '<' && *cp != '>' && *cp != '&' && *cp != '\n') { cp++; len++; }
        if (len >= ARGLEN) len = ARGLEN-1;
        strncpy(arglist[argnum], start, len);
        arglist[argnum][len] = '\0';
        argnum++;
    }

    if (argnum == 0) {
        for (int i = 0; i < MAXARGS + 1; ++i) free(arglist[i]);
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}
