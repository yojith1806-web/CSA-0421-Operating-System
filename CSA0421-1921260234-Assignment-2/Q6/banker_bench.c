/* Q6 D/E: how does the Banker's safety algorithm grow with n processes and
   m resource types?  Same algorithm as banker.c (scan all unfinished
   processes, repeat until none can finish), with sizes chosen at run time.
     worst case : processes can only finish in reverse order, so each pass
                  over the n processes finds exactly one -> n passes x n x m
     best case  : every process can finish in the first pass -> n x m
   Prediction: worst case O(m n^2): 10x more processes -> ~100x more time.   */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static int n, m, *avail, *maxm, *alloc;

static double now_us(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

static long long checks;
static int is_safe(int *seq)
{
    int *work = malloc(sizeof(int) * (size_t)m), *finish = calloc((size_t)n, sizeof(int)), count = 0;
    memcpy(work, avail, sizeof(int) * (size_t)m);
    while (count < n) {
        int found = 0;
        for (int p = 0; p < n; p++) {
            if (finish[p]) continue;
            int ok = 1;
            for (int r = 0; r < m; r++) {
                checks++;
                if (maxm[p * m + r] - alloc[p * m + r] > work[r]) { ok = 0; break; }
            }
            if (ok) {
                for (int r = 0; r < m; r++) work[r] += alloc[p * m + r];
                finish[p] = 1; seq[count++] = p; found = 1;
            }
        }
        if (!found) { free(work); free(finish); return 0; }
    }
    free(work); free(finish);
    return 1;
}

static void build(int worst)
{
    avail = malloc(sizeof(int) * (size_t)m);
    maxm = malloc(sizeof(int) * (size_t)n * m);
    alloc = malloc(sizeof(int) * (size_t)n * m);
    for (int r = 0; r < m; r++) avail[r] = 1;
    for (int p = 0; p < n; p++)
        for (int r = 0; r < m; r++) {
            alloc[p * m + r] = 1;
            /* worst: process p needs (n - p) more, which is only available after
               every process with a higher index has finished                     */
            maxm[p * m + r] = 1 + (worst ? n - p : 0);
        }
}

int main(void)
{
    int sizes[] = {10, 100, 1000, 2000};
    int ms[] = {3, 10};
    printf("%6s %4s %-6s %14s %16s %12s\n", "n", "m", "case", "time (us)", "need checks", "safe");
    for (int mi = 0; mi < 2; mi++) {
        m = ms[mi];
        for (int si = 0; si < 4; si++) {
            n = sizes[si];
            for (int worst = 1; worst >= 0; worst--) {
                build(worst);
                int *seq = malloc(sizeof(int) * (size_t)n);
                int reps = n <= 100 ? 1000 : (n <= 1000 ? 10 : 3), safe = 0;
                checks = 0;
                double t0 = now_us();
                for (int k = 0; k < reps; k++) safe = is_safe(seq);
                double t = (now_us() - t0) / reps;
                printf("%6d %4d %-6s %14.2f %16lld %12s\n", n, m, worst ? "worst" : "best", t, checks / reps, safe ? "yes" : "no");
                free(seq); free(avail); free(maxm); free(alloc);
            }
        }
    }
    return 0;
}
