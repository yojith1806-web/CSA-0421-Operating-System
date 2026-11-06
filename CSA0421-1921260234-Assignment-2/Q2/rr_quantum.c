/* Q2 quantum study: the same Round Robin as rr.c, with the quantum taken from
   the command line, and a count of context switches (the CPU moving from one
   process to a different one).   usage: ./rr_quantum Q                       */
#include <stdio.h>
#include <stdlib.h>

#define N 4

int main(int argc, char **argv)
{
    const char *name[N] = {"P1", "P2", "P3", "P4"};
    int arrival[N] = {0, 1, 2, 3};
    int burst[N] = {8, 4, 9, 5};
    int quantum = argc > 1 ? atoi(argv[1]) : 3;

    int left[N], finish[N], first[N], queue[256], head = 0, tail = 0, added[N] = {0};
    for (int i = 0; i < N; i++) { left[i] = burst[i]; first[i] = -1; }
    int time = 0, done = 0, switches = 0, last = -1;
    queue[tail++] = 0; added[0] = 1;
    printf("q=%-2d Gantt:", quantum);
    while (done < N) {
        int p = queue[head++];
        if (last != -1 && last != p) switches++;
        last = p;
        if (first[p] < 0) first[p] = time;
        int run = left[p] < quantum ? left[p] : quantum;
        printf(" %s(%d-%d)", name[p], time, time + run);
        time += run; left[p] -= run;
        for (int i = 0; i < N; i++)
            if (!added[i] && arrival[i] <= time) { queue[tail++] = i; added[i] = 1; }
        if (left[p] > 0) queue[tail++] = p; else { finish[p] = time; done++; }
    }
    double tw = 0, tt = 0, tr = 0;
    for (int i = 0; i < N; i++) {
        tt += finish[i] - arrival[i];
        tw += finish[i] - arrival[i] - burst[i];
        tr += first[i] - arrival[i];
    }
    printf("\n     avg TAT %.2f  avg WT %.2f  avg RT %.2f  context switches %d\n",
           tt / N, tw / N, tr / N, switches);
    return 0;
}
