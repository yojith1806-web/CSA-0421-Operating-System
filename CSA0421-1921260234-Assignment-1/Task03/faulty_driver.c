/*
 * faulty_driver.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 3 evidence: what a faulty driver does to the system in each structure.
   Monolithic: the driver runs in the same address space as everything else,
               so a wild pointer in it takes the whole "kernel" (here: the one
               process holding alarm + display + driver) down.
   Microkernel: the driver is a separate user-space server; when it crashes
               the alarm service keeps running and a supervisor restarts it.  */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>

static void buggy_display_driver(int tick)
{
    if (tick == 3) { volatile int *p = NULL; *p = 42; }      /* the bug */
}

static void alarm_service(int tick) { printf("  alarm service: tick %d, vitals checked\n", tick); fflush(stdout); }

static void monolithic(void)
{
    pid_t k = fork();
    if (k == 0) {                                    /* the whole kernel = one address space */
        for (int t = 1; t <= 6; t++) { alarm_service(t); buggy_display_driver(t); usleep(1000); }
        _exit(0);
    }
    int st; waitpid(k, &st, 0);
    if (WIFSIGNALED(st))
        printf("  KERNEL PANIC: signal %d in driver - alarm service died with it\n", WTERMSIG(st));
}

static pid_t start_driver(void)
{
    pid_t d = fork();
    if (d == 0) {
        for (int t = 1; ; t++) { buggy_display_driver(t); usleep(1500); }
    }
    return d;
}

static void microkernel(void)
{
    pid_t drv = start_driver();
    int restarts = 0;
    for (int t = 1; t <= 6; t++) {                    /* alarm server keeps its own address space */
        alarm_service(t);
        int st;
        if (waitpid(drv, &st, WNOHANG) == drv && WIFSIGNALED(st)) {
            printf("  supervisor: display driver crashed (signal %d) - restarting it, alarms unaffected\n", WTERMSIG(st));
            drv = start_driver(); restarts++;
        }
        usleep(2500);
    }
    kill(drv, SIGKILL); waitpid(drv, NULL, 0);
    printf("  finished all 6 ticks, driver restarted %d time(s)\n", restarts);
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);            /* no duplicated buffers across fork() */
    printf("== monolithic structure ==\n"); monolithic();
    printf("== microkernel structure ==\n"); microkernel();
    return 0;
}
