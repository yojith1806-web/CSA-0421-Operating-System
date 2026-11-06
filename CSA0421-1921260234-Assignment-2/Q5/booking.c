/* Q5 deployment decision: railway seat booking with three lock granularities.
     global : one mutex for the whole booking system
     train  : one mutex per train
     coach  : one mutex per coach
     none   : no lock (shows double-selling)
   8 agents (threads) book random seats on 10 trains x 20 coaches x 72 seats.
   Booking = find a free seat in the chosen coach, check the fare/quota (a
   short piece of work inside the critical section), mark it sold.
   Throughput = bookings per second; correctness = no seat sold twice.
   usage: ./booking [agents]                                                 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TRAINS 10
#define COACHES 20
#define SEATS 72
#define ATTEMPTS 40000                       /* per agent */

static int sold[TRAINS][COACHES][SEATS];     /* how many times each seat was sold */
static pthread_mutex_t g_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t t_lock[TRAINS];
static pthread_mutex_t c_lock[TRAINS][COACHES];
static int mode;                             /* 0 none, 1 global, 2 train, 3 coach */
static long booked_total;
static pthread_mutex_t count_lock = PTHREAD_MUTEX_INITIALIZER;

static double now_s(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static pthread_mutex_t *lock_for(int t, int c)
{
    switch (mode) {
    case 1: return &g_lock;
    case 2: return &t_lock[t];
    case 3: return &c_lock[t][c];
    default: return NULL;
    }
}

static void *agent(void *arg)
{
    unsigned seed = (unsigned)(long)arg * 2654435761u + 1;
    long mine = 0;
    for (int k = 0; k < ATTEMPTS; k++) {
        seed = seed * 1103515245u + 12345u; int t = (seed >> 8) % TRAINS;
        seed = seed * 1103515245u + 12345u; int c = (seed >> 8) % COACHES;
        pthread_mutex_t *l = lock_for(t, c);
        if (l) pthread_mutex_lock(l);
        int s = 0;
        while (s < SEATS && sold[t][c][s]) s++;               /* find a free seat */
        if (s < SEATS) {
            volatile int fare = 0;
            for (int w = 0; w < 300; w++) fare += w;          /* fare / quota check */
            sold[t][c][s]++;                                  /* sell it */
            mine++;
        }
        if (l) pthread_mutex_unlock(l);
    }
    pthread_mutex_lock(&count_lock); booked_total += mine; pthread_mutex_unlock(&count_lock);
    return NULL;
}

int main(int argc, char **argv)
{
    int agents = argc > 1 ? atoi(argv[1]) : 8;
    for (int t = 0; t < TRAINS; t++) {
        pthread_mutex_init(&t_lock[t], NULL);
        for (int c = 0; c < COACHES; c++) pthread_mutex_init(&c_lock[t][c], NULL);
    }
    const char *names[] = {"no lock", "one global lock", "one lock per train", "one lock per coach"};
    int order[] = {1, 2, 3, 0};
    printf("%d agents x %d attempts, %d seats in total\n", agents, ATTEMPTS, TRAINS * COACHES * SEATS);
    printf("%-20s %12s %14s %12s\n", "locking", "time (ms)", "bookings/s", "double-sold");
    for (int o = 0; o < 4; o++) {
        mode = order[o];
        memset(sold, 0, sizeof sold); booked_total = 0;
        pthread_t th[64];
        double t0 = now_s();
        for (int i = 0; i < agents; i++) pthread_create(&th[i], NULL, agent, (void *)(long)i);
        for (int i = 0; i < agents; i++) pthread_join(th[i], NULL);
        double dt = now_s() - t0;
        int dbl = 0;
        for (int t = 0; t < TRAINS; t++) for (int c = 0; c < COACHES; c++) for (int s = 0; s < SEATS; s++)
            if (sold[t][c][s] > 1) dbl += sold[t][c][s] - 1;
        printf("%-20s %12.1f %14.0f %12d\n", names[mode], dt * 1e3, agents * (double)ATTEMPTS / dt, dbl);
    }
    return 0;
}
