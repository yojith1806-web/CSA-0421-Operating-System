#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC alarm_latency.c -o alarm_latency
hog() { for i in $(seq $(($(nproc)*2))); do timeout 25 sh -c 'while :; do :; done' & done; }
{
echo "## idle system, normal (time-sharing) scheduling"
./alarm_latency 10000
echo "## CPU hogs on every core x2, normal scheduling"
hog; sleep 1; ./alarm_latency 10000; wait
echo "## CPU hogs on every core x2, SCHED_FIFO real-time priority 80"
hog; sleep 1; sudo ./alarm_latency 10000 rt; wait
} 2>&1 | tee outputs/alarm_latency.txt
