/*
 * read_demo.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 5 evidence: a program that obtains OS services through system calls.
   It opens a patient record, read()s it in 64-byte pieces and write()s each
   piece to standard output. Run it under strace to see every trap:
       strace ./read_demo patient_record.txt                                */
#define _POSIX_C_SOURCE 200809L
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

int main(int argc, char **argv)
{
    const char *path = argc > 1 ? argv[1] : "patient_record.txt";
    int fd = open(path, O_RDONLY);                  /* file management */
    if (fd < 0) { perror("open"); return 1; }
    char buf[64];
    ssize_t n;
    while ((n = read(fd, buf, sizeof buf)) > 0)     /* file management */
        write(STDOUT_FILENO, buf, (size_t)n);       /* device management (terminal) */
    close(fd);
    return 0;
}
