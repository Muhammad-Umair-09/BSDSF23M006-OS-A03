#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <strings.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration of variable functions */
extern const char* get_variable(const char* name);

/*
 * read_cmd()
 * Uses GNU Readline.
 * Caller must free() the returned string.
 */
char* read_cmd(char* prompt, FILE* fp) {
    (void)fp; // unused
    char* cmdline = readline(prompt);

    if (!cmdline)
        return NULL;

    if (strlen(cmdline) > 0) add_history(cmdline);

    return cmdline;
}

/*
 * tokenize()
 * Splits the input line into arguments.
 * Handles quoted strings as single tokens.
 */
char** tokenize(const char* line) {
    if (!line) return NULL;

    int capacity = 16;
    char** arglist = malloc(sizeof(char*) * capacity);
    if (!arglist) return NULL;
    int argnum = 0;

    const char* p = line;
    while (*p) {
        while (*p && isspace((unsigned char)*p)) p++;
        if (!*p) break;

        char buffer[ARGLEN];
        int len = 0;

        if (*p == '"') {
            // Quoted string
            p++; // skip opening quote
            while (*p && *p != '"') {
                if (len < ARGLEN - 1) buffer[len++] = *p;
                p++;
            }
            if (*p == '"') p++; // skip closing quote
        } else if (*p == '<' || *p == '>' || *p == '&' || *p == ';' || *p == '|') {
            // Special single or double character tokens
            if (*p == '>' && *(p+1) == '>') {
                buffer[0] = '>'; buffer[1] = '>'; buffer[2] = '\0';
                p += 2;
            } else {
                buffer[0] = *p;
                buffer[1] = '\0';
                p++;
            }
        } else {
            // Normal argument
            while (*p && !isspace((unsigned char)*p) && *p != '<' && *p != '>' &&
                   *p != '&' && *p != '|' && *p != ';' && *p != '\n') {
                if (len < ARGLEN - 1) buffer[len++] = *p;
                p++;
            }
        }

        buffer[len] = '\0';
        if (len > 0) {
            if (argnum + 1 >= capacity) {
                capacity *= 2;
                arglist = realloc(arglist, sizeof(char*) * capacity);
            }
            arglist[argnum++] = strdup(buffer);
        }
    }

    if (argnum == 0) {
        free(arglist);
        return NULL;
    }

    arglist[argnum] = NULL;
    return arglist;
}

/*
 * expand_variables()
 * Replaces any argument starting with '$' with its value.
 */
void expand_variables(char** arglist) {
    if (!arglist) return;
    for (int i = 0; arglist[i]; i++) {
        if (arglist[i][0] == '$' && strlen(arglist[i]) > 1) {
            const char* val = get_variable(arglist[i]+1);
            if (val) {
                free(arglist[i]);
                arglist[i] = strdup(val);
            } else {
                free(arglist[i]);
                arglist[i] = strdup(""); // unset variable expands to empty string
            }
        }
    }
}
