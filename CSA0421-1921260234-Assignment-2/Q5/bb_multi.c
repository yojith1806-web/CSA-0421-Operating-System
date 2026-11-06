/* Q5 test of the design: bounded buffer with P producers and C consumers.
   Every item has a unique id; consumers record how many times each id was
   consumed, so we can show "every item consumed exactly once".
   usage: ./bb_multi PRODUCERS CONSUMERS [nomutex]
   With "nomutex" the mutex around in/out is removed (semaphores kept).      */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#define SIZE 5
#define PER_PRODUCER 20000

static int buffer[SIZE], in = 0, out = 0, use_mutex = 1, P, C;
static sem_t empty, full;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_int *seen;

static void *producer(void *arg)
{
    int base = (int)(long)arg * PER_PRODUCER;
    for (int k = 0; k < PER_PRODUCER; k++) {
        sem_wait(&empty);
        if (use_mutex) pthread_mutex_lock(&lock);
        buffer[in] = base + k + 1;                  /* ids 1 .. P*PER_PRODUCER */
        in = (in + 1) % SIZE;
        if (use_mutex) pthread_mutex_unlock(&lock);
        sem_post(&full);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    int n = (int)(long)arg;                          /* how many items this consumer takes */
    for (int k = 0; k < n; k++) {
        sem_wait(&full);
        if (use_mutex) pthread_mutex_lock(&lock);
        int item = buffer[out];
        out = (out + 1) % SIZE;
        if (use_mutex) pthread_mutex_unlock(&lock);
        sem_post(&empty);
        if (item >= 1 && item <= P * PER_PRODUCER) atomic_fetch_add(&seen[item], 1);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    P = argc > 1 ? atoi(argv[1]) : 1;
    C = argc > 2 ? atoi(argv[2]) : 1;
    use_mutex = !(argc > 3 && strcmp(argv[3], "nomutex") == 0);
    int total = P * PER_PRODUCER;
    seen = calloc((size_t)total + 1, sizeof *seen);
    sem_init(&empty, 0, SIZE);
    sem_init(&full, 0, 0);
    pthread_t pt[64], ct[64];
    for (int i = 0; i < P; i++) pthread_create(&pt[i], NULL, producer, (void *)(long)i);
    for (int i = 0; i < C; i++) pthread_create(&ct[i], NULL, consumer, (void *)(long)(total / C + (i < total % C)));
    for (int i = 0; i < P; i++) pthread_join(pt[i], NULL);
    for (int i = 0; i < C; i++) pthread_join(ct[i], NULL);
    int once = 0, missing = 0, dup = 0;
    for (int i = 1; i <= total; i++) {
        int s = atomic_load(&seen[i]);
        if (s == 1) once++; else if (s == 0) missing++; else dup += s - 1;
    }
    printf("%d producer(s), %d consumer(s), mutex %-3s: %d items, consumed exactly once %d, missing %d, duplicates %d -> %s\n",
           P, C, use_mutex ? "ON" : "OFF", total, once, missing, dup, (once == total) ? "CORRECT" : "WRONG");
    return 0;
}
