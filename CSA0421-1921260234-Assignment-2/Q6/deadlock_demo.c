/* Q6 A/B: a real deadlock on the lab cluster, then two fixes.
   Job A (simulation) takes the GPU, then wants the software LICENCE.
   Job B (render)     takes the LICENCE, then wants the GPU.
   1) as written     -> circular wait -> deadlock. We detect it with a
                        watchdog (no progress for 2 s) and RECOVER by aborting
                        job B (it releases the licence and its work is lost).
   2) PREVENTION     -> both jobs request resources in one global order
                        (GPU before LICENCE), so circular wait cannot occur. */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

static pthread_mutex_t gpu = PTHREAD_MUTEX_INITIALIZER, licence = PTHREAD_MUTEX_INITIALIZER;
static atomic_int abort_b, a_done, b_done;
static int ordered;

static double now_s(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void *job_a(void *arg)
{
    (void)arg;
    pthread_mutex_lock(&gpu);           printf("  job A: holds GPU, requests LICENCE\n");
    usleep(100000);
    pthread_mutex_lock(&licence);       printf("  job A: holds GPU + LICENCE -> running simulation\n");
    usleep(50000);
    pthread_mutex_unlock(&licence); pthread_mutex_unlock(&gpu);
    atomic_store(&a_done, 1);           printf("  job A: finished\n");
    return NULL;
}

static void *job_b(void *arg)
{
    (void)arg;
    pthread_mutex_t *first = ordered ? &gpu : &licence, *second = ordered ? &licence : &gpu;
    pthread_mutex_lock(first);          printf("  job B: holds %s, requests %s\n", ordered ? "GPU" : "LICENCE", ordered ? "LICENCE" : "GPU");
    usleep(100000);
    while (pthread_mutex_trylock(second) != 0) {        /* blocked waiting ...       */
        if (atomic_load(&abort_b)) {                    /* ... unless recovery aborts */
            pthread_mutex_unlock(first);
            printf("  job B: ABORTED by recovery, released %s (its 0.1 s of work is lost)\n", ordered ? "GPU" : "LICENCE");
            return NULL;
        }
        usleep(1000);
    }
    printf("  job B: holds GPU + LICENCE -> rendering\n");
    usleep(50000);
    pthread_mutex_unlock(second); pthread_mutex_unlock(first);
    atomic_store(&b_done, 1);           printf("  job B: finished\n");
    return NULL;
}

static void scenario(int ord)
{
    ordered = ord; atomic_store(&abort_b, 0); atomic_store(&a_done, 0); atomic_store(&b_done, 0);
    pthread_t a, b;
    double t0 = now_s();
    pthread_create(&a, NULL, job_a, NULL);
    pthread_create(&b, NULL, job_b, NULL);
    while (!(atomic_load(&a_done) && atomic_load(&b_done))) {      /* watchdog / detector */
        if (now_s() - t0 > 2.0 && !atomic_load(&abort_b)) {
            printf("  DETECTOR: no progress for 2 s - A waits for LICENCE held by B,\n"
                   "            B waits for GPU held by A -> cycle -> DEADLOCK. Aborting job B.\n");
            atomic_store(&abort_b, 1);
        }
        if (atomic_load(&abort_b) && atomic_load(&a_done)) break;
        usleep(10000);
    }
    pthread_join(a, NULL); pthread_join(b, NULL);
    printf("  total time %.2f s\n", now_s() - t0);
}

int main(void)
{
    setvbuf(stdout, NULL, _IOLBF, 0);
    printf("== 1) jobs lock in opposite orders (all four conditions hold) ==\n");
    scenario(0);
    printf("\n== 2) prevention: every job locks GPU before LICENCE (no circular wait) ==\n");
    scenario(1);
    return 0;
}
