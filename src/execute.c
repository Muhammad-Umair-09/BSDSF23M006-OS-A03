#include "shell.h"
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Forward declaration for variable expansion functions */
const char* get_variable(const char* name);
void set_variable(const char* name, const char* value);
void print_variables(void);

/* Execute a single command, optionally in background.
 * Return 1 -> tell main to exit the shell (exit builtin),
 * otherwise 0 to continue.
 */
int execute(char* arglist[], int background) {
    pid_t pid;
    int status;

    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    // ---------- Built-in: history ----------
    if (strcmp(arglist[0], "history") == 0) {
        for (int h = 0; h < history_count; ++h) {
            int idx = (history_start + h) % HISTORY_SIZE;
            printf("%d  %s\n", h + 1, history_buf[idx]);
        }
        return 0;
    }

    // ---------- Built-in: jobs ----------
    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 0;
    }

    // ---------- Built-in: fg ----------
    if (strcmp(arglist[0], "fg") == 0) {
        if (arglist[1])
            bring_job_to_foreground(atoi(arglist[1]));
        else
            fprintf(stderr, "Usage: fg <job_id>\n");
        return 0;
    }

    // ---------- Built-in: bg ----------
    if (strcmp(arglist[0], "bg") == 0) {
        if (arglist[1])
            continue_job_in_background(atoi(arglist[1]));
        else
            fprintf(stderr, "Usage: bg <job_id>\n");
        return 0;
    }

    // ---------- Built-in: exit ----------
    if (strcmp(arglist[0], "exit") == 0) {
        return 1;
    }

    // ---------- Built-in: set (print variables) ----------
    if (strcmp(arglist[0], "set") == 0) {
        print_variables();
        return 0;
    }

    // ---------- Detect variable assignment ----------
    if (strchr(arglist[0], '=') != NULL) {
        char* name = strtok(arglist[0], "=");
        char* value = strtok(NULL, "");

        if (!name) return 0;
        if (!value) value = "";

        // Handle quotes
        if (value[0] == '"' && value[strlen(value) - 1] == '"') {
            value[strlen(value) - 1] = '\0'; // remove closing quote
            value++; // skip opening quote
        }

        set_variable(name, value);
        return 0; // do not execute as external command
    }

    // ---------- Variable expansion for args ----------
    for (int i = 0; arglist[i] != NULL; i++) {
        if (arglist[i][0] == '$') {
            const char* val = get_variable(arglist[i] + 1);
            free(arglist[i]);
            arglist[i] = strdup(val ? val : "");
        }
    }

    // ---------- External commands ----------
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 0;
    }

    if (pid == 0) {
        // ---- Child process ----

        if (setpgid(0, 0) < 0) perror("setpgid (child)");

        if (!background) {
            if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) ;
        }

        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        execvp(arglist[0], arglist);
        fprintf(stderr, "myshell: command not found: %s\n", arglist[0]);
        exit(EXIT_FAILURE);

    } else {
        // ---- Parent process ----
        if (setpgid(pid, pid) < 0 && errno != EACCES) ;

        if (background) {
            add_job(pid, arglist[0], 1);
            printf("[BG] Started job %d with PID %d: %s\n", job_count, (int)pid, arglist[0]);
        } else {
            if (tcsetpgrp(STDIN_FILENO, pid) < 0) ;

            pid_t w;
            do {
                w = waitpid(pid, &status, WUNTRACED);
            } while (w == -1 && errno == EINTR);

            if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) ;

            if (w == -1) perror("waitpid");
            else {
                if (WIFSTOPPED(status)) {
                    add_job(pid, arglist[0], 0);
                    update_job_status(pid, "Stopped");
                    printf("[STOPPED] Job %d stopped: %s (pid %d)\n", job_count, arglist[0], (int)pid);
                } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    remove_job(pid);
                }
            }
        }
    }

    return 0;
}
