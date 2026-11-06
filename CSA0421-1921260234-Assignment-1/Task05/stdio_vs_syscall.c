/*
 * stdio_vs_syscall.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 5 alternative concept: C library (fopen/fread/fwrite) versus direct
   open/read/write.  Both copy SRC to DST moving b bytes per call; run each
   under "strace -c" to count the real system calls.

   usage: ./stdio_vs_syscall sys   SRC DST b
          ./stdio_vs_syscall stdio SRC DST b                                */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc, char **argv)
{
    if (argc != 5) { fprintf(stderr, "usage: %s sys|stdio SRC DST b\n", argv[0]); return 2; }
    size_t b = (size_t)atol(argv[4]);
    char *buf = malloc(b);
    long calls = 0;
    if (strcmp(argv[1], "sys") == 0) {
        int in = open(argv[2], O_RDONLY), out = open(argv[3], O_WRONLY | O_CREAT | O_TRUNC, 0644);
        ssize_t n;
        while ((n = read(in, buf, b)) > 0) { write(out, buf, (size_t)n); calls++; }
        close(in); close(out);
    } else {
        FILE *in = fopen(argv[2], "rb"), *out = fopen(argv[3], "wb");
        size_t n;
        while ((n = fread(buf, 1, b, in)) > 0) { fwrite(buf, 1, n, out); calls++; }
        fclose(in); fclose(out);
    }
    fprintf(stderr, "%s: %ld library-level read calls of %zu bytes\n", argv[1], calls, b);
    free(buf);
    return 0;
}
