# CSA04 – Operating Systems – Assignment 1 (CO1)
**Evaluate and Justify the Operating-System Design of a Smart Hospital**

| | |
|---|---|
| **Name** | YojithP |
| **Register Number** | 1921260234 |
| **Course** | CSA04 – Operating Systems |
| **Report** | [`REPORT.md`](REPORT.md) (also submitted as a Word document) |

## Environment used

| Item | Value |
|---|---|
| Machine | Apple M4 (10 cores: 4 performance + 6 efficiency), 16 GB RAM |
| OS used for the runs in `outputs/` | macOS 27.0.1 (Darwin 27, XNU kernel – a POSIX/UNIX system) |
| Compiler | Apple clang 21.0.0 (invoked as `gcc`) |
| Target for the Linux-only parts | Ubuntu 24.04 LTS, gcc 13, strace 6.x (college lab) |

Exact compile command used for every program:

```
gcc -std=c11 -Wall -Wextra -O2 -pthread <file>.c -o <name>
```
(on macOS add `-D_DARWIN_C_SOURCE`; `Task03/syscall_vs_ipc.c` is compiled with `-D_GNU_SOURCE` on Linux.)

All programs are written for Linux and also build on macOS; the few Linux-only calls
(`clock_nanosleep`, `SCHED_FIFO`, `sched_setaffinity`, `posix_fadvise`, `/proc`) are inside `#ifdef __linux__`.
Each task folder has a `run.sh` that compiles everything and runs it on Linux, including the
`strace -c`, `strace -f` and `/usr/bin/time` commands; results go to `TaskNN/outputs/`.

## Repository layout and how to run each task

| Folder | Files | How to run | Result (short note) |
|---|---|---|---|
| Task01 | `alarm_latency.c` | `./alarm_latency 5000` (Linux: `sudo ./alarm_latency 5000 rt` for SCHED_FIFO) | 1 ms periodic task on a general-purpose OS: worst wake-up 7.2 ms idle, 99.9th pct 3.2 ms – no bound is guaranteed |
| Task02 | `imaging_smp.c` | `./imaging_smp 20` | SMP scaling of an MRI-style filter: 8.2x speed-up on 10 cores, flattens after 8 threads |
| Task03 | `syscall_vs_ipc.c`, `faulty_driver.c` | `./syscall_vs_ipc`, `./faulty_driver` | system call 155 ns vs microkernel-style message round trip 3,075 ns (20x); faulty driver kills a monolithic kernel but is restarted by a microkernel supervisor |
| Task04 | `services_footprint.sh`, `record_protection.sh` | Linux: `sh services_footprint.sh`, `sudo sh record_protection.sh` | service footprint of a full OS; file permissions stop another user reading a record (EACCES) |
| Task05 | `read_demo.c`, `raw_syscall.c`, `stdio_vs_syscall.c`, `patient_record.txt` | `./read_demo patient_record.txt`, `strace ./read_demo patient_record.txt`, `./raw_syscall` | `read()` traced step by step; `write` issued with a raw `svc` trap, arguments in registers |
| Task06 | `copy_file.c` (**Complete the Code**), `copy_cli.c`, `copy_bench.c`, `mmap_copy.c`, `syscall_cost.c` | `./copy_file`, `strace -c ./copy_file`, `./copy_bench backup_src.dat 8` | output matches expected exactly (10001 / 21 / 4); system call 31x a function call; best buffer 64 KB |
| Task07 | `shell_jobs.c` (**Complete the Code**), `shell_jobs_dup.c`, `mysh.c`, `zombie_orphan.c`, `spawn_bench.c` | `./shell_jobs`, `./shell_jobs_dup > f.txt`, `./mysh`, `./zombie_orphan`, `./spawn_bench 1000` | output matches expected exactly; duplicated lines explained; zombie `Z` and orphan re-parented to PID 1 observed |
| Task08 | `privileged.c`, `poll_vs_interrupt.c` | `./privileged`, `./poll_vs_interrupt 10` | disabling interrupts from user mode → SIGILL; polling every 20 ms misses the 10 ms deadline |
| Task09 | `mem_hierarchy.c`, `record_lookup.c` | `./mem_hierarchy`, `./record_lookup` | measured hierarchy: ~1 ns (L1) → ~3–9 ns (L2/SLC) → 70–90 ns (DRAM) |
| Task10 | `create_bench.c` (+ `../Task06/copy_cli.c`) | `./create_bench proc 1000`, `./create_bench thread 1000` | 1,000 processes ≈ 320–380 ms, 1,000 threads ≈ 3.5 ms (~100x) |
| Task11 | `hospital_os_plan.yaml` | (configuration / plan file) | final per-environment OS plan |
| Task12 | `home_monitor.c`, `revised_plan.yaml` | `./home_monitor` | edge alarm 4/4 episodes vs cloud alarm 1/4 when the home internet drops |

## Complete-the-Code results (verified)

`Task06/copy_file.c`:
```
buffer    1 bytes: 10001 read() calls
buffer  512 bytes:    21 read() calls
buffer 4096 bytes:     4 read() calls
```
`Task07/shell_jobs.c`:
```
shell: running job 1
42
job 1 exit status 0
shell: running job 2
job 2 exit status 3
```

## Linux-lab commands (strace evidence)
```
cd Task05 && strace ./read_demo patient_record.txt
cd Task06 && strace -c ./copy_file
cd Task07 && strace -f -e trace=clone,execve,wait4,exit_group ./shell_jobs
cd TaskNN && ./run.sh        # runs everything for that task and saves outputs/
```
