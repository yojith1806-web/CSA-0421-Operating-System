/*
 * create_bench.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 10 evidence: time and memory to create 1,000 processes versus 1,000
   threads. All 1,000 stay alive (blocked on a pipe) until the last one has
   been created, so the memory they hold at the same time can be measured
   from MemAvailable in /proc/meminfo.
   usage: ./create_bench proc|thread [count]                                */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

static int gate[2];

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static long mem_available_kb(void)
{
    FILE *f = fopen("/proc/meminfo", "r"); char line[128]; long v = -1;
    if (!f) return -1;                                  /* not Linux: no /proc */
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "MemAvailable: %ld kB", &v) == 1) break;
    fclose(f); return v;
}

static void *thread_body(void *arg) { char c; (void)arg; read(gate[0], &c, 1); return NULL; }

int main(int argc, char **argv)
{
    int threads = argc > 1 && strcmp(argv[1], "thread") == 0;
    int n = argc > 2 ? atoi(argv[2]) : 1000;
    pipe(gate);
    long m0 = mem_available_kb();
    double a = now_ms();
    pid_t *pids = malloc(sizeof(pid_t) * (size_t)n);
    pthread_t *tids = malloc(sizeof(pthread_t) * (size_t)n);
    for (int i = 0; i < n; i++) {
        if (threads) {
            if (pthread_create(&tids[i], NULL, thread_body, NULL) != 0) { perror("pthread_create"); return 1; }
        } else {
            pids[i] = fork();
            if (pids[i] < 0) { perror("fork"); return 1; }
            if (pids[i] == 0) { char c; read(gate[0], &c, 1); _exit(0); }
        }
    }
    double t = now_ms() - a;
    usleep(200000);                                     /* let memory settle */
    long m1 = mem_available_kb();
    for (int i = 0; i < n; i++) write(gate[1], "x", 1);  /* release everyone */
    for (int i = 0; i < n; i++) {
        if (threads) pthread_join(tids[i], NULL); else waitpid(pids[i], NULL, 0);
    }
    if (m0 < 0)
        printf("%-9s created=%d  time=%.1f ms (%.1f us each)  memory=n/a (no /proc on this OS)\n",
               threads ? "threads" : "processes", n, t, t * 1e3 / n);
    else
        printf("%-9s created=%d  time=%.1f ms (%.1f us each)  memory=%.1f MB (%.1f KB each)\n",
               threads ? "threads" : "processes", n, t, t * 1e3 / n,
               (m0 - m1) / 1024.0, (double)(m0 - m1) / n);
    return 0;
}
