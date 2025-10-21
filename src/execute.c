#include "shell.h"
#include <limits.h>
#include <fcntl.h>      // open flags
#include <sys/wait.h>
#include <unistd.h>     // dup2, execvp

int execute(char* arglist[], int background) {
    if (arglist == NULL || arglist[0] == NULL) return 0;

    /* Built-ins handled here if desired (some handled in main) */
    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) fprintf(stderr, "cd: missing argument\n");
        else if (chdir(arglist[1]) != 0) perror("cd failed");
        return 0;
    }
    if (strcmp(arglist[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd))) printf("%s\n", cwd);
        else perror("pwd failed");
        return 0;
    }
    if (strcmp(arglist[0], "echo") == 0) {
        for (int i = 1; arglist[i] != NULL; ++i) {
            printf("%s", arglist[i]);
            if (arglist[i+1]) printf(" ");
        }
        printf("\n");
        return 0;
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork failed");
        return -1;
    }

    if (pid == 0) { /* child */
        /* Put child in its own process group to manage signals for fg/bg */
        setpgid(0, 0);

        /* Handle I/O redirection: build new_argv excluding redirection tokens */
        char* new_argv[MAXARGS + 1];
        int j = 0;
        for (int i = 0; arglist[i] != NULL; ++i) {
            if (strcmp(arglist[i], "<") == 0) {
                if (arglist[i+1]) {
                    int fd = open(arglist[i+1], O_RDONLY);
                    if (fd < 0) { perror("input redirection"); exit(1); }
                    dup2(fd, STDIN_FILENO); close(fd);
                    i++; continue;
                }
            } else if (strcmp(arglist[i], ">") == 0) {
                if (arglist[i+1]) {
                    int fd = open(arglist[i+1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd < 0) { perror("output redirection"); exit(1); }
                    dup2(fd, STDOUT_FILENO); close(fd);
                    i++; continue;
                }
            } else if (strcmp(arglist[i], ">>") == 0) {
                if (arglist[i+1]) {
                    int fd = open(arglist[i+1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd < 0) { perror("append redirection"); exit(1); }
                    dup2(fd, STDOUT_FILENO); close(fd);
                    i++; continue;
                }
            }
            new_argv[j++] = arglist[i];
        }
        new_argv[j] = NULL;

        execvp(new_argv[0], new_argv);
        perror("Command not found");
        exit(1);
    } else { /* parent */
        /* Put child in its own process group (ensure setpgid for parent too) */
        setpgid(pid, pid);

        if (background) {
            add_job(pid, arglist[0]);
            /* don't wait */
            return 0;
        } else {
            int status = 0;
            if (waitpid(pid, &status, WUNTRACED) == -1) perror("waitpid");
            /* if stopped (WIFSTOPPED), would keep job; for simplicity stop handling omitted here */
            return 0;
        }
    }
}
