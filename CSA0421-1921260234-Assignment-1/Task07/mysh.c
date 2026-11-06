/*
 * mysh.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 7 evidence: a tiny command shell for the database server.
   Reads a line, splits it into words, fork()s, the child exec()s the command,
   the parent wait()s and reports the exit status (like "echo $?").
   Built-ins: exit.   usage: ./mysh   (or: printf 'cmd\n...' | ./mysh)       */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>

int main(void)
{
    char line[512];
    for (;;) {
        printf("mysh> "); fflush(stdout);
        if (!fgets(line, sizeof line, stdin)) break;
        char *argv[32]; int argc = 0;
        for (char *tok = strtok(line, " \t\n"); tok && argc < 31; tok = strtok(NULL, " \t\n"))
            argv[argc++] = tok;
        argv[argc] = NULL;
        if (argc == 0) continue;
        if (strcmp(argv[0], "exit") == 0) break;
        printf("\n"); fflush(stdout);

        pid_t pid = fork();                              /* 1. duplicate the shell */
        if (pid < 0) { perror("fork"); continue; }
        if (pid == 0) {
            execvp(argv[0], argv);                       /* 2. child becomes the command */
            fprintf(stderr, "mysh: %s: command not found\n", argv[0]);
            _exit(127);
        }
        int status;
        waitpid(pid, &status, 0);                        /* 3. parent waits, collects status */
        if (WIFEXITED(status))
            printf("[pid %d exited with status %d]\n", pid, WEXITSTATUS(status));
        else if (WIFSIGNALED(status))
            printf("[pid %d killed by signal %d]\n", pid, WTERMSIG(status));
    }
    printf("\n");
    return 0;
}
