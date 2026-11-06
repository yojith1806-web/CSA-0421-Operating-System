/*
 * syscall_cost.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 6 evidence: why a system call costs much more than a function call.
   Times 10 million calls of an ordinary (non-inlined) function and of the
   getppid() system call, which does almost no work inside the kernel, so the
   difference is the trap + mode switch + return.                          */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <time.h>

__attribute__((noinline)) static int plain_function(int x) { __asm__ volatile(""); return x + 1; }

static double now_ns(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

int main(void)
{
    const long N = 10000000;
    volatile int acc = 0;
    double a = now_ns();
    for (long i = 0; i < N; i++) acc = plain_function(acc);
    double f = (now_ns() - a) / N;

    a = now_ns();
    for (long i = 0; i < N; i++) acc += getppid();
    double s = (now_ns() - a) / N;

    printf("ordinary function call : %7.2f ns\n", f);
    printf("getppid() system call  : %7.2f ns\n", s);
    printf("system call / function : %7.0fx more expensive\n", s / f);
    return 0;
}
