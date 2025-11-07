#include "shell.h"
#include <sys/types.h>
#include <signal.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>

/* Execute a single command, optionally in background.
 * Return 1 -> tell main to exit the shell (exit builtin),
 * otherwise 0 to continue.
 */
int execute(char* arglist[], int background) {
    pid_t pid;
    int status;

    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    // ---------- Built-in commands ----------
    if (strcmp(arglist[0], "jobs") == 0) {
        print_jobs();
        return 0;
    }

    if (strcmp(arglist[0], "fg") == 0) {
        if (arglist[1])
            bring_job_to_foreground(atoi(arglist[1]));
        else
            fprintf(stderr, "Usage: fg <job_id>\n");
        return 0;
    }

    if (strcmp(arglist[0], "bg") == 0) {
        if (arglist[1])
            continue_job_in_background(atoi(arglist[1]));
        else
            fprintf(stderr, "Usage: bg <job_id>\n");
        return 0;
    }

    if (strcmp(arglist[0], "exit") == 0) {
        /* Signal main loop to terminate */
        return 1;
    }

    // ---------- External commands ----------
    pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return 0;
    }

    if (pid == 0) {
        // ---- Child process ----

        /* Put child into new process group (group leader = child pid) */
        if (setpgid(0, 0) < 0) {
            /* Not fatal, but warn */
            perror("setpgid (child)");
        }

        /* If foreground, give terminal to child's process group */
        if (!background) {
            if (tcsetpgrp(STDIN_FILENO, getpid()) < 0) {
                /* It can fail in some environments; warn but continue */
                // perror("tcsetpgrp (child)");
            }
        }

        /* restore default signals for child */
        signal(SIGINT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);

        /* execute the requested program */
        execvp(arglist[0], arglist);
        /* only reach here on failure */
        fprintf(stderr, "myshell: command not found: %s\n", arglist[0]);
        exit(EXIT_FAILURE);
    } else {
        // ---- Parent process ----

        /* Ensure child is in its own group (do it in parent too to avoid race) */
        if (setpgid(pid, pid) < 0 && errno != EACCES) {
            /* sometimes EACCES can be returned if child already set pgid */
            // perror("setpgid (parent)");
        }

        if (background) {
            add_job(pid, arglist[0], 1);
            printf("[BG] Started job %d with PID %d: %s\n", job_count, (int)pid, arglist[0]);
            /* parent returns to prompt immediately */
        } else {
            /* Foreground: give terminal control to child's process group */
            if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
                // perror("tcsetpgrp (parent)");
            }

            /* Wait for the foreground child; handle stopped children */
            pid_t w;
            do {
                w = waitpid(pid, &status, WUNTRACED);
            } while (w == -1 && errno == EINTR);

            /* Return terminal control to shell (our process group) */
            if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
                // perror("tcsetpgrp (restore shell)");
            }

            if (w == -1) {
                perror("waitpid");
            } else {
                if (WIFSTOPPED(status)) {
                    /* Child was stopped (Ctrl+Z) - add to jobs list as stopped */
                    add_job(pid, arglist[0], 0);
                    update_job_status(pid, "Stopped");
                    printf("[STOPPED] Job %d stopped: %s (pid %d)\n", job_count, arglist[0], (int)pid);
                } else if (WIFEXITED(status) || WIFSIGNALED(status)) {
                    /* Finished normally or terminated by signal - ensure job removed */
                    remove_job(pid); /* remove if present */
                }
            }
        }
    }

    return 0; /* Continue main loop */
}
