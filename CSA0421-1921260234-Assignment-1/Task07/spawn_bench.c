/*
 * spawn_bench.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 7 alternative concept: start 1,000 commands with
     (a) fork() + exec()      (b) posix_spawn()      (c) threads (no new program)
   and measure the total time. The parent first touches a 64 MB buffer so it
   is a "big" process like a database server: fork() must copy its page
   tables (copy-on-write), posix_spawn() (vfork-based in glibc) does not.
   usage: ./spawn_bench [count]                                              */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <spawn.h>
#include <pthread.h>
#include <time.h>
#include <sys/wait.h>

extern char **environ;
static volatile long work_sink;

static double now_ms(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e3 + t.tv_nsec / 1e6;
}

static void *thread_job(void *arg) { work_sink += (long)arg; return NULL; }

int main(int argc, char **argv)
{
    int n = argc > 1 ? atoi(argv[1]) : 1000;
    size_t big = 64u << 20;
    char *mem = malloc(big); memset(mem, 1, big);         /* 64 MB resident in parent */
    char *const cmd[] = { "/bin/true", NULL };

    double a = now_ms();
    for (int i = 0; i < n; i++) {
        pid_t p = fork();
        if (p == 0) { execv(cmd[0], cmd); _exit(127); }
        waitpid(p, NULL, 0);
    }
    double t_fork = now_ms() - a;

    a = now_ms();
    for (int i = 0; i < n; i++) {
        pid_t p;
        posix_spawn(&p, cmd[0], NULL, NULL, cmd, environ);
        waitpid(p, NULL, 0);
    }
    double t_spawn = now_ms() - a;

    a = now_ms();
    for (int i = 0; i < n; i++) {
        pthread_t t;
        pthread_create(&t, NULL, thread_job, (void *)(long)i);
        pthread_join(t, NULL);
    }
    double t_thr = now_ms() - a;

    printf("parent resident size: 64 MB, commands started: %d\n", n);
    printf("%-22s %10s %14s\n", "method", "total(ms)", "per start(us)");
    printf("%-22s %10.1f %14.1f\n", "fork()+exec()", t_fork, t_fork * 1e3 / n);
    printf("%-22s %10.1f %14.1f\n", "posix_spawn()", t_spawn, t_spawn * 1e3 / n);
    printf("%-22s %10.1f %14.1f   (cannot run a separate program)\n", "pthread_create()", t_thr, t_thr * 1e3 / n);
    free(mem);
    return 0;
}
