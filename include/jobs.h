/* include/jobs.h */
#ifndef JOBS_H
#define JOBS_H

#include <sys/types.h>

/* ---- Job Structure ---- */
typedef struct {
    pid_t pid;
    char name[256];
    int job_id;
    int running; /* 1 = running, 0 = stopped */
} Job;

/* ---- Function Prototypes ---- */
void add_job(pid_t pid, const char *name);
void remove_job(pid_t pid);
void print_jobs(void);
void bring_job_foreground(int job_id);
void continue_job_background(int job_id);

#endif /* JOBS_H */
