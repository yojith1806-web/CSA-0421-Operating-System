# CSA04 Operating Systems - Assignment 3 (CO3: Memory Management)

| Field | Value |
|---|---|
| Name | **< Yojith P >** |
| Register number | **1921260234** |
| Repository | https://github.com/yojith1806-web/CSA-0421-Operating-System/tree/main |
| Compiler | gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1) |
| OS | Ubuntu 24.04.5 LTS, Linux kernel 6.18 (virtual machine, 4 GB RAM, no swap) |
| Compile command | `gcc -std=c11 -Wall -Wextra` (plus `-pthread` not needed; `-ldl` for Q1 dlopen; `-O1/-O2` for timing programs, stated in each `run.sh`) |
| Python | python3 + matplotlib (only for calculations and plots) |

## How to run
```
bash run_all.sh          # everything, or:
bash Q1/run.sh           # one task; results are written to Q1/output.txt
```
`tools/runstat.c` is a tiny replacement for `/usr/bin/time -v` (that binary is not installed in this sandbox): it prints max RSS, minor/major page faults and wall time.

## Folder contents and result of each task
| Folder | What it contains | Result in one line |
|---|---|---|
| Q1 | `libenc.c`, `libenc_v2.c`, `bank.c`, `bank_dl.c`, `run.sh`, `output.txt` | Static exe 790,072 B vs dynamic 16,448 B + 15,224 B library; library swapped with no recompilation; two processes share libc pages; missing lib -> exit 127 (dlopen version recovers) |
| Q2 | `page_translate.c` (TODO 1 completed), `bounds_test.c`, `workload.c`, `analysis.py`, charts | Output equals the expected output; 2048-byte pages recomputed by hand and matched; page table 4 MB (32-bit) / 512 GB (48-bit); EAT 130/115/111 ns |
| Q3 | `seg.c`, `segfault.c`, `layout.c`, `analysis.py`, diagrams | 8 translations incl. 3 traps; SIGSEGV exit 139; 67% external fragmentation after removing 10 of 20 segments |
| Q4 | `touch.c`, `limited.c`, `cow.c`, `analysis.py` | EAT 8,099.9 / 180 / 108 ns; max fault rate for 10% slowdown = 1 in 800,000; COW shown with smaps |
| Q5 | `replace.c` (TODO 1 completed), `sim.c`, `analysis.py`, `results.csv` | Output equals expected (15/12/9, 10/8/8, Belady 9->10); LRU/Clock/OPT curves for 3-10 frames |
| Q6 | `readspeed.c`, `raid_parity.c`, `analysis.py` | RAID capacities 24/12/20/16/12 TB; P(loss) RAID1 5.5e-5, RAID5 2.8e-4; swap 16 GB on mirrored array |
| Q7 | `rt_lock.c`, `task_sim.py` | mlock: max loop delay 2,251 us -> 14.7 us, zero faults; simulation: RT task has 0 faults when locked |
| report | `CSA04_Assignment3_Report.docx` | Full written report for Q1-Q7 |

## Honest limitations of this environment
* Sandbox has 4 GB RAM and **no swap**, so true swapping could not be provoked: Q4 uses a *software frame limit* (madvise/fadvise on a file mapping) so that every evicted page is a real page fault.
* Only one (virtual) disk exists, so SSD-vs-HDD comparison (Q6) was not possible; cold vs warm read is reported instead and the virtual disk is cached by the host.
* `fdisk` and `/usr/bin/time` are not installed; `lsblk`, `/proc/partitions` and `tools/runstat` are used instead (use `fdisk -l` / `/usr/bin/time -v` on your own machine).
* **Before submitting**: put your name/register number above, re-run the programs on your own machine, and commit progressively (one commit per task) to your own GitHub repository.
