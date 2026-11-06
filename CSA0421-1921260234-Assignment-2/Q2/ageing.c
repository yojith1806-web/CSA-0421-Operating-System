/* Q2: starvation in priority scheduling and how ageing removes it.
   A low-priority job L (priority 10) arrives at t=0. A high-priority job
   (priority 1, burst 3) arrives every 3 time units for 300 units, so the CPU
   is never free. Without ageing L never runs. With ageing, every time unit a
   job waits its priority number drops by 1 every AGE units (it becomes more
   important) until it beats the newcomers.                                   */
#include <stdio.h>

#define HORIZON 300
#define MAXJ 200

typedef struct { int arr, burst, left, prio, base; } J;

static void simulate(int age_every)
{
    J j[MAXJ]; int n = 0;
    j[n++] = (J){0, 5, 5, 10, 10};                         /* the low-priority job L */
    for (int t = 0; t < HORIZON; t += 3) j[n++] = (J){t, 3, 3, 1, 1};
    int cur = -1, L_start = -1, L_finish = -1;
    for (int t = 0; t < HORIZON + 50; t++) {
        if (cur >= 0 && j[cur].left == 0) cur = -1;
        if (cur < 0) {                                     /* non-preemptive pick */
            for (int i = 0; i < n; i++)
                if (j[i].left > 0 && j[i].arr <= t && (cur < 0 || j[i].prio < j[cur].prio)) cur = i;
        }
        if (age_every > 0)                                 /* ageing of waiting jobs */
            for (int i = 0; i < n; i++)
                if (i != cur && j[i].left > 0 && j[i].arr <= t && (t - j[i].arr) > 0 &&
                    (t - j[i].arr) % age_every == 0 && j[i].prio > 0) j[i].prio--;
        if (cur < 0) continue;
        if (cur == 0 && L_start < 0) L_start = t;
        if (--j[cur].left == 0 && cur == 0) L_finish = t + 1;
    }
    if (L_start < 0 || L_start >= HORIZON)
        printf("ageing off          : L never ran while high-priority jobs kept arriving (%d units) -> STARVED\n",
               HORIZON);
    else
        printf("ageing every %2d units: L waited %3d units, started at t=%d, finished at t=%d\n",
               age_every, L_start, L_start, L_finish);
}

int main(void)
{
    simulate(0);
    simulate(5);
    simulate(10);
    return 0;
}
