#include "shell.h"
#include "variables.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>

/* ---- History buffer ---- */
char* history_buf[HISTORY_SIZE] = {0};
int history_count = 0;
int history_start = 0;

/* ---- Variable helpers (from feature-8) ---- */
typedef struct var_s {
    char* name;
    char* value;
    struct var_s* next;
} var_t;

static var_t* var_list = NULL;

void set_variable(const char* name, const char* value) {
    var_t* cur = var_list;
    while (cur) {
        if (strcmp(cur->name, name) == 0) {
            free(cur->value);
            cur->value = strdup(value);
            return;
        }
        cur = cur->next;
    }
    var_t* new_var = malloc(sizeof(var_t));
    new_var->name = strdup(name);
    new_var->value = strdup(value);
    new_var->next = var_list;
    var_list = new_var;
}

const char* get_variable(const char* name) {
    var_t* cur = var_list;
    while (cur) {
        if (strcmp(cur->name, name) == 0)
            return cur->value;
        cur = cur->next;
    }
    return NULL;
}

void print_variables(void) {
    var_t* cur = var_list;
    while (cur) {
        printf("%s=\"%s\"\n", cur->name, cur->value);
        cur = cur->next;
    }
}

void free_variables(void) {
    var_t* cur = var_list;
    while (cur) {
        var_t* next = cur->next;
        free(cur->name);
        free(cur->value);
        free(cur);
        cur = next;
    }
    var_list = NULL;
}

/* ---- History helpers ---- */
static void add_history_internal(const char* line) {
    if (!line || !*line) return;

    const char* p = line;
    while (*p) {
        if (*p != ' ' && *p != '\t' && *p != '\n') break;
        p++;
    }
    if (*p == '\0') return;

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

static const char* history_get_by_index(int n) {
    if (n < 1 || n > history_count) return NULL;
    int idx = (history_start + (n - 1)) % HISTORY_SIZE;
    return history_buf[idx];
}

/* ---- Pre-execution variable expansion ---- */
static void expand_variables(char** arglist) {
    for (int i = 0; arglist[i]; i++) {
        if (arglist[i][0] == '$' && strlen(arglist[i]) > 1) {
            const char* val = get_variable(arglist[i] + 1);
            if (val) {
                free(arglist[i]);
                arglist[i] = strdup(val);
            }
        }
    }
}

/* ---- Detect variable assignment ---- */
static int handle_assignment(char** arglist) {
    if (!arglist[0]) return 0;
    char* eq = strchr(arglist[0], '=');
    if (!eq) return 0;

    /* no spaces allowed around = */
    int idx = eq - arglist[0];
    if (idx == 0) return 0; // = at start is invalid

    *eq = '\0';
    char* name = arglist[0];
    char* value = eq + 1;

    /* remove surrounding quotes if present */
    if (value[0] == '"' && value[strlen(value) - 1] == '"') {
        value[strlen(value) - 1] = '\0';
        value++;
    }

    set_variable(name, value);
    return 1;
}

/* ---- Main Shell ---- */
int main() {
    char* cmdline;
    char** arglist;

    /* Put shell in its own process group and take control of terminal */
    pid_t shell_pid = getpid();
    setpgid(shell_pid, shell_pid);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, handle_sigchld);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    tcsetpgrp(STDIN_FILENO, shell_pid);

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {

        char* trim = cmdline;
        while (*trim && (*trim == ' ' || *trim == '\t')) trim++;

        /* ---- Handle !n history recall ---- */
        if (trim[0] == '!' && trim[1] != '\0') {
            int n = atoi(&trim[1]);
            const char* entry = history_get_by_index(n);
            if (!entry) {
                fprintf(stderr, "Error: invalid history reference !%d\n", n);
                free(cmdline);
                continue;
            }
            free(cmdline);
            cmdline = strdup(entry);
            printf("%s\n", cmdline);
        }

        /* ---- Add to history ---- */
        if (strlen(cmdline) > 0)
            add_history_internal(cmdline);

        /* ---- Tokenize ---- */
        if ((arglist = tokenize(cmdline)) != NULL) {

            /* ---- Handle background '&' ---- */
            int background = 0;
            int i = 0; while (arglist[i]) i++;
            if (i > 0 && strcmp(arglist[i - 1], "&") == 0) {
                background = 1;
                free(arglist[i - 1]);
                arglist[i - 1] = NULL;
            }

            /* ---- Variable assignment ---- */
            if (!handle_assignment(arglist)) {
                /* ---- Variable expansion ---- */
                expand_variables(arglist);

                /* ---- Built-in 'history' ---- */
                if (arglist[0] && strcmp(arglist[0], "history") == 0) {
                    for (int h = 0; h < history_count; ++h) {
                        int idx = (history_start + h) % HISTORY_SIZE;
                        printf("%d  %s\n", h + 1, history_buf[idx]);
                    }
                }
                /* ---- Built-in 'set' ---- */
                else if (arglist[0] && strcmp(arglist[0], "set") == 0) {
                    print_variables();
                }
                else {
                    /* ---- Execute Command ---- */
                    int exit_flag = execute(arglist, background);
                    if (exit_flag) {
                        for (int j = 0; arglist[j]; j++) free(arglist[j]);
                        free(arglist);
                        free(cmdline);
                        break;
                    }
                }
            }

            for (int j = 0; arglist[j]; j++) free(arglist[j]);
            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");

    for (int i = 0; i < history_count; i++) {
        int idx = (history_start + i) % HISTORY_SIZE;
        free(history_buf[idx]);
    }

    free_variables();

    return 0;
}
