/*
 * alarm_latency.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 1 evidence: how late does a periodic "bedside monitor" task wake up on a
   general-purpose OS?  The task asks to be woken every 1 ms (as a monitor that
   samples a sensor would) and records how late each wake-up actually was.
   On a hard real-time OS the worst case is bounded; on a general-purpose OS
   it is not guaranteed and grows with load.

   usage: ./alarm_latency [iterations] [rt]
          rt -> run as SCHED_FIFO priority 80 (needs root)            */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sched.h>

static int cmp(const void *a, const void *b)
{
    long x = *(const long *)a, y = *(const long *)b;
    return (x > y) - (x < y);
}

int main(int argc, char **argv)
{
    int iters = argc > 1 ? atoi(argv[1]) : 10000;
    int rt = argc > 2 && strcmp(argv[2], "rt") == 0;
    if (rt) {
#ifdef __linux__
        struct sched_param p = { .sched_priority = 80 };
        if (sched_setscheduler(0, SCHED_FIFO, &p) != 0) { perror("sched_setscheduler"); return 1; }
#else
        fprintf(stderr, "SCHED_FIFO is only available on Linux\n"); return 1;
#endif
    }
    long *lat = malloc(sizeof(long) * (size_t)iters);
    struct timespec next, now;
    clock_gettime(CLOCK_MONOTONIC, &next);
    for (int i = 0; i < iters; i++) {
        next.tv_nsec += 1000000;                         /* +1 ms */
        if (next.tv_nsec >= 1000000000) { next.tv_nsec -= 1000000000; next.tv_sec++; }
#ifdef __linux__
        clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, &next, NULL);
#else   /* macOS has no clock_nanosleep: sleep for the remaining relative time */
        clock_gettime(CLOCK_MONOTONIC, &now);
        long rem = (next.tv_sec - now.tv_sec) * 1000000000L + (next.tv_nsec - now.tv_nsec);
        if (rem > 0) { struct timespec r = { rem / 1000000000L, rem % 1000000000L }; nanosleep(&r, NULL); }
#endif
        clock_gettime(CLOCK_MONOTONIC, &now);
        lat[i] = (now.tv_sec - next.tv_sec) * 1000000000L + (now.tv_nsec - next.tv_nsec);
    }
    qsort(lat, (size_t)iters, sizeof(long), cmp);
    double sum = 0; long over1 = 0, over10 = 0;
    for (int i = 0; i < iters; i++) {
        sum += lat[i];
        if (lat[i] > 1000000) over1++;
        if (lat[i] > 10000000) over10++;
    }
    printf("policy=%s wakeups=%d  min=%.1fus avg=%.1fus p99=%.1fus p99.9=%.1fus max=%.1fus  >1ms=%ld >10ms=%ld\n",
           rt ? "SCHED_FIFO" : "SCHED_OTHER", iters, lat[0] / 1e3, sum / iters / 1e3,
           lat[iters * 99 / 100] / 1e3, lat[iters * 999 / 1000] / 1e3, lat[iters - 1] / 1e3,
           over1, over10);
    free(lat);
    return 0;
}
