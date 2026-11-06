/* Q3: traffic-camera number-plate reader with POSIX threads.
   64 cameras each deliver a 640x480 grey frame. "Reading the plate" is
   simulated by real CPU work on the frame: an edge filter (Sobel-like) plus a
   brightness histogram, repeated, and a checksum that stands for the plate.
   Runs: sequential, one thread per camera, and pools of 4, 8 and 16 threads.
   Part 2 shows lost updates on a shared counter without protection.       */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE         /* MAP_ANON, _SC_NPROCESSORS_ONLN on Linux */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <unistd.h>

#define CAMS 64
#define W 640
#define H 480
#define PASSES 4

static unsigned char *frames[CAMS];
static unsigned long plate[CAMS];
static atomic_int next_cam;

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static void read_plate(int cam)                     /* per-camera CPU work */
{
    const unsigned char *f = frames[cam];
    unsigned long sum = 0;
    for (int p = 0; p < PASSES; p++) {
        unsigned hist[256] = {0};
        for (int y = 1; y < H - 1; y++)
            for (int x = 1; x < W - 1; x++) {
                int gx = f[y * W + x + 1] - f[y * W + x - 1];
                int gy = f[(y + 1) * W + x] - f[(y - 1) * W + x];
                int g = abs(gx) + abs(gy);
                hist[g > 255 ? 255 : g]++;
            }
        for (int i = 0; i < 256; i++) sum = sum * 31 + hist[i];
    }
    plate[cam] = sum;
}

static void *one_camera(void *arg) { read_plate((int)(long)arg); return NULL; }

static void *pool_worker(void *arg)
{
    (void)arg;
    int c;
    while ((c = atomic_fetch_add(&next_cam, 1)) < CAMS) read_plate(c);
    return NULL;
}

/* ---- Part 2: shared counter ---- */
static long counter;
static atomic_long acounter;
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
#define INC 1000000
static void *inc_plain(void *a) { (void)a; for (int i = 0; i < INC; i++) { long v = counter; __asm__ volatile("" ::: "memory"); counter = v + 1; } return NULL; }
static void *inc_mutex(void *a) { (void)a; for (int i = 0; i < INC; i++) { pthread_mutex_lock(&m); counter++; pthread_mutex_unlock(&m); } return NULL; }
static void *inc_atomic(void *a) { (void)a; for (int i = 0; i < INC; i++) atomic_fetch_add(&acounter, 1); return NULL; }

static long two_threads(void *(*fn)(void *))
{
    pthread_t a, b; counter = 0; atomic_store(&acounter, 0);
    pthread_create(&a, NULL, fn, NULL); pthread_create(&b, NULL, fn, NULL);
    pthread_join(a, NULL); pthread_join(b, NULL);
    return fn == inc_atomic ? atomic_load(&acounter) : counter;
}

int main(void)
{
    for (int c = 0; c < CAMS; c++) {
        frames[c] = malloc(W * H);
        for (int i = 0; i < W * H; i++) frames[c][i] = (unsigned char)((i * 7 + c * 13) ^ (i >> 5));
    }
    printf("online cores: %ld   cameras: %d   frame %dx%d\n", sysconf(_SC_NPROCESSORS_ONLN), CAMS, W, H);
    double t0 = now_ms();
    for (int c = 0; c < CAMS; c++) read_plate(c);
    double seq = now_ms() - t0;
    unsigned long check = 0; for (int c = 0; c < CAMS; c++) check ^= plate[c];
    printf("%-26s %9s %9s %8s\n", "method", "time(ms)", "speed-up", "same result");
    printf("%-26s %9.1f %9.2f %8s\n", "sequential (1 thread)", seq, 1.0, "yes");

    pthread_t th[CAMS];
    t0 = now_ms();
    for (int c = 0; c < CAMS; c++) pthread_create(&th[c], NULL, one_camera, (void *)(long)c);
    for (int c = 0; c < CAMS; c++) pthread_join(th[c], NULL);
    double t = now_ms() - t0;
    unsigned long ck = 0; for (int c = 0; c < CAMS; c++) ck ^= plate[c];
    printf("%-26s %9.1f %9.2f %8s\n", "one thread per camera (64)", t, seq / t, ck == check ? "yes" : "NO");

    int pools[] = {2, 4, 8, 16};
    for (int k = 0; k < 4; k++) {
        double best = 1e18;
        for (int rep = 0; rep < 3; rep++) {
            atomic_store(&next_cam, 0);
            t0 = now_ms();
            for (int i = 0; i < pools[k]; i++) pthread_create(&th[i], NULL, pool_worker, NULL);
            for (int i = 0; i < pools[k]; i++) pthread_join(th[i], NULL);
            t = now_ms() - t0; if (t < best) best = t;
        }
        ck = 0; for (int c = 0; c < CAMS; c++) ck ^= plate[c];
        char label[40]; snprintf(label, sizeof label, "pool of %d threads", pools[k]);
        printf("%-26s %9.1f %9.2f %8s\n", label, best, seq / best, ck == check ? "yes" : "NO");
    }

    printf("\nshared counter, 2 threads x %d increments (expected %d):\n", INC, 2 * INC);
    for (int r = 1; r <= 3; r++) printf("  run %d unprotected : %ld  (lost %ld updates)\n", r, two_threads(inc_plain), 2L * INC - counter);
    printf("  mutex-protected   : %ld\n", two_threads(inc_mutex));
    printf("  atomic_fetch_add  : %ld\n", two_threads(inc_atomic));
    return 0;
}
