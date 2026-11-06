/*
 * mmap_copy.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 6 alternative concept: copy with memory-mapped files instead of
   read()/write() with a buffer. Only a handful of system calls are made no
   matter how large the file is; the data is copied once, page by page, as
   page faults bring it in.
   usage: ./mmap_copy SRC DST                                                 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>

int main(int argc, char **argv)
{
    if (argc != 3) { fprintf(stderr, "usage: %s SRC DST\n", argv[0]); return 2; }
    int in = open(argv[1], O_RDONLY);
    int out = open(argv[2], O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (in < 0 || out < 0) { perror("open"); return 1; }
    struct stat st; fstat(in, &st);
    size_t len = (size_t)st.st_size;
    if (ftruncate(out, st.st_size) != 0) { perror("ftruncate"); return 1; }
    char *s = mmap(NULL, len, PROT_READ, MAP_PRIVATE, in, 0);
    char *d = mmap(NULL, len, PROT_READ | PROT_WRITE, MAP_SHARED, out, 0);
    if (s == MAP_FAILED || d == MAP_FAILED) { perror("mmap"); return 1; }
    memcpy(d, s, len);                       /* no read()/write() at all */
    munmap(s, len); munmap(d, len);
    close(in); close(out);
    fprintf(stderr, "mmap copy of %zu bytes done\n", len);
    return 0;
}
