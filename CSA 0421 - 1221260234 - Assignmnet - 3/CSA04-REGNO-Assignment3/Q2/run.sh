#!/bin/bash
cd "$(dirname "$0")"; exec > output.txt 2>&1
gcc -std=c11 -Wall -Wextra -o page_translate page_translate.c && echo "=== Complete the code: PAGE_SIZE 1024 ===" && ./page_translate
sed 's/#define PAGE_SIZE 1024/#define PAGE_SIZE 2048/' page_translate.c > page_translate_2048.c
gcc -std=c11 -Wall -Wextra -o page_translate_2048 page_translate_2048.c && echo "=== PAGE_SIZE 2048 (program) ===" && ./page_translate_2048
cat <<'H'
=== PAGE_SIZE 2048 hand calculation (page table unchanged: p0->5, p1->2, p2->-1, p3->7, p4->0, p5..7 -> -1/3/-1)
0    : p=0 d=0    frame5 -> 5*2048+0    = 10240
1500 : p=0 d=1500 frame5 -> 10240+1500 = 11740
3100 : p=1 d=1052 frame2 -> 4096+1052  = 5148
6200 : p=3 d=56   frame7 -> 14336+56   = 14392
9000 : p=4 d=808  frame0 -> 0+808      = 808   (valid now; it was invalid with 1 KB pages)
2049 : p=1 d=1    frame2 -> 4096+1    = 4097   (was a page fault with 1 KB pages)
H
gcc -std=c11 -Wall -Wextra -o bounds_test bounds_test.c && echo "=== Start/end of page and beyond last page ===" && ./bounds_test
echo "=== getconf PAGESIZE ==="; getconf PAGESIZE
gcc -std=c11 -O0 -o workload workload.c && echo "=== Page faults of a 64 MB program (expect ~16384 + startup) ===" && ../tools/runstat ./workload
python3 analysis.py
