#!/bin/bash
# Re-run every experiment. Requires gcc, python3 with matplotlib, bc.
cd "$(dirname "$0")"
gcc -std=c11 -Wall -Wextra -o tools/runstat tools/runstat.c -D_GNU_SOURCE 2>/dev/null || gcc -std=c11 -o tools/runstat tools/runstat.c
for q in Q1 Q2 Q3 Q4 Q5 Q6 Q7; do echo "== $q =="; bash $q/run.sh; done
echo "Outputs are in Qn/output.txt"
