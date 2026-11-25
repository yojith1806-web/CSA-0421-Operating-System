#!/bin/bash
cd "$(dirname "$0")"; exec > output.txt 2>&1
gcc -std=c11 -O1 -Wall -Wextra -o rt_lock rt_lock.c && echo "=== mlock experiment (1 MB control buffer, 3000 iterations) ===" && ./rt_lock && ./rt_lock | tail -2
echo; echo "=== Page-replacement simulation of the three tasks ==="; python3 task_sim.py
