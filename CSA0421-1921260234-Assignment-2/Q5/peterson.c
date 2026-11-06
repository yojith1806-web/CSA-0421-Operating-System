/* Q5: Peterson's solution for two threads, and why it can fail on modern
   processors. Each thread increments a shared counter N times inside the
   critical section guarded by Peterson's algorithm.
     plain : flag[] and turn are ordinary (volatile) variables. The CPU (ARM
             and x86 store buffers) may let a thread read the other's flag
             BEFORE its own store is visible -> both enter -> lost updates.
     fence : the same algorithm with a sequentially-consistent memory fence
             after the stores -> mutual exclusion is restored.             */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>

#define N 2000000

static volatile int flag[2], turn;
static volatile long counter;
static int use_fence;

static void *run(void *arg)
{
    int i = (int)(long)arg, j = 1 - i;
    for (int k = 0; k < N; k++) {
        flag[i] = 1;                                          /* I want to enter */
        turn = j;                                             /* you go first    */
        if (use_fence) atomic_thread_fence(memory_order_seq_cst);
        while (flag[j] && turn == j) {}                       /* busy wait       */
        counter++;                                            /* critical section */
        if (use_fence) atomic_thread_fence(memory_order_seq_cst);
        flag[i] = 0;                                          /* exit section    */
    }
    return NULL;
}

static long trial(int fence)
{
    use_fence = fence; counter = 0; flag[0] = flag[1] = 0; turn = 0;
    pthread_t a, b;
    pthread_create(&a, NULL, run, (void *)0L);
    pthread_create(&b, NULL, run, (void *)1L);
    pthread_join(a, NULL); pthread_join(b, NULL);
    return counter;
}

int main(void)
{
    printf("expected counter = %d\n", 2 * N);
    for (int r = 1; r <= 3; r++) {
        long c = trial(0);
        printf("plain Peterson, run %d : %ld  (%ld lost -> mutual exclusion %s)\n", r, c, 2L * N - c, c == 2L * N ? "held" : "VIOLATED");
    }
    for (int r = 1; r <= 3; r++) {
        long c = trial(1);
        printf("with seq_cst fence, run %d: %ld  (%ld lost -> mutual exclusion %s)\n", r, c, 2L * N - c, c == 2L * N ? "held" : "VIOLATED");
    }
    return 0;
}
