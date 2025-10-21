#include "shell.h"
#include <limits.h>
#include <fcntl.h>      // For open(), O_RDONLY, etc.
#include <sys/wait.h>   // For waitpid()
#include <unistd.h>     // For dup2(), close()

int execute(char* arglist[]) {
    if (arglist == NULL || arglist[0] == NULL)
        return 0;

    /* ---------- Built-in Commands ---------- */
    if (strcmp(arglist[0], "exit") == 0) {
        printf("Exiting shell...\n");
        exit(0);
    }

    if (strcmp(arglist[0], "cd") == 0) {
        if (arglist[1] == NULL) {
            fprintf(stderr, "cd: missing argument\n");
        } else {
            if (chdir(arglist[1]) != 0)
                perror("cd failed");
        }
        return 0;
    }

    if (strcmp(arglist[0], "pwd") == 0) {
        char cwd[1024];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("pwd failed");
        return 0;
    }

    if (strcmp(arglist[0], "echo") == 0) {
        for (int i = 1; arglist[i] != NULL; i++) {
            printf("%s", arglist[i]);
            if (arglist[i + 1] != NULL) printf(" ");
        }
        printf("\n");
        return 0;
    }

    if (strcmp(arglist[0], "history") == 0) {
        for (int i = 0; i < history_count; i++) {
            int idx = (history_start + i) % HISTORY_SIZE;
            printf("%d  %s\n", i + 1, history_buf[idx]);
        }
        return 0;
    }

    /* ---------- External Commands ---------- */
    pid_t cpid = fork();
    int status;

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);

        case 0: { /* Child process */

            int fd_in = -1, fd_out = -1;
            char* new_argv[MAXARGS + 1];
            int j = 0;

            // Scan for I/O redirection operators
            for (int i = 0; arglist[i] != NULL; i++) {
                if (strcmp(arglist[i], "<") == 0 && arglist[i + 1] != NULL) {
                    fd_in = open(arglist[i + 1], O_RDONLY);
                    if (fd_in < 0) {
                        perror("input redirection failed");
                        exit(1);
                    }
                    dup2(fd_in, STDIN_FILENO);
                    close(fd_in);
                    i++; // skip filename
                }
                else if (strcmp(arglist[i], ">") == 0 && arglist[i + 1] != NULL) {
                    fd_out = open(arglist[i + 1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
                    if (fd_out < 0) {
                        perror("output redirection failed");
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                    i++; // skip filename
                }
                else if (strcmp(arglist[i], ">>") == 0 && arglist[i + 1] != NULL) {
                    fd_out = open(arglist[i + 1], O_WRONLY | O_CREAT | O_APPEND, 0644);
                    if (fd_out < 0) {
                        perror("append redirection failed");
                        exit(1);
                    }
                    dup2(fd_out, STDOUT_FILENO);
                    close(fd_out);
                    i++; // skip filename
                }
                else {
                    // Regular command argument
                    new_argv[j++] = arglist[i];
                }
            }

            new_argv[j] = NULL;

            execvp(new_argv[0], new_argv);
            perror("Command not found");
            exit(1);
        }

        default: /* Parent process */
            waitpid(cpid, &status, 0);
            return 0;
    }
}
