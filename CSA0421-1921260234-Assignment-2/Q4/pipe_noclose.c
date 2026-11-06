/* Q4 experiment: the Complete-the-Code program with close(fd[1]) REMOVED in the
   parent. The child's read() never returns 0 because a write end of the pipe
   is still open (the parent's), so the child blocks forever and the parent
   blocks forever in waitpid(): the program hangs.
   Run with a time limit, e.g.:  perl -e 'alarm 3; exec @ARGV' ./pipe_noclose */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    int fd[2];
    if (pipe(fd) == -1) { perror("pipe"); return 1; }
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]);
        int r, count = 0, sum = 0, max = 0;
        while (read(fd[0], &r, sizeof r) == (ssize_t)sizeof r) {
            count++; sum += r; if (r > max) max = r;
            printf("analyser: got reading %d, waiting for more...\n", r); fflush(stdout);
        }
        printf("analyser: %d readings, average %.1f, max %d\n", count,
               count ? (double)sum / count : 0.0, max);
        _exit(0);
    }
    close(fd[0]);
    int readings[] = {31, 33, 36, 34, 41, 38};
    for (int i = 0; i < 6; i++)
        if (write(fd[1], &readings[i], sizeof readings[i]) < 0) perror("write");
    /* close(fd[1]);   <-- REMOVED */
    printf("reader: all 6 readings sent, now waiting for the analyser...\n"); fflush(stdout);
    waitpid(pid, NULL, 0);
    printf("reader: done\n");
    return 0;
}
