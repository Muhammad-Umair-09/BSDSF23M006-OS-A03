#include "shell.h"
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>

/* Job table */
job_t jobs[MAX_JOBS];
int job_count = 0;

/* Helper: check if job exists by pid, return index or -1 */
int job_index_by_pid(pid_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].pid == pid) return i;
    }
    return -1;
}

/* Exported helper used by other modules */
int job_exists(pid_t pid) {
    return job_index_by_pid(pid) != -1;
}

/* Add a job to the table */
void add_job(pid_t pid, const char* cmdline, int background) {
    if (job_count >= MAX_JOBS) {
        fprintf(stderr, "jobs: job table full, cannot add job\n");
        return;
    }

    jobs[job_count].pid = pid;
    jobs[job_count].job_id = job_count + 1;
    if (cmdline)
        strncpy(jobs[job_count].cmdline, cmdline, MAX_LEN - 1);
    else
        jobs[job_count].cmdline[0] = '\0';
    jobs[job_count].cmdline[MAX_LEN - 1] = '\0';

    strncpy(jobs[job_count].status, "Running", sizeof(jobs[job_count].status) - 1);
    jobs[job_count].status[sizeof(jobs[job_count].status) - 1] = '\0';

    jobs[job_count].background = background ? 1 : 0;

    job_count++;
}

/* Remove job by pid (shift table) */
void remove_job(pid_t pid) {
    int idx = job_index_by_pid(pid);
    if (idx == -1) return;
    for (int i = idx; i < job_count - 1; i++) {
        jobs[i] = jobs[i + 1];
    }
    job_count--;
    /* Reassign job_id to be contiguous */
    for (int i = 0; i < job_count; ++i) jobs[i].job_id = i + 1;
}

/* Update a job's status by pid */
void update_job_status(pid_t pid, const char* status) {
    if (!status) return;  // prevent invalid pointer crash
    int idx = job_index_by_pid(pid);
    if (idx == -1) return;
    strncpy(jobs[idx].status, status, sizeof(jobs[idx].status) - 1);
    jobs[idx].status[sizeof(jobs[idx].status) - 1] = '\0';
}

/* Print all jobs */
void print_jobs(void) {
    if (job_count == 0) {
        printf("No background jobs.\n");
        return;
    }
    for (int i = 0; i < job_count; ++i) {
        printf("[%d] %d %s %s\n", jobs[i].job_id, (int)jobs[i].pid,
               jobs[i].status, jobs[i].cmdline);
    }
}

/* Bring a job to foreground */
int bring_job_to_foreground(int job_id) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].job_id == job_id) {
            pid_t pid = jobs[i].pid;

            /* Give terminal to job's process group */
            if (tcsetpgrp(STDIN_FILENO, pid) < 0) {
                // perror("tcsetpgrp (fg)");
            }

            /* Continue job if it was stopped */
            if (kill(-pid, SIGCONT) < 0 && errno != ESRCH)
                perror("kill(SIGCONT)");

            int status;
            if (waitpid(pid, &status, WUNTRACED) == -1)
                perror("waitpid");

            /* Restore terminal to shell */
            if (tcsetpgrp(STDIN_FILENO, getpgrp()) < 0) {
                // perror("tcsetpgrp (restore)");
            }

            if (WIFSTOPPED(status)) {
                strncpy(jobs[i].status, "Stopped", sizeof(jobs[i].status) - 1);
                jobs[i].status[sizeof(jobs[i].status) - 1] = '\0';
            } else {
                /* finished - remove job */
                remove_job(pid);
            }

            return 0;
        }
    }
    fprintf(stderr, "fg: job %d not found\n", job_id);
    return -1;
}

/* Continue a stopped job in background */
int continue_job_in_background(int job_id) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].job_id == job_id) {
            pid_t pid = jobs[i].pid;

            if (kill(-pid, SIGCONT) < 0 && errno != ESRCH)
                perror("kill(SIGCONT)");

            strncpy(jobs[i].status, "Running", sizeof(jobs[i].status) - 1);
            jobs[i].status[sizeof(jobs[i].status) - 1] = '\0';

            printf("[%d] %d resumed in background\n", jobs[i].job_id, (int)pid);
            return 0;
        }
    }
    fprintf(stderr, "bg: job %d not found\n", job_id);
    return -1;
}

/* SIGCHLD handler: reap children and update job statuses */
void handle_sigchld(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;

    /* Reap all children that have changed state */
    while ((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0) {
        if (WIFEXITED(status) || WIFSIGNALED(status)) {
            /* process finished */
            update_job_status(pid, "Done");
            /* remove completed jobs from job table */
            remove_job(pid);
        } else if (WIFSTOPPED(status)) {
            update_job_status(pid, "Stopped");
            /* ensure it's present in job table; if not, add it */
            if (!job_exists(pid)) {
                add_job(pid, "stopped_job", 0);
                update_job_status(pid, "Stopped");
            }
        } else if (WIFCONTINUED(status)) {
            update_job_status(pid, "Running");
        }
    }

    errno = saved_errno;
}

/* Shell ignores SIGTSTP itself; placeholder if needed */
void handle_sigtstp(int sig) {
    (void)sig;
    /* intentionally empty: shell ignores SIGTSTP */
}
