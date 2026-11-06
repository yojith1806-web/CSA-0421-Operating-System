/*
 * shell_jobs_dup.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Variant asked for in Task 7: the second fflush(stdout) is removed and
   _exit(3) is changed to exit(3). Run with output redirected to a file:
       ./shell_jobs_dup > dup_out.txt
   and "shell: running job 2" appears twice (see README / report). */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    printf("shell: running job 1\n");
    fflush(stdout);
    int status;
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {
        execlp("expr", "expr", "6", "*", "7", (char *)NULL);
        exit(127);
    }
    waitpid(pid, &status, 0);
    printf("job 1 exit status %d\n", WEXITSTATUS(status));

    printf("shell: running job 2\n");
    /* fflush(stdout);   <-- removed */
    pid = fork();
    if (pid == 0) exit(3);                   /* exit() flushes the child's copy of the buffer */
    waitpid(pid, &status, 0);
    printf("job 2 exit status %d\n", WEXITSTATUS(status));
    return 0;
}
