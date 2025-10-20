#include "shell.h"

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
        return 0;  // no child forked
    }

    if (strcmp(arglist[0], "pwd") == 0) {
        char cwd[PATH_MAX];
        if (getcwd(cwd, sizeof(cwd)) != NULL)
            printf("%s\n", cwd);
        else
            perror("pwd failed");
        return 0;
    }

    if (strcmp(arglist[0], "echo") == 0) {
        for (int i = 1; arglist[i] != NULL; i++)
            printf("%s ", arglist[i]);
        printf("\n");
        return 0;
    }

    /* ---------- External Commands ---------- */
    int status;
    pid_t cpid = fork();

    switch (cpid) {
        case -1:
            perror("fork failed");
            exit(1);

        case 0: // Child process
            execvp(arglist[0], arglist);
            perror("Command not found");
            exit(1);

        default: // Parent waits
            waitpid(cpid, &status, 0);
            return 0;
    }
}
