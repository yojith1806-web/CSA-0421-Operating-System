/*
 * privileged.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 8 evidence: user mode cannot execute privileged instructions or touch
   memory that is not its own.  Each attempt traps to the kernel, which kills
   (here: signals) the offending program instead of letting it run.
     1. disable interrupts       (arm64: msr daifset,#2   x86-64: cli)
     2. kernel-only CPU control  (arm64: write SCTLR_EL1  x86-64: hlt)
     3. read a kernel address
     4. read a NULL / unmapped address
     5. read another process's memory via /proc/<pid>/mem                   */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

static sigjmp_buf env;
static void handler(int sig) { siglongjmp(env, sig); }

static void attempt(const char *what, void (*fn)(void))
{
    int sig = sigsetjmp(env, 1);
    if (sig == 0) {
        fn();
        printf("  %-38s -> executed (not privileged here)\n", what);
    } else {
        printf("  %-38s -> trapped, kernel sent %s\n", what, strsignal(sig));
    }
}

static void disable_interrupts(void)
{
#if defined(__aarch64__)
    __asm__ volatile("msr daifset, #2");
#elif defined(__x86_64__)
    __asm__ volatile("cli");
#endif
}

static void halt_cpu(void)
{
#if defined(__aarch64__)
    __asm__ volatile("msr sctlr_el1, xzr");        /* write a kernel-only control register */
#elif defined(__x86_64__)
    __asm__ volatile("hlt");
#endif
}

static void read_kernel(void)
{
#if defined(__aarch64__)
    volatile long *k = (long *)0xffff800008000000UL;
#else
    volatile long *k = (long *)0xffffffff81000000UL;
#endif
    long v = *k; (void)v;
}

static void read_null(void) { volatile long *p = (long *)8; long v = *p; (void)v; }

int main(void)
{
    struct sigaction sa; memset(&sa, 0, sizeof sa);
    sa.sa_handler = handler;
    sigaction(SIGILL, &sa, NULL); sigaction(SIGSEGV, &sa, NULL); sigaction(SIGBUS, &sa, NULL);

    printf("Running in USER mode as pid %d:\n", getpid());
    attempt("disable interrupts", disable_interrupts);
#if defined(__aarch64__)
    attempt("write system control register", halt_cpu);
#else
    attempt("halt the CPU (hlt)", halt_cpu);
#endif
    attempt("read a kernel-space address", read_kernel);
    attempt("read an unmapped address (NULL+8)", read_null);

    /* another process's memory: start a child that holds a "patient record" */
    static char secret[64] = "P-1042 HIV status: confidential";
    pid_t c = fork();
    if (c == 0) { pause(); _exit(0); }
    char path[64]; snprintf(path, sizeof path, "/proc/%d/mem", c);
    int fd = open(path, O_RDONLY);
    char buf[64] = {0};
    ssize_t r = fd >= 0 ? pread(fd, buf, sizeof buf, (off_t)(long)secret) : -1;
    if (r > 0)
        printf("  %-38s -> allowed (same user + ptrace rights): '%s'\n", "read child memory /proc/pid/mem", buf);
    else
        printf("  %-38s -> refused: %s\n", "read child memory /proc/pid/mem", strerror(errno));
    if (fd >= 0) close(fd);
    kill(c, SIGKILL); waitpid(c, NULL, 0);
    printf("The program is still alive: every violation was caught by hardware + kernel.\n");
    return 0;
}
