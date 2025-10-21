#include "shell.h"
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

Job jobs[MAX_JOBS];
int job_count = 0;

/* Add a background job */
void add_job(pid_t pid, const char* cmdline) {
    if (job_count >= MAX_JOBS) return;
    jobs[job_count].pid = pid;
    strncpy(jobs[job_count].cmdline, cmdline, MAX_LEN-1);
    jobs[job_count].cmdline[MAX_LEN-1] = '\0';
    jobs[job_count].job_id = job_count + 1;
    jobs[job_count].running = 1;
    job_count++;
}

/* Remove job by pid */
void remove_job(pid_t pid) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].pid == pid) {
            for (int j = i; j < job_count - 1; ++j) jobs[j] = jobs[j+1];
            job_count--;
            break;
        }
    }
}

/* Update job running/stopped status (called by SIGCHLD or list_jobs) */
static void update_jobs_states(void) {
    for (int i = 0; i < job_count; ++i) {
        int status;
        pid_t r = waitpid(jobs[i].pid, &status, WNOHANG);
        if (r == 0) continue; /* still running */
        if (r == jobs[i].pid) {
            /* process finished */
            printf("\n[BG] process %d finished: %s\n", jobs[i].pid, jobs[i].cmdline);
            remove_job(jobs[i].pid);
            i = -1; /* restart scanning since job_count changed */
            continue;
        }
    }
}

/* List jobs (refresh states first) */
void list_jobs(void) {
    update_jobs_states();
    if (job_count == 0) {
        printf("No background jobs.\n");
        return;
    }
    for (int i = 0; i < job_count; ++i) {
        printf("[%d] PID: %d  %s (%s)\n",
               jobs[i].job_id, jobs[i].pid, jobs[i].cmdline,
               jobs[i].running ? "Running" : "Stopped");
    }
}

/* Bring a job to foreground by job_id */
int bring_fg(int job_id) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].job_id == job_id) {
            pid_t pid = jobs[i].pid;
            int status;
            /* send SIGCONT to the process group */
            kill(-pid, SIGCONT);
            /* Wait in foreground */
            if (waitpid(pid, &status, WUNTRACED) == -1) perror("waitpid");
            remove_job(pid);
            return 0;
        }
    }
    printf("No such job ID: %d\n", job_id);
    return -1;
}

/* Resume a stopped job in background */
int resume_bg(int job_id) {
    for (int i = 0; i < job_count; ++i) {
        if (jobs[i].job_id == job_id) {
            pid_t pid = jobs[i].pid;
            kill(-pid, SIGCONT);
            jobs[i].running = 1;
            return 0;
        }
    }
    printf("No such job ID: %d\n", job_id);
    return -1;
}

/* SIGCHLD handler to reap finished background children */
void handle_sigchld(int sig) {
    (void)sig;
    int saved_errno = errno;
    pid_t pid;
    int status;
    /* Reap all finished children */
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        /* Notify and remove job if tracked */
        for (int i = 0; i < job_count; ++i) {
            if (jobs[i].pid == pid) {
                printf("\n[BG] process %d finished: %s\n", pid, jobs[i].cmdline);
                remove_job(pid);
                break;
            }
        }
    }
    errno = saved_errno;
}
