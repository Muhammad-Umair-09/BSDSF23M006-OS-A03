#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <strings.h> // bzero
#include <ctype.h>

/*
 * read_cmd()
 * Uses GNU Readline instead of manual input.
 * Supports command-line editing, history navigation (↑/↓), and tab completion.
 * Caller must free() the returned string.
 */
char* read_cmd(char* prompt, FILE* fp) {
    (void)fp; // unused

    char* cmdline = readline(prompt);

    /* Handle Ctrl+D (EOF) */
    if (cmdline == NULL)
        return NULL;

    /* If command is not empty, add to readline history (readline internal) */
    if (strlen(cmdline) > 0) {
        add_history(cmdline);
    }

    return cmdline; /* caller must free() this string (readline allocates heap memory) */
}

/*
 * tokenize()
 * Converts the command string into a dynamically allocated array of argument
 * strings for execvp(). Caller must free each element and the array itself.
 *
 * This tokenizer treats '<', '>', '>>', and '&' as separate tokens and splits on
 * whitespace. It does not implement full shell quoting/escaping — keep it simple
 * but safe for the assignment scope.
 */
char** tokenize(char* cmdline) {
    if (cmdline == NULL) return NULL;

    /* Skip leading whitespace */
    char* p = cmdline;
    while (*p && isspace((unsigned char)*p)) p++;
    if (*p == '\0') return NULL;

    int capacity = 16;
    char** arglist = malloc(sizeof(char*) * capacity);
    if (!arglist) return NULL;
    int argnum = 0;

    while (*p != '\0') {
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '\0' || *p == '\n') break;

        /* Handle special tokens */
        if (*p == '<' || *p == '>' || *p == '&' || *p == ';' || *p == '|') {
            char tok[3] = {0};
            if (*p == '>' && *(p + 1) == '>') {
                tok[0] = '>'; tok[1] = '>';
                p += 2;
            } else {
                tok[0] = *p;
                p++;
            }
            /* push token */
            if (argnum + 1 >= capacity) {
                capacity *= 2;
                arglist = realloc(arglist, sizeof(char*) * capacity);
            }
            arglist[argnum] = strdup(tok);
            if (!arglist[argnum]) break;
            argnum++;
            continue;
        }

        /* Regular argument */
        char buffer[ARGLEN];
        int len = 0;
        while (*p && !isspace((unsigned char)*p) && *p != '<' && *p != '>' && *p != '&' && *p != '|' && *p != ';' && *p != '\n') {
            if (len < ARGLEN - 1) buffer[len++] = *p;
            p++;
        }
        buffer[len] = '\0';
        if (len > 0) {
            if (argnum + 1 >= capacity) {
                capacity *= 2;
                arglist = realloc(arglist, sizeof(char*) * capacity);
            }
            arglist[argnum] = strdup(buffer);
            if (!arglist[argnum]) break;
            argnum++;
        }
    }

    if (argnum == 0) {
        free(arglist);
        return NULL;
    }

    /* NULL-terminate */
    arglist[argnum] = NULL;

    return arglist;
}
