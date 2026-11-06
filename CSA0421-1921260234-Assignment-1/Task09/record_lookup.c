/*
 * record_lookup.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 9 evidence: effect of caching on patient-record look-up.
   100,000 fixed-size records (256 bytes) are stored in records.db.
   1,000 random records are fetched three ways:
     cold : page cache dropped for the file -> every look-up goes to the SSD
     warm : file pages already in the OS page cache (main memory)
     app  : records held in the application's own in-memory table          */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>

#define NREC 100000
#define RSZ 256
#define LOOKUPS 1000

static double now_us(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

static double lookups_file(int fd, const int *ids)
{
    char rec[RSZ]; long chk = 0;
    double a = now_us();
    for (int i = 0; i < LOOKUPS; i++) {
        pread(fd, rec, RSZ, (off_t)ids[i] * RSZ);
        chk += rec[0];
    }
    double t = (now_us() - a) / LOOKUPS;
    if (chk == 42) puts("");
    return t;
}

int main(void)
{
    int fd = open("records.db", O_RDWR | O_CREAT | O_TRUNC, 0644);
    char rec[RSZ];
    for (int i = 0; i < NREC; i++) {
        memset(rec, ' ', RSZ);
        snprintf(rec, RSZ, "P-%06d|ward %d|bed %d|HR %d|BP %d/%d", i, i % 20, i % 40, 60 + i % 40, 100 + i % 40, 60 + i % 30);
        write(fd, rec, RSZ);
    }
    fsync(fd);                                           /* make pages clean so they can be dropped */
    int ids[LOOKUPS];
    srand(7);
    for (int i = 0; i < LOOKUPS; i++) ids[i] = rand() % NREC;

#ifdef __linux__
    posix_fadvise(fd, 0, 0, POSIX_FADV_DONTNEED);        /* evict this file from the page cache */
    double cold = lookups_file(fd, ids);
#else
    fcntl(fd, F_NOCACHE, 1);                             /* macOS: bypass the unified buffer cache */
    double cold = lookups_file(fd, ids);
    fcntl(fd, F_NOCACHE, 0);
#endif
    double warm = lookups_file(fd, ids);

    char *table = malloc((size_t)NREC * RSZ);            /* application-level cache */
    pread(fd, table, (size_t)NREC * RSZ, 0);
    long chk = 0;
    double a = now_us();
    for (int i = 0; i < LOOKUPS; i++) chk += table[(size_t)ids[i] * RSZ];
    double app = (now_us() - a) / LOOKUPS;
    if (chk == 42) puts("");

    printf("records: %d x %d bytes = %.1f MB, look-ups: %d random\n", NREC, RSZ, NREC * RSZ / 1048576.0, LOOKUPS);
    printf("%-38s %10.3f us per record\n", "cold: page cache dropped (SSD)", cold);
    printf("%-38s %10.3f us per record  (%.0fx faster)\n", "warm: OS page cache (pread syscall)", warm, cold / warm);
    printf("%-38s %10.3f us per record  (%.0fx faster than cold)\n", "app: in-memory table (no syscall)", app, cold / app);
    free(table); close(fd); unlink("records.db");
    return 0;
}
