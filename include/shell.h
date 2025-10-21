#ifndef SHELL_H
#define SHELL_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <signal.h>
#include <limits.h>

/* ---- History ---- */
#define HISTORY_SIZE 20
extern char *history_buf[HISTORY_SIZE];
extern int history_count;
extern int history_start;

/* ---- Constants ---- */
#define MAX_LEN 512
#define MAXARGS 64
#define ARGLEN  128
#define PROMPT "Umair> "

/* ---- Job structure ---- */
typedef struct {
    pid_t pid;
    char cmdline[MAX_LEN];
    int job_id;     /* 1-based id shown to user */
    int running;    /* 1 = running, 0 = stopped */
} Job;

#define MAX_JOBS 50
extern Job jobs[MAX_JOBS];
extern int job_count;

/* ---- Public APIs ---- */
char* read_cmd(char* prompt, FILE* fp);
char** tokenize(char* cmdline);            /* tokenizes a single command (no ';' splitting) */
int execute(char* arglist[], int background); /* run single command with background flag */

/* job control */
void add_job(pid_t pid, const char* cmdline);
void remove_job(pid_t pid);
void list_jobs(void);
int bring_fg(int job_id);
int resume_bg(int job_id);
void handle_sigchld(int sig);

/* helpers used in main.c */
int split_commands(char* line, char* commands[], int max_commands);

#endif // SHELL_H
