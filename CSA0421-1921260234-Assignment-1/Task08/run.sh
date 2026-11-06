#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC privileged.c -o privileged
$CC poll_vs_interrupt.c -o poll_vs_interrupt
./privileged | tee outputs/privileged.txt
./poll_vs_interrupt 20 | tee outputs/poll_vs_interrupt.txt
