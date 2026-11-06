/* Q1 design decision: serve 2,000 student sessions three ways.
   Each session = 200 us of CPU (grading an answer) + 2 ms waiting for I/O
   (saving to disk / network), as in the exam server.
     (a) one process per student   (fork, run session, exit)
     (b) one thread per student    (pthread_create)
     (c) fixed pool of W worker threads taking sessions from a shared queue
   Reports total time and peak live tasks.                                    */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/wait.h>

#define SESSIONS 2000
static atomic_int next_session;

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static void session_work(void)
{
    double t0 = now_ms();
    volatile unsigned long x = 0;
    while (now_ms() - t0 < 0.2) x++;              /* 200 us CPU */
    struct timespec io = {0, 2000000};            /* 2 ms I/O wait */
    nanosleep(&io, NULL);
}

static void *thread_session(void *arg) { (void)arg; session_work(); return NULL; }

static void *pool_worker(void *arg)
{
    (void)arg;
    while (atomic_fetch_add(&next_session, 1) < SESSIONS) session_work();
    return NULL;
}

int main(void)
{
    double t0 = now_ms();
    pid_t *pids = malloc(sizeof(pid_t) * SESSIONS);
    int made = 0;
    for (int i = 0; i < SESSIONS; i++) {
        pid_t p = fork();
        if (p == 0) { session_work(); _exit(0); }
        if (p < 0) { perror("fork"); break; }
        pids[made++] = p;
    }
    for (int i = 0; i < made; i++) waitpid(pids[i], NULL, 0);
    double t_proc = now_ms() - t0;

    t0 = now_ms();
    pthread_t *th = malloc(sizeof(pthread_t) * SESSIONS);
    for (int i = 0; i < SESSIONS; i++) pthread_create(&th[i], NULL, thread_session, NULL);
    for (int i = 0; i < SESSIONS; i++) pthread_join(th[i], NULL);
    double t_thr = now_ms() - t0;

    printf("2,000 sessions, each 0.2 ms CPU + 2 ms I/O wait\n");
    printf("%-34s %10s %12s\n", "design", "total(ms)", "live tasks");
    printf("%-34s %10.0f %12d\n", "one process per student", t_proc, made);
    printf("%-34s %10.0f %12d\n", "one thread per student", t_thr, SESSIONS);
    int pools[] = {4, 16, 64};
    for (int k = 0; k < 3; k++) {
        atomic_store(&next_session, 0);
        t0 = now_ms();
        for (int i = 0; i < pools[k]; i++) pthread_create(&th[i], NULL, pool_worker, NULL);
        for (int i = 0; i < pools[k]; i++) pthread_join(th[i], NULL);
        char label[64]; snprintf(label, sizeof label, "fixed pool of %d workers", pools[k]);
        printf("%-34s %10.0f %12d\n", label, now_ms() - t0, pools[k]);
    }
    free(pids); free(th);
    return 0;
}
