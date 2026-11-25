#!/bin/bash
cd "$(dirname "$0")"; exec > output.txt 2>&1
for f in touch limited cow; do gcc -std=c11 -O1 -Wall -Wextra -o $f $f.c || exit 1; done
echo "=== Sequential vs random touching ==="; ./touch
echo; echo "=== Run time as available frames shrink below what is needed (software limit) ==="; ./limited; echo; ./limited locality
echo; echo "=== Copy-on-write with fork() ==="; ./cow
echo; python3 analysis.py
