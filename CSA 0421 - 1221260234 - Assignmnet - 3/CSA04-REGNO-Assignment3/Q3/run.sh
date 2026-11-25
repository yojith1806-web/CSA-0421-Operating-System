#!/bin/bash
cd "$(dirname "$0")"; exec > output.txt 2>&1
gcc -std=c11 -Wall -Wextra -o seg seg.c && echo "=== 8 (segment,offset) translations ===" && ./seg
gcc -std=c11 -Wall -Wextra -o segfault segfault.c
echo "=== Trigger a segmentation fault (NULL dereference) ==="; ./segfault; echo "exit status = $? (128+11 = SIGSEGV)"
echo "=== Write to read-only code page ==="; ./segfault w; echo "exit status = $?"
gcc -std=c11 -Wall -Wextra -no-pie -o layout layout.c; echo "=== Segments of a running program (non-PIE build so code address is fixed) ==="; ./layout
python3 analysis.py
