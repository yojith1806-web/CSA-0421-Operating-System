/* Q4 growth investigation: move 1 MB, 10 MB and 100 MB of sensor readings
   from a reader process to an analyser process
     (a) through a PIPE  (message passing: write() copies user->kernel,
         read() copies kernel->user = 2 copies per byte)
     (b) through SHARED MEMORY (mmap MAP_SHARED) with two named POSIX
         semaphores (empty / full) guarding a ring of 64 KB slots: the
         producer writes readings straight into the shared slot and the
         consumer reads them in place = no copy through the kernel.
   The analyser sums every reading so both sides really touch the data.     */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE         /* MAP_ANON, _SC_NPROCESSORS_ONLN on Linux */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <semaphore.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define CHUNK (64 * 1024)
#define SLOTS 8

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static void fill(int *p, size_t n, size_t base) { for (size_t i = 0; i < n; i++) p[i] = (int)((base + i) % 50); }

static double via_pipe(size_t bytes, long long *sum_out)
{
    int fd[2]; pipe(fd);
    int res[2]; pipe(res);
    double t0 = now_ms();
    pid_t pid = fork();
    if (pid == 0) {                                   /* analyser */
        close(fd[1]);
        int *buf = malloc(CHUNK); long long sum = 0; ssize_t n;
        while ((n = read(fd[0], buf, CHUNK)) > 0)
            for (ssize_t i = 0; i < n / 4; i++) sum += buf[i];
        write(res[1], &sum, sizeof sum);
        _exit(0);
    }
    close(fd[0]);
    int *buf = malloc(CHUNK);
    for (size_t done = 0; done < bytes; done += CHUNK) {
        fill(buf, CHUNK / 4, done / 4);
        write(fd[1], buf, CHUNK);
    }
    close(fd[1]);
    read(res[0], sum_out, sizeof *sum_out);
    waitpid(pid, NULL, 0);
    free(buf); close(res[0]); close(res[1]);
    return now_ms() - t0;
}

static double via_shm(size_t bytes, long long *sum_out)
{
    int *ring = mmap(NULL, SLOTS * CHUNK + 64, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANON, -1, 0);
    long long *shared_sum = (long long *)((char *)ring + SLOTS * CHUNK);
    char en[64], fn[64];
    snprintf(en, sizeof en, "/q4e_%d", getpid()); snprintf(fn, sizeof fn, "/q4f_%d", getpid());
    sem_t *empty = sem_open(en, O_CREAT | O_EXCL, 0600, SLOTS);
    sem_t *full = sem_open(fn, O_CREAT | O_EXCL, 0600, 0);
    sem_unlink(en); sem_unlink(fn);                   /* names gone, handles stay valid */
    size_t chunks = bytes / CHUNK;
    double t0 = now_ms();
    pid_t pid = fork();
    if (pid == 0) {                                   /* analyser */
        long long sum = 0;
        for (size_t c = 0; c < chunks; c++) {
            sem_wait(full);
            int *slot = (int *)((char *)ring + (c % SLOTS) * CHUNK);
            for (size_t i = 0; i < CHUNK / 4; i++) sum += slot[i];   /* read in place */
            sem_post(empty);
        }
        *shared_sum = sum;
        _exit(0);
    }
    for (size_t c = 0; c < chunks; c++) {
        sem_wait(empty);
        fill((int *)((char *)ring + (c % SLOTS) * CHUNK), CHUNK / 4, c * CHUNK / 4);  /* write in place */
        sem_post(full);
    }
    waitpid(pid, NULL, 0);
    double t = now_ms() - t0;
    *sum_out = *shared_sum;
    sem_close(empty); sem_close(full);
    munmap(ring, SLOTS * CHUNK + 64);
    return t;
}

int main(void)
{
    size_t sizes[] = {1u << 20, 10u << 20, 100u << 20};
    printf("%-8s %14s %14s %10s %s\n", "size", "pipe (ms)", "shm+sem (ms)", "pipe/shm", "sums equal");
    for (int i = 0; i < 3; i++) {
        long long s1 = 0, s2 = 0;
        double bp = 1e18, bs = 1e18;
        for (int r = 0; r < 3; r++) {
            double a = via_pipe(sizes[i], &s1); if (a < bp) bp = a;
            double b = via_shm(sizes[i], &s2);  if (b < bs) bs = b;
        }
        printf("%5zu MB %14.2f %14.2f %9.2fx %s\n", sizes[i] >> 20, bp, bs, bp / bs, s1 == s2 ? "yes" : "NO");
    }
    return 0;
}
