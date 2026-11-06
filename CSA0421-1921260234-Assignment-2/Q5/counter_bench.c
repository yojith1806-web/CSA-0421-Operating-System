/* Q5 alternative concept: protecting a shared counter with
     mutex, semaphore (as a binary lock), C11 atomic_fetch_add, spin lock
   Part 1: time per update with 1, 4 and 16 threads (short critical section).
   Part 2: a LONG critical section (~20 us) with 16 threads: compare wall time
           and the CPU time burned (getrusage), to see the cost of spinning.  */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/resource.h>

#define TOTAL 2000000

static long counter;
static atomic_long acounter;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static sem_t sem;
static atomic_flag spin = ATOMIC_FLAG_INIT;
static int method, per_thread, long_cs;

static double now_s(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static double cpu_s(void)
{
    struct rusage u; getrusage(RUSAGE_SELF, &u);
    return u.ru_utime.tv_sec + u.ru_utime.tv_usec / 1e6 + u.ru_stime.tv_sec + u.ru_stime.tv_usec / 1e6;
}

static void work_inside(void)
{
    if (!long_cs) { counter++; return; }
    double t0 = now_s();
    while (now_s() - t0 < 20e-6) {}                       /* 20 us of work while holding the lock */
    counter++;
}

static void *worker(void *arg)
{
    (void)arg;
    for (int i = 0; i < per_thread; i++) {
        switch (method) {
        case 0: pthread_mutex_lock(&mtx); work_inside(); pthread_mutex_unlock(&mtx); break;
        case 1: sem_wait(&sem); work_inside(); sem_post(&sem); break;
        case 2: atomic_fetch_add(&acounter, 1); break;
        case 3: while (atomic_flag_test_and_set_explicit(&spin, memory_order_acquire)) {}
                work_inside();
                atomic_flag_clear_explicit(&spin, memory_order_release); break;
        }
    }
    return NULL;
}

static void run(int threads, int total, double *wall, double *cpu, long *result)
{
    pthread_t th[64];
    counter = 0; atomic_store(&acounter, 0);
    per_thread = total / threads;
    double c0 = cpu_s(), t0 = now_s();
    for (int i = 0; i < threads; i++) pthread_create(&th[i], NULL, worker, NULL);
    for (int i = 0; i < threads; i++) pthread_join(th[i], NULL);
    *wall = now_s() - t0; *cpu = cpu_s() - c0;
    *result = method == 2 ? atomic_load(&acounter) : counter;
}

int main(void)
{
    sem_init(&sem, 0, 1);
    const char *names[] = {"mutex", "semaphore", "atomic_fetch_add", "spin lock"};
    int tcount[] = {1, 4, 16};
    printf("Part 1: %d updates in total, short critical section (counter++)\n", TOTAL);
    printf("%-18s %8s %14s %10s %10s\n", "method", "threads", "ns per update", "CPU/wall", "correct");
    for (method = 0; method < 4; method++)
        for (int k = 0; k < 3; k++) {
            double w, c; long r;
            run(tcount[k], TOTAL, &w, &c, &r);
            printf("%-18s %8d %14.1f %10.2f %10s\n", names[method], tcount[k], w * 1e9 / TOTAL, c / w,
                   r == (long)(TOTAL / tcount[k]) * tcount[k] ? "yes" : "NO");
        }
    long_cs = 1;
    int ops = 16 * 200;
    printf("\nPart 2: LONG critical section (20 us), 16 threads, %d updates\n", ops);
    printf("%-18s %12s %14s %18s\n", "method", "wall (ms)", "CPU time (ms)", "CPU wasted (ms)");
    int long_methods[] = {0, 3};
    for (int k = 0; k < 2; k++) {
        method = long_methods[k];
        double w, c; long r;
        run(16, ops, &w, &c, &r);
        double useful = ops * 20e-6;
        printf("%-18s %12.1f %14.1f %18.1f\n", names[method], w * 1e3, c * 1e3, (c - useful) * 1e3);
    }
    sem_destroy(&sem);
    return 0;
}
