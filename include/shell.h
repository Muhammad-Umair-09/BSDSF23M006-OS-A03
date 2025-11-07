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

/* ---------------- History ---------------- */
#define HISTORY_SIZE 20
extern char *history_buf[HISTORY_SIZE];
extern int history_count;
extern int history_start;

/* ---------------- Core constants ---------------- */
#define MAX_LEN   512
#define MAXARGS   64
#define ARGLEN    128
#define PROMPT    "Umair> "

/* ---------------- Job control ---------------- */
typedef struct {
    pid_t pid;
    int   job_id;
    char  cmdline[MAX_LEN];
    char  status[16];
    int   background;
} job_t;

#define MAX_JOBS 64
extern job_t jobs[MAX_JOBS];
extern int job_count;

/* ---------------- Function prototypes ---------------- */

/* Input / parsing */
char*  read_cmd(char* prompt, FILE* fp);
char** tokenize(const char* cmdline);


/* Execution */
int    execute(char* arglist[], int background);

/* Job management helpers */
void   add_job(pid_t pid, const char* cmdline, int background);
void   update_job_status(pid_t pid, const char* status);
void   print_jobs(void);
int    bring_job_to_foreground(int job_id);
int    continue_job_in_background(int job_id);
void   remove_job(pid_t pid);
int    job_exists(pid_t pid);

/* Signal handlers */
void   handle_sigchld(int sig);
void   handle_sigtstp(int sig);

/* Variables (Feature 8) */
void   set_variable(const char* name, const char* value);
const char* get_variable(const char* name);
void   print_variables(void);
void   free_variables(void);

#endif
