#include "shell.h"

/* History storage (defined here) */
char *history_buf[HISTORY_SIZE] = {0};
int history_count = 0;   /* current number of entries stored (<= HISTORY_SIZE) */
int history_start = 0;   /* index of oldest entry */

static void add_to_history(const char *line) {
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

/* Helper: get history entry by 1-based index in the printed list */
static const char* history_get_by_index(int n) {
    if (n < 1 || n > history_count) return NULL;
    int idx = (history_start + (n - 1)) % HISTORY_SIZE;
    return history_buf[idx];
}

int main() {
    char* cmdline;
    char** arglist;

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        /* Handle EOF (Ctrl+D) already done in read_cmd */

        /* Trim leading spaces for easier '!' detection */
        char *trim = cmdline;
        while (*trim == ' ' || *trim == '\t') trim++;

        /* Handle !n syntax BEFORE adding to history and BEFORE tokenization */
        if (trim[0] == '!' && trim[1] != '\0') {
            int n = atoi(&trim[1]); /* if not a number, atoi returns 0 */
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
                /* Replace cmdline with strdup of the retrieved command */
                free(cmdline);
                cmdline = strdup(entry);
                if (!cmdline) {
                    perror("strdup");
                    continue;
                }
            }
        }

        /* Now add the (possibly expanded) command to history */
        add_to_history(cmdline);

        /* Tokenize and execute */
        if ((arglist = tokenize(cmdline)) != NULL) {
            execute(arglist);

            /* Free the memory allocated by tokenize() */
            for (int i = 0; arglist[i] != NULL; i++) {
                free(arglist[i]);
            }
            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    /* free history on exit */
    for (int i = 0; i < history_count; i++) {
        int idx = (history_start + i) % HISTORY_SIZE;
        free(history_buf[idx]);
    }
    return 0;
}
