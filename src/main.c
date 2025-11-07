#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>

/* ---- History storage (defined here) ---- */
char *history_buf[HISTORY_SIZE] = {0};
int history_count = 0;   /* current number of entries stored (<= HISTORY_SIZE) */
int history_start = 0;   /* index of oldest entry */

/* ---- Helper: Add to internal circular history buffer ---- */
static void add_history_internal(const char *line) {
    if (line == NULL || *line == '\0') return;

    /* Skip adding pure whitespace lines */
    const char *p = line;
    while (*p != '\0') {
        if (*p != ' ' && *p != '\t' && *p != '\n') break;
        p++;
    }
    if (*p == '\0') return;

    char *copy = strdup(line);
    if (!copy) return;

    if (history_count < HISTORY_SIZE) {
        int idx = (history_start + history_count) % HISTORY_SIZE;
        history_buf[idx] = copy;
        history_count++;
    } else {
        /* overwrite oldest */
        free(history_buf[history_start]);
        history_buf[history_start] = copy;
        history_start = (history_start + 1) % HISTORY_SIZE;
    }
}

/* ---- Helper: get history entry by 1-based index ---- */
static const char* history_get_by_index(int n) {
    if (n < 1 || n > history_count) return NULL;
    int idx = (history_start + (n - 1)) % HISTORY_SIZE;
    return history_buf[idx];
}

/* ---- Main Shell ---- */
int main() {
    char* cmdline;
    char** arglist;

    /* Put shell in its own process group and take control of terminal */
    pid_t shell_pid = getpid();
    if (setpgid(shell_pid, shell_pid) < 0) {
        // perror("setpgid (shell)");
    }
    /* ignore terminal background read/write signals so tcsetpgrp won't stop us */
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);

    /* Register signal handlers */
    signal(SIGCHLD, handle_sigchld);
    /* Shell ignores SIGTSTP and SIGINT so it doesn't get stopped/killed by Ctrl-Z/Ctrl-C */
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);

    /* Ensure shell owns terminal */
    if (tcsetpgrp(STDIN_FILENO, shell_pid) < 0) {
        // perror("tcsetpgrp (init)");
    }

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        /* Trim leading spaces for easier '!n' detection */
        char *trim = cmdline;
        while (*trim == ' ' || *trim == '\t') trim++;

        /* ---- Handle !n syntax (history recall) ---- */
        if (trim[0] == '!' && trim[1] != '\0') {
            int n = atoi(&trim[1]);
            if (n <= 0 || n > history_count) {
                fprintf(stderr, "Error: invalid history reference !%d\n", n);
                free(cmdline);
                continue;
            } else {
                const char *entry = history_get_by_index(n);
                if (!entry) {
                    fprintf(stderr, "Error: history entry not found\n");
                    free(cmdline);
                    continue;
                }
                free(cmdline);
                cmdline = strdup(entry);
                if (!cmdline) {
                    perror("strdup");
                    continue;
                }
                printf("%s\n", cmdline); // Echo recalled command
            }
        }

        /* ---- Add to history (both internal circular buffer and readline history) ---- */
        if (strlen(cmdline) > 0) {
            add_history_internal(cmdline);
            /* readline's add_history is called inside read_cmd when using readline,
               but if you ever obtain input by other means you can call add_history(cmdline) */
        }

        /* ---- Tokenize ---- */
        if ((arglist = tokenize(cmdline)) != NULL) {

            /* ---- Detect background process (&) ---- */
            int background = 0;
            int i = 0;
            while (arglist[i] != NULL) i++;
            if (i > 0 && strcmp(arglist[i - 1], "&") == 0) {
                background = 1;
                free(arglist[i - 1]);        // free the token storage
                arglist[i - 1] = NULL;      // Remove '&' from argument list
            }

            /* ---- Detect builtin 'history' to print internal history buffer ---- */
            if (arglist[0] && strcmp(arglist[0], "history") == 0) {
                for (int h = 0; h < history_count; ++h) {
                    int idx = (history_start + h) % HISTORY_SIZE;
                    printf("%d  %s\n", h + 1, history_buf[idx]);
                }
            } else {
                /* ---- Execute Command ---- */
                int exit_flag = execute(arglist, background);
                if (exit_flag) {
                    /* Free tokenized args */
                    for (int j = 0; arglist[j] != NULL; j++) {
                        free(arglist[j]);
                    }
                    free(arglist);
                    free(cmdline);
                    break;
                }
            }

            /* ---- Free tokenized args ---- */
            for (int j = 0; arglist[j] != NULL; j++) {
                free(arglist[j]);
            }
            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");

    /* ---- Free internal history buffer ---- */
    for (int i = 0; i < history_count; i++) {
        int idx = (history_start + i) % HISTORY_SIZE;
        free(history_buf[idx]);
    }

    return 0;
}
