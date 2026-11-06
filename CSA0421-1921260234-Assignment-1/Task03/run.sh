#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC -D_GNU_SOURCE syscall_vs_ipc.c -o syscall_vs_ipc
$CC faulty_driver.c -o faulty_driver
./syscall_vs_ipc 2>&1 | tee outputs/syscall_vs_ipc.txt
./faulty_driver 2>&1 | tee outputs/faulty_driver.txt
