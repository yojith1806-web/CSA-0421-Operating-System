/* Q2: one simulator, six policies, same job set.
   FCFS, SJF (non-preemptive), SRTF, Priority (non-preemptive, lower number =
   higher priority), Round Robin (q = 3) and a 3-level MLFQ (q = 2, 4, FCFS).
   Prints the Gantt chart and per-process WT / TAT / RT and the averages.
   Part 2 runs the "video service" mix (short thumbnail jobs + long film jobs).
   Part 3 shows starvation in priority scheduling and how ageing removes it. */
#include <stdio.h>
#include <string.h>

#define MAXP 16

typedef struct { const char *name; int arr, burst, prio; } Job;
typedef struct { int n; Job j[MAXP]; } JobSet;

static int finish_t[MAXP], first_t[MAXP];

#define GCAP 65536
static void gantt_add(char *g, const char *name, int a, int b)
{
    char seg[32]; snprintf(seg, sizeof seg, " %s(%d-%d)", name, a, b);
    if (strlen(g) + strlen(seg) < GCAP - 1) strcat(g, seg);   /* never overflow */
}

/* generic one-unit-at-a-time simulator; pick() chooses the next process */
typedef int (*Picker)(const JobSet *, const int *left, int t, int cur);

static int pick_fcfs(const JobSet *s, const int *left, int t, int cur)
{
    if (cur >= 0 && left[cur] > 0) return cur;            /* non-preemptive */
    int b = -1;
    for (int i = 0; i < s->n; i++)
        if (left[i] > 0 && s->j[i].arr <= t && (b < 0 || s->j[i].arr < s->j[b].arr)) b = i;
    return b;
}
static int pick_sjf(const JobSet *s, const int *left, int t, int cur)
{
    if (cur >= 0 && left[cur] > 0) return cur;
    int b = -1;
    for (int i = 0; i < s->n; i++)
        if (left[i] > 0 && s->j[i].arr <= t && (b < 0 || s->j[i].burst < s->j[b].burst)) b = i;
    return b;
}
static int pick_srtf(const JobSet *s, const int *left, int t, int cur)
{
    (void)cur; int b = -1;
    for (int i = 0; i < s->n; i++)
        if (left[i] > 0 && s->j[i].arr <= t && (b < 0 || left[i] < left[b])) b = i;
    return b;
}
static int pick_prio(const JobSet *s, const int *left, int t, int cur)
{
    if (cur >= 0 && left[cur] > 0) return cur;
    int b = -1;
    for (int i = 0; i < s->n; i++)
        if (left[i] > 0 && s->j[i].arr <= t && (b < 0 || s->j[i].prio < s->j[b].prio)) b = i;
    return b;
}

static void run_picker(const char *title, const JobSet *s, Picker pick, int verbose)
{
    int left[MAXP], t = 0, done = 0, cur = -1, segstart = 0;
    static char g[GCAP]; g[0] = 0;
    for (int i = 0; i < s->n; i++) { left[i] = s->j[i].burst; first_t[i] = -1; }
    while (done < s->n) {
        int p = pick(s, left, t, cur);
        if (p != cur) {
            if (cur >= 0 && t > segstart) gantt_add(g, s->j[cur].name, segstart, t);
            else if (cur < 0 && t > segstart) gantt_add(g, "idle", segstart, t);
            segstart = t; cur = p;
        }
        if (p < 0) { t++; continue; }
        if (first_t[p] < 0) first_t[p] = t;
        left[p]--; t++;
        if (left[p] == 0) { finish_t[p] = t; done++; }
    }
    gantt_add(g, s->j[cur].name, segstart, t);
    double w = 0, ta = 0, r = 0;
    if (verbose) printf("\n%s\nGantt:%s\n%-8s %4s %4s %4s\n", title, g, "Job", "WT", "TAT", "RT");
    for (int i = 0; i < s->n; i++) {
        int tat = finish_t[i] - s->j[i].arr, wt = tat - s->j[i].burst, rt = first_t[i] - s->j[i].arr;
        w += wt; ta += tat; r += rt;
        if (verbose) printf("%-8s %4d %4d %4d\n", s->j[i].name, wt, tat, rt);
    }
    printf("%-22s avg WT %7.2f   avg TAT %7.2f   avg RT %7.2f\n", title, w / s->n, ta / s->n, r / s->n);
}

/* Round Robin (and MLFQ) need a queue, so they get their own simulators */
static void run_rr(const char *title, const JobSet *s, int q, int verbose)
{
    int left[MAXP], added[MAXP] = {0}, queue[4096], h = 0, tl = 0, t = 0, done = 0;
    static char g[GCAP]; g[0] = 0;
    for (int i = 0; i < s->n; i++) { left[i] = s->j[i].burst; first_t[i] = -1; }
    for (int i = 0; i < s->n; i++) if (s->j[i].arr <= 0) { queue[tl++] = i; added[i] = 1; }
    while (done < s->n) {
        if (h == tl) {                                     /* CPU idle until next arrival */
            t++;
            for (int i = 0; i < s->n; i++) if (!added[i] && s->j[i].arr <= t) { queue[tl++] = i; added[i] = 1; }
            continue;
        }
        int p = queue[h++];
        if (first_t[p] < 0) first_t[p] = t;
        int run = left[p] < q ? left[p] : q;
        gantt_add(g, s->j[p].name, t, t + run);
        t += run; left[p] -= run;
        for (int i = 0; i < s->n; i++) if (!added[i] && s->j[i].arr <= t) { queue[tl++] = i; added[i] = 1; }
        if (left[p] > 0) queue[tl++] = p; else { finish_t[p] = t; done++; }
    }
    double w = 0, ta = 0, r = 0;
    if (verbose) printf("\n%s\nGantt:%s\n%-8s %4s %4s %4s\n", title, g, "Job", "WT", "TAT", "RT");
    for (int i = 0; i < s->n; i++) {
        int tat = finish_t[i] - s->j[i].arr, wt = tat - s->j[i].burst, rt = first_t[i] - s->j[i].arr;
        w += wt; ta += tat; r += rt;
        if (verbose) printf("%-8s %4d %4d %4d\n", s->j[i].name, wt, tat, rt);
    }
    printf("%-22s avg WT %7.2f   avg TAT %7.2f   avg RT %7.2f\n", title, w / s->n, ta / s->n, r / s->n);
}

/* MLFQ: Q0 (q=2) -> Q1 (q=4) -> Q2 (FCFS). New jobs enter Q0; a job that uses
   its whole quantum drops one level. Higher queues always run first.        */
static void run_mlfq(const char *title, const JobSet *s, int verbose, double *short_wt, double *long_wt, int long_threshold)
{
    int left[MAXP], added[MAXP] = {0}, lvl[MAXP];
    int qs[3][4096], h[3] = {0}, tl[3] = {0}, quantum[3] = {2, 4, 1 << 30};
    int t = 0, done = 0;
    static char g[GCAP]; g[0] = 0;
    for (int i = 0; i < s->n; i++) { left[i] = s->j[i].burst; first_t[i] = -1; lvl[i] = 0; }
    for (;;) {
        for (int i = 0; i < s->n; i++) if (!added[i] && s->j[i].arr <= t) { qs[0][tl[0]++] = i; added[i] = 1; }
        if (done == s->n) break;
        int L = 0; while (L < 3 && h[L] == tl[L]) L++;
        if (L == 3) { t++; continue; }
        int p = qs[L][h[L]++];
        if (first_t[p] < 0) first_t[p] = t;
        int run = 0;
        while (run < quantum[L] && left[p] > 0) {           /* run one unit at a time so a */
            run++; left[p]--; t++;                           /* new arrival can preempt Q1/Q2 */
            int arrived = 0;
            for (int i = 0; i < s->n; i++) if (!added[i] && s->j[i].arr <= t) { qs[0][tl[0]++] = i; added[i] = 1; arrived = 1; }
            if (arrived && L > 0) break;
        }
        gantt_add(g, s->j[p].name, t - run, t);
        if (left[p] == 0) { finish_t[p] = t; done++; }
        else if (run == quantum[L] && L < 2) { lvl[p] = L + 1; qs[L + 1][tl[L + 1]++] = p; }
        else qs[L][tl[L]++] = p;                             /* preempted: same level */
    }
    double w = 0, ta = 0, r = 0, sw = 0, lw = 0; int ns = 0, nl = 0;
    if (verbose) printf("\n%s\nGantt:%s\n%-8s %4s %4s %4s\n", title, g, "Job", "WT", "TAT", "RT");
    for (int i = 0; i < s->n; i++) {
        int tat = finish_t[i] - s->j[i].arr, wt = tat - s->j[i].burst, rt = first_t[i] - s->j[i].arr;
        w += wt; ta += tat; r += rt;
        if (s->j[i].burst >= long_threshold) { lw += wt; nl++; } else { sw += wt; ns++; }
        if (verbose) printf("%-8s %4d %4d %4d\n", s->j[i].name, wt, tat, rt);
    }
    printf("%-22s avg WT %7.2f   avg TAT %7.2f   avg RT %7.2f\n", title, w / s->n, ta / s->n, r / s->n);
    if (short_wt) *short_wt = ns ? sw / ns : 0;
    if (long_wt) *long_wt = nl ? lw / nl : 0;
}

static void short_long(const char *title, const JobSet *s, int thr)
{
    double sw = 0, lw = 0; int ns = 0, nl = 0;
    for (int i = 0; i < s->n; i++) {
        int wt = finish_t[i] - s->j[i].arr - s->j[i].burst;
        if (s->j[i].burst >= thr) { lw += wt; nl++; } else { sw += wt; ns++; }
    }
    printf("    %-18s short jobs avg WT %7.2f   long jobs avg WT %7.2f\n", title, sw / ns, lw / nl);
}

int main(void)
{
    /* Part 1: the assignment's job set (+ priorities for the priority policy) */
    JobSet a = {4, {{"P1", 0, 8, 2}, {"P2", 1, 4, 1}, {"P3", 2, 9, 4}, {"P4", 3, 5, 3}}};
    printf("=== Part 1: P1-P4 (arrival 0,1,2,3; burst 8,4,9,5; priority 2,1,4,3) ===\n");
    run_picker("FCFS", &a, pick_fcfs, 1);
    run_picker("SJF (non-preemptive)", &a, pick_sjf, 1);
    run_picker("SRTF (preemptive SJF)", &a, pick_srtf, 1);
    run_picker("Priority (non-preempt)", &a, pick_prio, 1);
    run_rr("Round Robin q=3", &a, 3, 1);
    run_mlfq("MLFQ (2,4,FCFS)", &a, 1, NULL, NULL, 100);

    /* Part 2: video service. Time unit = 1 s. Two 1,200 s films + eight 2 s thumbnails */
    JobSet v = {10, {{"F1", 0, 1200, 3}, {"T1", 1, 2, 1}, {"T2", 2, 2, 1}, {"F2", 3, 1200, 3},
                     {"T3", 4, 2, 1}, {"T4", 5, 2, 1}, {"T5", 6, 2, 1}, {"T6", 7, 2, 1},
                     {"T7", 8, 2, 1}, {"T8", 9, 2, 1}}};
    printf("\n=== Part 2: video service (2 films of 1200 s, 8 thumbnails of 2 s) ===\n");
    run_picker("FCFS", &v, pick_fcfs, 0);        short_long("FCFS", &v, 100);
    run_picker("SJF", &v, pick_sjf, 0);          short_long("SJF", &v, 100);
    run_picker("SRTF", &v, pick_srtf, 0);        short_long("SRTF", &v, 100);
    run_rr("RR q=3", &v, 3, 0);                  short_long("RR q=3", &v, 100);
    run_rr("RR q=20", &v, 20, 0);                short_long("RR q=20", &v, 100);
    double sw, lw;
    run_mlfq("MLFQ (2,4,FCFS)", &v, 0, &sw, &lw, 100);
    printf("    %-18s short jobs avg WT %7.2f   long jobs avg WT %7.2f\n", "MLFQ", sw, lw);
    return 0;
}
