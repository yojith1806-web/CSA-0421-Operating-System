/*
 * syscall_vs_ipc.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 3 evidence: cost of one service request in a monolithic kernel
   (a single system call, the service runs inside the kernel) versus a
   microkernel (the request is a message to a user-space server process and
   the reply is another message).

   (a) monolithic : getppid() system call                     -> 1 trap
   (b) microkernel: write request to server pipe, server reads it and writes a
                    reply, client reads the reply             -> 4 traps + 2 context switches
   Both are measured with the two processes on the same CPU (like a small
   single-core monitor) and on any CPU.                                      */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sched.h>
#include <time.h>
#include <sys/wait.h>

static double now_ns(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

static void pin_cpu0(void)
{
#ifdef __linux__
    cpu_set_t s; CPU_ZERO(&s); CPU_SET(0, &s);
    sched_setaffinity(0, sizeof s, &s);
#endif   /* macOS has no CPU pinning API: the "pinned" run is unpinned there */
}

static double ipc_round_trip(int iters, int pin)
{
    int req[2], rep[2];
    if (pipe(req) || pipe(rep)) { perror("pipe"); exit(1); }
    if (pin) pin_cpu0();
    pid_t pid = fork();
    if (pid == 0) {                         /* the "file-system server" */
        char m;
        close(req[1]); close(rep[0]);       /* keep only the ends the server uses */
        while (read(req[0], &m, 1) == 1) write(rep[1], &m, 1);
        _exit(0);
    }
    char m = 'x';
    double a = now_ns();
    for (int i = 0; i < iters; i++) { write(req[1], &m, 1); read(rep[0], &m, 1); }
    double b = now_ns();
    close(req[1]); waitpid(pid, NULL, 0);
    close(req[0]); close(rep[0]); close(rep[1]);
    return (b - a) / iters;
}

int main(void)
{
    const int N = 50000;
    volatile pid_t sink = 0;
    double a = now_ns();
    for (int i = 0; i < N * 5; i++) sink += getppid();
    double sys = (now_ns() - a) / (N * 5);
    (void)sink;

    double ipc_any = ipc_round_trip(N, 0);
    double ipc_pin = ipc_round_trip(N, 1);
    printf("monolithic: one system call (getppid)            : %8.1f ns\n", sys);
    printf("microkernel: message round trip, any CPU          : %8.1f ns  (%.0fx)\n", ipc_any, ipc_any / sys);
    printf("microkernel: message round trip, same CPU (pinned): %8.1f ns  (%.0fx)\n", ipc_pin, ipc_pin / sys);
    return 0;
}
