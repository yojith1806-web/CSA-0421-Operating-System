#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC mem_hierarchy.c -o mem_hierarchy
$CC record_lookup.c -o record_lookup
lscpu | grep -iE "model name|cache|^CPU\(s\)" | tee outputs/lscpu.txt
./mem_hierarchy | tee outputs/mem_hierarchy.txt
./record_lookup | tee outputs/record_lookup.txt
