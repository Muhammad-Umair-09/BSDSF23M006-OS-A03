#include "shell.h"
#include <readline/readline.h>
#include <readline/history.h>
#include <strings.h>
#include <ctype.h>

/* ---- History storage (defined here) ---- */
char *history_buf[HISTORY_SIZE] = {0};
int history_count = 0;   
int history_start = 0;   

static void add_history_internal(const char *line) {
    if (line == NULL || *line == '\0') return;
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

/* ---- Execute a list of commands (array of strings) ---- */
static void execute_command_list(char **cmds, int count) {
    for (int i = 0; i < count; i++) {
        char **args = tokenize(cmds[i]);
        if (args) {
            int background = 0;
            int j = 0;
            while (args[j] != NULL) j++;
            if (j > 0 && strcmp(args[j - 1], "&") == 0) {
                background = 1;
                free(args[j - 1]);
                args[j - 1] = NULL;
            }
            execute(args, background);
            for (int k = 0; args[k] != NULL; k++) free(args[k]);
            free(args);
        }
    }
}

/* ---- Main Shell ---- */
int main() {
    char* cmdline;

    pid_t shell_pid = getpid();
    if (setpgid(shell_pid, shell_pid) < 0) {}
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTTIN, SIG_IGN);
    signal(SIGCHLD, handle_sigchld);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGINT, SIG_IGN);
    if (tcsetpgrp(STDIN_FILENO, shell_pid) < 0) {}

    while ((cmdline = read_cmd(PROMPT, stdin)) != NULL) {
        char *trim = cmdline;
        while (*trim == ' ' || *trim == '\t') trim++;

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
                printf("%s\n", cmdline);
            }
        }

        if (strlen(cmdline) > 0) add_history_internal(cmdline);

        /* ---- Feature-7: Handle if-then-else-fi ---- */
        if (strncmp(trim, "if", 2) == 0 && isspace((unsigned char)trim[2])) {
            /* Read lines until 'fi' */
            char *if_cmd = NULL;
            char *then_block[64] = {0};
            char *else_block[64] = {0};
            int then_count = 0, else_count = 0;
            int in_else = 0;

            /* Parse if line */
            if_cmd = strdup(trim + 2); // skip "if "
            if (!if_cmd) continue;

            while (1) {
                char *line = read_cmd(">> ", stdin);
                if (!line) break;
                /* Trim leading spaces */
                char *ltrim = line;
                while (*ltrim && isspace((unsigned char)*ltrim)) ltrim++;

                if (strncmp(ltrim, "then", 4) == 0) { free(line); continue; }
                if (strncmp(ltrim, "else", 4) == 0) { in_else = 1; free(line); continue; }
                if (strncmp(ltrim, "fi", 2) == 0) { free(line); break; }

                if (!in_else) then_block[then_count++] = strdup(ltrim);
                else else_block[else_count++] = strdup(ltrim);
                free(line);
            }

            /* Execute if command */
            char **args = tokenize(if_cmd);
            int status = execute(args, 0); /* foreground */
            int exit_code = 0;
            if (args) {
                free(args[0]);
                free(args);
            }

            /* Retrieve actual exit code via fork-wait hack */
            pid_t pid = fork();
            if (pid == 0) {
                char **ifargs = tokenize(if_cmd);
                if (ifargs) {
                    execvp(ifargs[0], ifargs);
                    exit(1);
                }
                exit(1);
            } else if (pid > 0) {
                int wstatus;
                waitpid(pid, &wstatus, 0);
                exit_code = WEXITSTATUS(wstatus);
            }

            if (exit_code == 0) execute_command_list(then_block, then_count);
            else execute_command_list(else_block, else_count);

            /* Free allocated memory */
            free(if_cmd);
            for (int i = 0; i < then_count; i++) free(then_block[i]);
            for (int i = 0; i < else_count; i++) free(else_block[i]);
            free(cmdline);
            continue;
        }

        char **arglist = tokenize(cmdline);
        if (arglist != NULL) {
            int background = 0;
            int i = 0;
            while (arglist[i] != NULL) i++;
            if (i > 0 && strcmp(arglist[i - 1], "&") == 0) {
                background = 1;
                free(arglist[i - 1]);
                arglist[i - 1] = NULL;
            }

            if (arglist[0] && strcmp(arglist[0], "history") == 0) {
                for (int h = 0; h < history_count; ++h) {
                    int idx = (history_start + h) % HISTORY_SIZE;
                    printf("%d  %s\n", h + 1, history_buf[idx]);
                }
            } else {
                int exit_flag = execute(arglist, background);
                if (exit_flag) {
                    for (int j = 0; arglist[j] != NULL; j++) free(arglist[j]);
                    free(arglist);
                    free(cmdline);
                    break;
                }
            }

            for (int j = 0; arglist[j] != NULL; j++) free(arglist[j]);
            free(arglist);
        }

        free(cmdline);
    }

    printf("\nShell exited.\n");
    for (int i = 0; i < history_count; i++) {
        int idx = (history_start + i) % HISTORY_SIZE;
        free(history_buf[idx]);
    }

    return 0;
}
