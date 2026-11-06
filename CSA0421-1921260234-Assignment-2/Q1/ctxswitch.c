/* Q1 alternative concept: context-switch cost between two PROCESSES versus two
   THREADS of one process. Two parties pass a token back and forth through two
   pipes; each round trip forces two switches, so switch = round trip / 2.
   (Includes pipe read/write cost, so it is an upper bound on the bare switch.) */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

#define ROUNDS 100000
static int a[2], b[2];

static double now_ns(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

static void *echo(void *arg)
{
    (void)arg; char c;
    for (int i = 0; i < ROUNDS; i++) { read(a[0], &c, 1); write(b[1], &c, 1); }
    return NULL;
}

static double ping(void)
{
    char c = 'x';
    double t0 = now_ns();
    for (int i = 0; i < ROUNDS; i++) { write(a[1], &c, 1); read(b[0], &c, 1); }
    return (now_ns() - t0) / ROUNDS / 2;
}

int main(void)
{
    pipe(a); pipe(b);
    pid_t pid = fork();
    if (pid == 0) { echo(NULL); _exit(0); }
    double proc = ping();
    waitpid(pid, NULL, 0);

    pthread_t t;
    pthread_create(&t, NULL, echo, NULL);
    double thr = ping();
    pthread_join(t, NULL);

    printf("switch between processes : %7.0f ns\n", proc);
    printf("switch between threads   : %7.0f ns\n", thr);
    printf("2,000 sessions switched once each: processes %.2f ms, threads %.2f ms\n",
           proc * 2000 / 1e6, thr * 2000 / 1e6);
    return 0;
}
