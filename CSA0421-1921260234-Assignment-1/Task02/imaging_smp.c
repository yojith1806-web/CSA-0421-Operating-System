/*
 * imaging_smp.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 2 evidence: how well does an MRI-style image workload scale on a
   symmetric multiprocessor (SMP)?  A 2048x2048 "slice" is smoothed with a 3x3
   filter several times; the rows are split evenly between T threads, and any
   thread may run on any core (SMP).  Speed-up = time(1 thread) / time(T).

   usage: ./imaging_smp [max_threads]                                        */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>

#define N 2048
#define PASSES 6

static float *src, *dst;
typedef struct { int r0, r1; } Range;

static void *worker(void *arg)
{
    Range *r = arg;
    for (int y = r->r0; y < r->r1; y++) {
        if (y == 0 || y == N - 1) continue;
        for (int x = 1; x < N - 1; x++) {
            float s = 0;
            for (int dy = -1; dy <= 1; dy++)
                for (int dx = -1; dx <= 1; dx++)
                    s += src[(y + dy) * N + x + dx];
            dst[y * N + x] = s / 9.0f;
        }
    }
    return NULL;
}

static double run(int T)
{
    pthread_t th[64]; Range rg[64];
    struct timespec a, b;
    clock_gettime(CLOCK_MONOTONIC, &a);
    for (int p = 0; p < PASSES; p++) {
        for (int t = 0; t < T; t++) {
            rg[t].r0 = N * t / T; rg[t].r1 = N * (t + 1) / T;
            pthread_create(&th[t], NULL, worker, &rg[t]);
        }
        for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
        float *tmp = src; src = dst; dst = tmp;          /* next pass reads result */
    }
    clock_gettime(CLOCK_MONOTONIC, &b);
    return (b.tv_sec - a.tv_sec) * 1e3 + (b.tv_nsec - a.tv_nsec) / 1e6;
}

int main(int argc, char **argv)
{
    long cores = sysconf(_SC_NPROCESSORS_ONLN);
    int maxT = argc > 1 ? atoi(argv[1]) : (int)cores * 2;
    if (maxT > 64) maxT = 64;
    src = malloc(sizeof(float) * N * N);
    dst = malloc(sizeof(float) * N * N);
    for (int i = 0; i < N * N; i++) src[i] = (float)(i % 251);
    printf("online cores: %ld, image %dx%d floats, %d passes\n", cores, N, N, PASSES);
    printf("%8s %12s %9s %11s\n", "threads", "time(ms)", "speedup", "efficiency");
    double t1 = 0;
    for (int T = 1; T <= maxT; T *= 2) {
        double best = 1e18;
        for (int rep = 0; rep < 3; rep++) { double t = run(T); if (t < best) best = t; }
        if (T == 1) t1 = best;
        printf("%8d %12.1f %9.2f %10.0f%%\n", T, best, t1 / best, 100.0 * t1 / best / T);
    }
    free(src); free(dst);
    return 0;
}
