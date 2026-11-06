/*
 * poll_vs_interrupt.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 8 alternative concept: polling versus interrupt-driven waiting.
   A "sensor" process produces an abnormal reading at a random moment.
   The monitor detects it either
     (a) by POLLING: check the sensor, sleep P ms, check again ... or
     (b) INTERRUPT-STYLE: block in read(); the kernel wakes the monitor when
         data arrives (the pipe write plays the role of the device interrupt).
   We measure detection delay and the CPU time burned while waiting.
   usage: ./poll_vs_interrupt [trials]                                       */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <sys/wait.h>
#include <sys/resource.h>

static double now_us(void)
{
    struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec * 1e6 + t.tv_nsec / 1e3;
}

static double cpu_ms(void)
{
    struct rusage u; getrusage(RUSAGE_SELF, &u);
    return u.ru_utime.tv_sec * 1e3 + u.ru_utime.tv_usec / 1e3 + u.ru_stime.tv_sec * 1e3 + u.ru_stime.tv_usec / 1e3;
}

/* returns detection delay in us; mode 0 = interrupt, otherwise poll interval in us (-1 = busy spin) */
static double trial(long poll_us, int delay_ms)
{
    int p[2]; pipe(p);
    pid_t s = fork();
    if (s == 0) {                                   /* the sensor */
        close(p[0]);
        usleep((useconds_t)delay_ms * 1000);
        double t = now_us();
        write(p[1], &t, sizeof t);                  /* abnormal reading + timestamp */
        _exit(0);
    }
    close(p[1]);
    double sent;
    if (poll_us == 0) {
        read(p[0], &sent, sizeof sent);             /* sleep until the "interrupt" */
    } else {
        fcntl(p[0], F_SETFL, O_NONBLOCK);
        while (read(p[0], &sent, sizeof sent) != (ssize_t)sizeof sent)
            if (poll_us > 0) usleep((useconds_t)poll_us);
    }
    double d = now_us() - sent;
    close(p[0]); waitpid(s, NULL, 0);
    return d;
}

int main(int argc, char **argv)
{
    int trials = argc > 1 ? atoi(argv[1]) : 20;
    long modes[] = {0, -1, 100, 1000, 5000, 20000};
    const char *names[] = {"interrupt (blocking read)", "busy polling (no sleep)", "poll every 0.1 ms",
                           "poll every 1 ms", "poll every 5 ms", "poll every 20 ms"};
    srand(1234);
    printf("%-27s %12s %12s %16s\n", "method", "avg delay", "worst delay", "CPU per trial");
    for (int m = 0; m < 6; m++) {
        double sum = 0, worst = 0, c0 = cpu_ms();
        for (int i = 0; i < trials; i++) {
            double d = trial(modes[m], 50 + rand() % 50);
            sum += d; if (d > worst) worst = d;
        }
        double cpu = (cpu_ms() - c0) / trials;
        printf("%-27s %10.0fus %10.0fus %13.2f ms  %s\n", names[m], sum / trials, worst, cpu,
               worst > 10000 ? "<-- misses 10 ms alarm deadline" : "");
    }
    printf("(each trial waits 50-100 ms for the abnormal reading)\n");
    return 0;
}
