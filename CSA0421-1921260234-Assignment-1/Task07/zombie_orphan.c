/*
 * zombie_orphan.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 7 evidence: a zombie and an orphan, observed with ps.
   1) Zombie: the child exits but the parent has not called wait() yet, so the
      child stays in the process table in state Z (<defunct>).
   2) Orphan: the parent exits first; the child is re-parented to init/systemd
      (or the nearest "subreaper"), which will reap it.                     */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    char cmd[128];
    /* ---- zombie ---- */
    pid_t z = fork();
    if (z == 0) _exit(0);                       /* child ends immediately */
    sleep(1);                                   /* parent "forgets" to wait */
    printf("== zombie: child %d has exited, parent %d has not waited ==\n", z, getpid());
    fflush(stdout);
    snprintf(cmd, sizeof cmd, "ps -o pid,ppid,stat,comm -p %d", z);
    system(cmd);
    waitpid(z, NULL, 0);                        /* reaping removes the zombie */
    printf("after waitpid():\n"); fflush(stdout);
    system(cmd);

    /* ---- orphan ---- */
    printf("\n== orphan ==\n"); fflush(stdout);
    pid_t mid = fork();
    if (mid == 0) {                             /* middle process */
        pid_t o = fork();
        if (o == 0) {                           /* grandchild = future orphan */
            printf("orphan-to-be %d: my parent is %d\n", getpid(), getppid()); fflush(stdout);
            sleep(2);                           /* parent dies meanwhile */
            printf("orphan %d: my parent is now %d (adopted, will be reaped by it)\n", getpid(), getppid());
            fflush(stdout);
            _exit(0);
        }
        sleep(1);
        printf("parent %d exits without waiting for %d\n", getpid(), o); fflush(stdout);
        _exit(0);
    }
    waitpid(mid, NULL, 0);
    sleep(3);                                   /* let the orphan print */
    return 0;
}
