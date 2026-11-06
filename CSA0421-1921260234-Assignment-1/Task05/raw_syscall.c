/*
 * raw_syscall.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 5 evidence: how parameters reach the kernel in registers.
   Calls write(1, msg, len) WITHOUT the C library, by loading the system-call
   number and the three arguments into registers and executing the trap
   instruction directly ("svc #0" on ARM64, "syscall" on x86-64).        */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>

static long raw_write(long fd, const void *buf, long len)
{
#if defined(__APPLE__) && defined(__aarch64__)
    register long x16 __asm__("x16") = 4;         /* SYS_write on macOS (XNU BSD layer) */
    register long x0 __asm__("x0") = fd;
    register long x1 __asm__("x1") = (long)buf;
    register long x2 __asm__("x2") = len;
    __asm__ volatile("svc #0x80" : "+r"(x0) : "r"(x16), "r"(x1), "r"(x2) : "memory", "cc");
    return x0;
#elif defined(__aarch64__)
    register long x8 __asm__("x8") = 64;          /* __NR_write on arm64 */
    register long x0 __asm__("x0") = fd;          /* arg 1 */
    register long x1 __asm__("x1") = (long)buf;   /* arg 2 */
    register long x2 __asm__("x2") = len;         /* arg 3 */
    __asm__ volatile("svc #0" : "+r"(x0) : "r"(x8), "r"(x1), "r"(x2) : "memory");
    return x0;                                    /* return value comes back in x0 */
#elif defined(__x86_64__)
    long ret;
    __asm__ volatile("syscall" : "=a"(ret)
                     : "a"(1L), "D"(fd), "S"(buf), "d"(len)  /* rax=1 (__NR_write), rdi, rsi, rdx */
                     : "rcx", "r11", "memory");
    return ret;
#else
#error "unsupported architecture"
#endif
}

int main(void)
{
    static const char msg[] = "ALARM: SpO2 below 90% on bed 17 (written with a raw trap)\n";
#if defined(__APPLE__) && defined(__aarch64__)
    printf("macOS arm64 : syscall number in x16, arguments in x0,x1,x2, trap = svc #0x80\n");
#elif defined(__aarch64__)
    printf("Linux arm64 : syscall number in x8, arguments in x0,x1,x2, trap = svc #0\n");
#else
    printf("Linux x86-64: syscall number in rax, arguments in rdi,rsi,rdx, trap = syscall\n");
#endif
    fflush(stdout);
    long r = raw_write(1, msg, sizeof msg - 1);
    printf("kernel returned %ld (bytes written)\n", r);
    return 0;
}
