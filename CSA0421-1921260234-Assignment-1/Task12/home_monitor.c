/*
 * home_monitor.c
 * Operating Systems assignment source file.
 * The implementation and task objective are preserved.
 */
/* Task 12 evidence: a remote home monitor must decide on alarms LOCALLY.
   Simulates 1 hour of readings (1 per second) from a home device whose
   internet link drops for three outages. Two designs are compared:
     cloud-decides : reading sent to the cloud, alarm decided there
                     -> no alarm at all while the link is down, + WAN round trip
     edge-decides  : the device's RTOS task decides in place (sub-ms) and
                     stores readings in a ring buffer, forwarding them when the
                     link returns (store-and-forward)                        */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>

#define SECONDS 3600
#define RING 1024

static int link_up(int t)
{
    return !((t >= 600 && t < 900) || (t >= 1800 && t < 1830) || (t >= 2700 && t < 3300));
}

int main(void)
{
    srand(2026);
    int spo2[SECONDS];
    for (int t = 0; t < SECONDS; t++) spo2[t] = 95 + rand() % 4;
    int events[] = {700, 1815, 2900, 3400};               /* desaturation episodes */
    for (int e = 0; e < 4; e++) for (int k = 0; k < 20; k++) spo2[events[e] + k] = 86;

    int cloud_alarms = 0, cloud_missed = 0, edge_alarms = 0;
    double cloud_delay_sum = 0;
    int ring[RING], head = 0, count = 0, dropped = 0, forwarded = 0, peak = 0, outage_s = 0;

    for (int e = 0; e < 4; e++) {
        int t = events[e];
        if (link_up(t)) { cloud_alarms++; cloud_delay_sum += 80 + rand() % 170; }  /* 80-250 ms WAN RTT */
        else cloud_missed++;
        edge_alarms++;                                    /* local check: always */
    }
    for (int t = 0; t < SECONDS; t++) {
        if (link_up(t)) { forwarded += count; count = 0; }
        else {
            outage_s++;                                   /* cloud-only design loses this reading */
            ring[head] = spo2[t]; head = (head + 1) % RING;
            if (count < RING) count++; else dropped++;
            if (count > peak) peak = count;
        }
    }
    printf("1 hour, 4 desaturation episodes, 3 internet outages (300 s, 30 s, 600 s)\n\n");
    printf("%-15s %8s %8s %18s %16s\n", "design", "alarms", "missed", "alarm delay", "readings lost");
    printf("%-15s %8d %8d %15.0f ms %16d\n", "cloud-decides", cloud_alarms, cloud_missed,
           cloud_alarms ? cloud_delay_sum / cloud_alarms : 0.0, outage_s);
    printf("%-15s %8d %8d %18s %16d\n", "edge-decides", edge_alarms, 0, "< 1 ms (local)", dropped);
    printf("\nedge ring buffer: %d slots x 4 B = %d B, peak use %d slots, %d readings forwarded after reconnect\n",
           RING, RING * 4, peak, forwarded);
    return 0;
}
