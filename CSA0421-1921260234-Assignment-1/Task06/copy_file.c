/*
 * copy_file.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

/* Copy src to dst with the read() and write() system calls, using a buffer of
   bufsize bytes. Returns the number of read() calls made, or -1 on error. */
long copy_file(const char *src, const char *dst, size_t bufsize)
{
    char buf[8192];
    int in = open(src, O_RDONLY);
    if (in < 0) return -1;
    int out = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out < 0) { close(in); return -1; }
    long calls = 0;
    ssize_t n;
    // TODO 1: Loop: read up to bufsize bytes into buf (count every read call in calls);
    //   stop when read returns 0 (end of file) or -1; write exactly the n bytes that
    //   were read
    if (bufsize > sizeof buf) bufsize = sizeof buf;      /* never overflow buf */
    for (;;) {
        n = read(in, buf, bufsize);
        calls++;                                         /* the EOF read counts too */
        if (n <= 0) break;                               /* 0 = end of file, -1 = error */
        ssize_t done = 0;
        while (done < n) {                               /* write() may be partial */
            ssize_t w = write(out, buf + done, (size_t)(n - done));
            if (w < 0) { n = -1; break; }
            done += w;
        }
        if (n < 0) break;
    }
    if (n < 0) calls = -1;

    close(in);
    close(out);
    return calls;
}

int main(void)
{
    FILE *f = fopen("records.dat", "w");      /* a 10,000-byte test file */
    for (int i = 0; i < 10000; i++) fputc('A' + i % 26, f);
    fclose(f);

    size_t sizes[] = {1, 512, 4096};
    for (int i = 0; i < 3; i++)
        printf("buffer %4zu bytes: %5ld read() calls\n", sizes[i],
               copy_file("records.dat", "copy.dat", sizes[i]));
    unlink("records.dat");
    unlink("copy.dat");
    return 0;
}
