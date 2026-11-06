/* Q1: an "exam server" parent forks three children in different states and
   shows them with ps (and /proc on Linux):
     session  - blocked reading the student's next answer   -> WAITING (S)
     autosave - busy saving answers (CPU work)               -> RUNNING/READY (R)
     proctor  - finished but not yet waited for              -> TERMINATED, zombie (Z)
   Then the parent reaps everything.                                          */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    int answers[2];
    pipe(answers);
    printf("exam server (parent) pid %d\n", getpid());

    pid_t session = fork();
    if (session == 0) {                         /* waits for input -> blocked */
        char c; read(answers[0], &c, 1); _exit(0);
    }
    pid_t autosave = fork();
    if (autosave == 0) {                        /* CPU-bound -> running / ready */
        volatile unsigned long x = 0; for (;;) x++;
    }
    pid_t proctor = fork();
    if (proctor == 0) _exit(0);                 /* ends at once -> zombie until reaped */

    sleep(1);
    char cmd[256];
    snprintf(cmd, sizeof cmd, "ps -o pid,ppid,stat,rss,vsz,time,comm -p %d,%d,%d,%d",
             getpid(), session, autosave, proctor);
    printf("\n$ %s\n", cmd);
    system(cmd);

    /* Linux only: the kernel's view of each PCB */
    if (access("/proc/self/status", R_OK) == 0) {
        pid_t kids[3] = {session, autosave, proctor};
        for (int i = 0; i < 3; i++) {
            snprintf(cmd, sizeof cmd,
                     "grep -E '^(Name|State|Pid|PPid|VmRSS|Threads|voluntary_ctxt_switches|nonvoluntary_ctxt_switches)' /proc/%d/status | tr '\\n' ' '; echo",
                     kids[i]);
            system(cmd);
        }
    }

    write(answers[1], "A", 1);                  /* answer arrives: session wakes and exits */
    kill(autosave, SIGTERM);
    int st;
    pid_t kids[3] = {session, autosave, proctor};
    const char *names[3] = {"session", "autosave", "proctor"};
    for (int i = 0; i < 3; i++) {
        waitpid(kids[i], &st, 0);
        if (WIFEXITED(st)) printf("reaped %-8s pid %d: exited, status %d\n", names[i], kids[i], WEXITSTATUS(st));
        else printf("reaped %-8s pid %d: killed by signal %d\n", names[i], kids[i], WTERMSIG(st));
    }
    return 0;
}
