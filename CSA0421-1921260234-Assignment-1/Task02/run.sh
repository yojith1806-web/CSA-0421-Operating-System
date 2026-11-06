#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC imaging_smp.c -o imaging_smp
./imaging_smp 2>&1 | tee outputs/imaging_smp.txt
