/*
 * mem_hierarchy.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 9 evidence: the memory hierarchy, measured.
   A random pointer-chase through a working set of growing size. Every load
   depends on the previous one, so the time per load is the access latency of
   whichever level (L1, L2, last-level cache, DRAM) holds the working set.  */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static double now_ns(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e9 + t.tv_nsec;
}

int main(void)
{
    printf("%12s %14s\n", "working set", "ns per load");
    for (size_t bytes = 4 << 10; bytes <= (size_t)256 << 20; bytes *= 4) {
        size_t n = bytes / sizeof(size_t);
        size_t *next = malloc(bytes);
        size_t *perm = malloc(bytes);
        for (size_t i = 0; i < n; i++) perm[i] = i;
        srand(42);
        for (size_t i = n - 1; i > 0; i--) {             /* random cycle defeats prefetching */
            size_t j = ((size_t)rand() * 65536u + (size_t)rand()) % (i + 1);
            size_t t = perm[i]; perm[i] = perm[j]; perm[j] = t;
        }
        for (size_t i = 0; i < n; i++) next[perm[i]] = perm[(i + 1) % n];
        size_t p = perm[0];
        long loads = 20000000;
        for (long i = 0; i < n && i < 1000000; i++) p = next[p];   /* warm up */
        double a = now_ns();
        for (long i = 0; i < loads; i++) p = next[p];
        double ns = (now_ns() - a) / loads;
        if (bytes < (1 << 20)) printf("%9zu KB %14.2f\n", bytes >> 10, ns);
        else                   printf("%9zu MB %14.2f\n", bytes >> 20, ns);
        if (p == (size_t)-1) puts("");                   /* keep p alive */
        free(next); free(perm);
    }
    return 0;
}
