#!/bin/bash
cd "$(dirname "$0")"; exec > output.txt 2>&1
gcc -std=c11 -Wall -Wextra -o replace replace.c && echo "=== Complete the code output ===" && ./replace
gcc -std=c11 -O2 -Wall -Wextra -o sim sim.c && ./sim > results.csv && echo "=== sim.c: results.csv written ===" && cat results.csv
./sim 2>&1 >/dev/null
python3 analysis.py
