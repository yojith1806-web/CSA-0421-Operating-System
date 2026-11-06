/*
 * copy_cli.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 6 / Task 10: the same read()/write() copy as copy_file.c, but taking the
   buffer size on the command line so it can be run under "strace -c" and
   "time" one size at a time.
   usage: ./copy_cli SRC DST BUFSIZE                                         */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 4) { fprintf(stderr, "usage: %s SRC DST BUFSIZE\n", argv[0]); return 2; }
    size_t b = (size_t)atol(argv[3]);
    char *buf = malloc(b);
    int in = open(argv[1], O_RDONLY);
    int out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (in < 0 || out < 0) { perror("open"); return 1; }
    long calls = 0;
    ssize_t n;
    for (;;) {
        n = read(in, buf, b); calls++;
        if (n <= 0) break;
        write(out, buf, (size_t)n);
    }
    close(in); close(out); free(buf);
    fprintf(stderr, "buffer %zu bytes: %ld read() calls\n", b, calls);
    return 0;
}
