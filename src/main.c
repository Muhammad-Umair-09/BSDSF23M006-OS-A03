#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <ctype.h>

char *history_buf[HISTORY_SIZE] = {0};
int history_count = 0;
int history_start = 0;

/* split_commands: split by ';' into commands[], returns count */
int split_commands(char* line, char* commands[], int max_commands) {
    int count = 0;
    char* saveptr = NULL;
    char* tok = strtok_r(line, ";", &saveptr);
    while (tok && count < max_commands) {
        /* trim leading/trailing spaces */
        while (isspace((unsigned char)*tok)) tok++;
        char* end = tok + strlen(tok) - 1;
        while (end > tok && isspace((unsigned char)*end)) { *end = '\0'; end--; }
        commands[count++] = strdup(tok);
        tok = strtok_r(NULL, ";", &saveptr);
    }
    commands[count] = NULL;
    return count;
}

/* Add to history (circular buffer) */
static void add_to_history_local(const char *line) {
    if (!line) return;
    /* skip whitespace-only */
    const char *p = line;
    while (*p && isspace((unsigned char)*p)) p++;
    if (!*p) return;

    char* copy = strdup(line);
    if (!copy) return;

    if (history_count < HISTORY_SIZE) {
        int idx = (history_start + history_count) % HISTORY_SIZE;
        history_buf[idx] = copy;
        history_count++;
    } else {
        free(history_buf[history_start]);
        history_buf[history_start] = copy;
        history_start = (history_start + 1) % HISTORY_SIZE;
    }
}

/* get history by 1-based printed index */
static const char* history_get_by_index(int n) {
    if (n < 1 || n > history_count) return NULL;
    int idx = (history_start + (n - 1)) % HISTORY_SIZE;
    return history_buf[idx];
}

void handle_sigchld_wrapper(int sig) { handle_sigchld(sig); }

int main() {
    /* setup SIGCHLD handler */
    struct sigaction sa;
    sa.sa_handler = handle_sigchld_wrapper;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART | SA_NOCLDSTOP;
    sigaction(SIGCHLD, &sa, NULL);

    char* line_raw = NULL;
    while ((line_raw = read_cmd(PROMPT, stdin)) != NULL) {
        /* handle empty */
        if (line_raw[0] == '\0') { free(line_raw); continue; }

        /* Support !n history recall (before adding to history) */
        char* trimmed = line_raw;
        while (*trimmed && isspace((unsigned char)*trimmed)) trimmed++;

        if (*trimmed == '!' && isdigit((unsigned char)trimmed[1])) {
            int n = atoi(trimmed + 1);
            const char* h = history_get_by_index(n);
            if (!h) {
                fprintf(stderr, "Error: no such history entry !%d\n", n);
                free(line_raw);
                continue;
            }
            free(line_raw);
            line_raw = strdup(h);
            if (!line_raw) { perror("strdup"); continue; }
            printf("%s\n", line_raw); /* echo the expanded command */
        }

        /* Add to history circular buffer (not readline history) */
        add_to_history_local(line_raw);

        /* split by ';' */
        char* commands[MAXARGS];
        int ncmds = split_commands(line_raw, commands, MAXARGS);

        for (int ci = 0; ci < ncmds; ++ci) {
            char* single = commands[ci];
            /* tokenize single command */
            char** argv = tokenize(single);
            if (!argv) { free(single); continue; }

            /* detect background if last token is '&' */
            int background = 0;
            int last = 0;
            while (argv[last] != NULL) last++;
            if (last > 0 && strcmp(argv[last-1], "&") == 0) {
                background = 1;
                free(argv[last-1]);
                argv[last-1] = NULL;
            }

            /* built-ins handled here */
            if (argv[0] == NULL) { /* nothing */ }
            else if (strcmp(argv[0], "exit") == 0) {
                /* free memory */
                for (int i = 0; argv[i]; ++i) free(argv[i]);
                free(argv);
                free(single);
                goto cleanup_and_exit;
            }
            else if (strcmp(argv[0], "jobs") == 0) {
                list_jobs();
            }
            else if (strcmp(argv[0], "fg") == 0) {
                if (argv[1]) bring_fg(atoi(argv[1])); else fprintf(stderr, "fg: usage fg <jobid>\n");
            }
            else if (strcmp(argv[0], "bg") == 0) {
                if (argv[1]) resume_bg(atoi(argv[1])); else fprintf(stderr, "bg: usage bg <jobid>\n");
            }
            else {
                execute(argv, background);
            }

            for (int i = 0; argv[i]; ++i) free(argv[i]);
            free(argv);
            free(single);
        }

        free(line_raw);
    }

cleanup_and_exit:
    printf("\nExiting shell...\n");
    /* free history */
    for (int i = 0; i < history_count; ++i) {
        int idx = (history_start + i) % HISTORY_SIZE;
        free(history_buf[idx]);
    }
    return 0;
}
