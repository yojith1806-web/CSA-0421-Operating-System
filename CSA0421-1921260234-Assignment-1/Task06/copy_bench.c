/*
 * copy_bench.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 6 / Task 10 evidence: time and read()/write() counts for the hospital
   backup copy at many buffer sizes, plus fread/fwrite (library buffering) and
   mmap for comparison.
   usage: ./copy_bench SRC [MB]   (creates SRC of MB megabytes if missing)   */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/stat.h>

static long g_reads, g_writes;

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static void copy_rw(const char *src, const char *dst, size_t b)
{
    char *buf = malloc(b);
    int in = open(src, O_RDONLY), out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    ssize_t n;
    g_reads = g_writes = 0;
    for (;;) {
        n = read(in, buf, b); g_reads++;
        if (n <= 0) break;
        write(out, buf, (size_t)n); g_writes++;
    }
    close(in); close(out); free(buf);
}

static void copy_stdio(const char *src, const char *dst, size_t b)
{
    char *buf = malloc(b);
    FILE *in = fopen(src, "rb"), *out = fopen(dst, "wb");
    size_t n;
    g_reads = g_writes = 0;
    while ((n = fread(buf, 1, b, in)) > 0) { fwrite(buf, 1, n, out); g_reads++; }
    fclose(in); fclose(out); free(buf);
}

static void copy_mmap(const char *src, const char *dst)
{
    int in = open(src, O_RDONLY), out = open(dst, O_RDWR | O_CREAT | O_TRUNC, 0644);
    struct stat st; fstat(in, &st);
    ftruncate(out, st.st_size);
    void *s = mmap(NULL, (size_t)st.st_size, PROT_READ, MAP_PRIVATE, in, 0);
    void *d = mmap(NULL, (size_t)st.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, out, 0);
    memcpy(d, s, (size_t)st.st_size);
    munmap(s, (size_t)st.st_size); munmap(d, (size_t)st.st_size);
    close(in); close(out);
}

int main(int argc, char **argv)
{
    const char *src = argc > 1 ? argv[1] : "backup_src.dat";
    long mb = argc > 2 ? atol(argv[2]) : 8;
    struct stat st;
    if (stat(src, &st) != 0) {
        FILE *f = fopen(src, "wb");
        for (long i = 0; i < mb * 1024 * 1024; i++) fputc('A' + i % 26, f);
        fclose(f); stat(src, &st);
    }
    printf("file size: %lld bytes (%.1f MB)\n", (long long)st.st_size, st.st_size / 1048576.0);
    printf("%-14s %10s %12s %12s %10s %12s\n", "method", "buffer", "read()", "write()", "time(ms)", "MB/s");
    size_t sizes[] = {1, 16, 64, 512, 1024, 4096, 8192, 16384, 65536, 131072, 1048576};
    copy_rw(src, "backup_dst.dat", 1048576);         /* warm the page cache once */
    for (size_t i = 0; i < sizeof sizes / sizeof *sizes; i++) {
        double a = now_ms(); copy_rw(src, "backup_dst.dat", sizes[i]); double t = now_ms() - a;
        printf("%-14s %10zu %12ld %12ld %10.1f %12.1f\n", "read/write", sizes[i], g_reads, g_writes, t,
               st.st_size / 1048576.0 / (t / 1e3));
    }
    size_t ssz[] = {1, 64, 4096};
    for (size_t i = 0; i < 3; i++) {
        double a = now_ms(); copy_stdio(src, "backup_dst.dat", ssz[i]); double t = now_ms() - a;
        printf("%-14s %10zu %12s %12s %10.1f %12.1f   (%ld fread calls)\n", "fread/fwrite", ssz[i], "~n/4096", "~n/4096", t,
               st.st_size / 1048576.0 / (t / 1e3), g_reads);
    }
    double a = now_ms(); copy_mmap(src, "backup_dst.dat"); double t = now_ms() - a;
    printf("%-14s %10s %12s %12s %10.1f %12.1f\n", "mmap+memcpy", "-", "0", "0", t, st.st_size / 1048576.0 / (t / 1e3));
    unlink("backup_dst.dat");
    return 0;
}
