#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <limits.h>


/* ---- History ---- */
#define HISTORY_SIZE 20
extern char *history_buf[HISTORY_SIZE];
extern int history_count;   /* number of entries currently stored (<= HISTORY_SIZE) */
extern int history_start;   /* index of the oldest entry in circular buffer */

#define MAX_LEN 512
#define MAXARGS 10
#define ARGLEN 30
#define PROMPT "Umair> "

// Function prototypes
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);
int execute(char** arglist);

#endif // SHELL_H
