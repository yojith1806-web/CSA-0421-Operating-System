/* Q3 alternative concept: the same 64-camera job with PROCESSES instead of
   threads. Each of P worker processes reads cameras c = w, w+P, w+2P, ... and
   sends (camera, plate) results back to the parent through a pipe, because
   processes do not share variables. Compare with plates.c.
   usage: ./plates_proc [workers]                                             */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/wait.h>

#define CAMS 64
#define W 640
#define H 480
#define PASSES 4

static unsigned char *frames[CAMS];

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static unsigned long read_plate(int cam)
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
    return sum;
}

int main(int argc, char **argv)
{
    int P = argc > 1 ? atoi(argv[1]) : 8;
    for (int c = 0; c < CAMS; c++) {
        frames[c] = malloc(W * H);
        for (int i = 0; i < W * H; i++) frames[c][i] = (unsigned char)((i * 7 + c * 13) ^ (i >> 5));
    }
    int fd[2]; pipe(fd);
    double t0 = now_ms();
    for (int w = 0; w < P; w++) {
        if (fork() == 0) {
            close(fd[0]);
            for (int c = w; c < CAMS; c += P) {
                unsigned long res[2] = {(unsigned long)c, read_plate(c)};
                write(fd[1], res, sizeof res);           /* IPC: copy result to parent */
            }
            _exit(0);
        }
    }
    close(fd[1]);
    unsigned long plate[CAMS], res[2]; int got = 0;
    while (read(fd[0], res, sizeof res) == (ssize_t)sizeof res) { plate[res[0]] = res[1]; got++; }
    while (wait(NULL) > 0) {}
    double t = now_ms() - t0;
    unsigned long ck = 0; for (int c = 0; c < CAMS; c++) ck ^= plate[c];
    printf("%d worker processes: %d results via pipe, time %.1f ms (checksum %lx)\n", P, got, t, ck);
    return 0;
}
