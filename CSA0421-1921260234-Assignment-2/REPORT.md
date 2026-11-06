# TITLE: Operating Systems – Assignment 2 (CO2)

::center **Process Management, Scheduling, Threads, IPC, Synchronisation and Deadlock Handling**

::center Course: CSA04 – Operating Systems   |   Total Marks: 50

::center **Name:** Saai Vardhan D     **Register Number:** 192525205

::center **GitHub Repository:** https://github.com/svdchikku-design/CSA04-192525205-Assignment2

::center Test machine: Windows PC (WSL2 Ubuntu 24.04), gcc 13 (`gcc -std=c11 -Wall -Wextra -O2 -pthread`)

<!-- pagebreak -->

# Q1 – Processes and Process States (6 marks) – Online Examination Server

## Process-state diagram

```
                 admitted                         exit
     [ NEW ] -------------> [ READY ] <------+   [ TERMINATED ]
                               |    ^        |         ^
              scheduler        |    | interrupt /      |
              dispatch         v    | quantum expired  |
                            [ RUNNING ] -----------------+
                               |    ^
          I/O or event wait    |    |  (none: a waiting process
          (e.g. read answer)   v    |   always goes back to READY)
                            [ WAITING ] ---- I/O or event completes ---> [ READY ]
```

- **new → ready**: the student logs in and the session process is created (`fork`) and admitted.
- **ready → running**: the scheduler dispatches it to a CPU.
- **running → ready**: the timer interrupt ends its quantum, or a higher-priority process arrives.
- **running → waiting**: it calls `read()` for the next answer, or waits for an auto-save disk write.
- **waiting → ready**: the I/O completes and the interrupt handler wakes it.
- **running → terminated**: the session ends with `exit()`. It stays a zombie until the parent calls `wait()`.

## Process Control Block (PCB)

| PCB field | Contents for one student session |
|---|---|
| Process state | new / ready / running / waiting / terminated |
| Process ID, parent ID | PID of the session, PPID of the exam server |
| Program counter + CPU registers | Saved on every context switch, restored on dispatch |
| CPU-scheduling information | Priority / nice value, queue links, time used, quantum left |
| Memory-management information | Page-table base register, segment / limit information, memory size |
| Accounting information | CPU time used, start time, I/O counts (the `TIME` column below) |
| I/O status information | Open files (answer file, socket to the student), pending I/O requests |
| Credentials | User ID, group ID (which student, which permissions) |

On Linux the PCB is `struct task_struct`, and `/proc/<pid>/status` shows part of it (State, PPid, VmRSS, voluntary/non-voluntary context switches).

## fork() program with ps evidence (`Q1/fork_states.c`)

The exam server forks a **session** (blocked reading an answer), an **autosave** (CPU-bound) and a **proctor** (finished, not yet reaped):

```
$ ps -o pid,ppid,stat,rss,vsz,time,comm -p 13521,13523,13524,13525
  PID  PPID STAT    RSS      VSZ      TIME COMM
13521 13520 S      1648 488781424   0:00.01 ./fork_states     <- exam server (parent)
13523 13521 S      1168 488777712   0:00.00 ./fork_states     <- session: WAITING (S = sleeping)
13524 13521 R      1152 488777712   0:01.01 ./fork_states     <- autosave: RUNNING (R)
13525 13521 Z         0        0   0:00.00 <defunct>          <- proctor: TERMINATED (zombie)
reaped session  pid 13523: exited, status 0
reaped autosave pid 13524: killed by signal 15
reaped proctor  pid 13525: exited, status 0
```

Each child has the parent's PID as PPID and costs about **1.1–1.2 MB of resident memory** even when it does nothing. The zombie keeps only its PCB entry (RSS 0) until it is waited for.

## Where the time goes with 2,000 active processes

| Cost | Measured / estimated | Effect at 2,000 sessions |
|---|---|---|
| Context switch | Process ↔ process **1,594 ns**, thread ↔ thread **1,076 ns** (`ctxswitch.c`; includes the pipe call) | One switch per session = 3.2 ms per round; with 10 ms quanta and 2,000 runnable sessions, a session can wait 2,000/10 cores × 10 ms ≈ **2 s** for its turn, which shows up as a "hang" |
| Memory per process | ≈ 1.2 MB resident each (ps above), plus page tables and kernel stack | 2,000 × 1.2 MB ≈ **2.4 GB**, more with proctoring video buffers, so swapping begins |
| Waiting for I/O | Each session spends most of its life in WAITING (answers, auto-save, network) | CPUs are idle while the run queue is long, and creation and teardown dominate |
| Creation cost | `sessions_bench.c`: 2,000 sessions (0.2 ms CPU + 2 ms I/O each) took **448 ms** as processes vs **47 ms** as threads | fork + exit + wait per student costs ~10× more than the work itself |

**Judgement.** A separate process per student is **not a sound design at 2,000 students.** It gives strong isolation, but it wastes memory, multiplies creation and switching costs, and puts 2,000 runnable entities in the scheduler. This is exactly the observed slowdown and "hanging" sessions.

## Design decision

| Design | Measured time for 2,000 sessions | Live tasks | Isolation | Verdict |
|---|---|---|---|---|
| One process per student | 448 ms | 2,000 | Full | Rejected: memory and switching cost |
| One thread per student | 47 ms | 2,000 | None: one crash kills all | Rejected: 2,000 stacks, no fault isolation |
| Fixed pool of 4 / 16 / 64 workers | 1,345 / 333 / **86 ms** | 4 / 16 / 64 | Per worker | **Chosen** (sized to I/O wait) |

**Decision: a fixed pool of worker processes**, each running a small pool of threads, plus a request queue. A pool of 64 served 2,000 sessions in 86 ms (close to the thread-per-student time) with only 64 live tasks. Because session work is mostly I/O wait, the pool should be about cores × (1 + wait/compute) ≈ 10 × (1 + 2/0.2) ≈ 110. Using **processes** for the pool (e.g. 16 processes × 8 threads) keeps a crash confined to 1/16 of the students, and a supervisor restarts the failed worker. Session state (answers) lives in the database, not in the process, so any worker can serve any student.

## Alternative concept: each session as a thread in one server process

| | Process per session | Thread per session |
|---|---|---|
| Context switch | 1.59 µs measured (address-space switch, TLB effects) | 1.08 µs measured (same address space) |
| Memory per session | Full address space: page tables + ≈ 1.2 MB resident, O(size of process) | Stack (default 512 KB–8 MB virtual, a few KB actually used) + thread control block, O(stack) |
| Creation (2,000) | 448 ms | 47 ms (≈ 10× faster) |
| One session crashes (e.g. NULL pointer) | Only that student is affected | **The whole server process dies, and all 2,000 students lose their session** |
| Sharing data | Needs IPC | Shared variables, but these need locks (Q5) |

Threads win on time and space, O(1) small per session, but lose isolation. That is why the chosen design mixes the two: processes for isolation, threads inside them for cheapness.

<!-- pagebreak -->

# Q2 – CPU Scheduling (8 marks) – Cloud Video-Encoding Service

## The job set

| Job | Arrival | Burst | Priority (1 = highest) |
|---|---|---|---|
| P1 | 0 | 8 | 2 |
| P2 | 1 | 4 | 1 |
| P3 | 2 | 9 | 4 |
| P4 | 3 | 5 | 3 |

WT = TAT − burst, TAT = finish − arrival, RT = first run − arrival. All charts were produced by `Q2/sched_sim.c` and checked by hand.

## FCFS

```
| P1        | P2    | P3          | P4     |
0           8       12            21       26
```

| Job | WT | TAT | RT |
|---|---|---|---|
| P1 | 0 | 8 | 0 |
| P2 | 7 | 11 | 7 |
| P3 | 10 | 19 | 10 |
| P4 | 18 | 23 | 18 |
| **Avg** | **8.75** | **15.25** | **8.75** |

## SJF (non-preemptive)

At t = 8 the ready jobs are P2(4), P3(9) and P4(5), so the order is P2, then P4, then P3.

```
| P1        | P2    | P4     | P3          |
0           8       12       17            26
```

| Job | WT | TAT | RT |
|---|---|---|---|
| P1 | 0 | 8 | 0 |
| P2 | 7 | 11 | 7 |
| P3 | 15 | 24 | 15 |
| P4 | 9 | 14 | 9 |
| **Avg** | **7.75** | **14.25** | **7.75** |

## SRTF (preemptive SJF)

At t = 1, P2 (4) < P1's remaining 7, so P1 is preempted. At t = 5, P4 (5) < P1 (7).

```
|P1| P2    | P4     | P1        | P3          |
0  1       5        10          17            26
```

| Job | WT | TAT | RT |
|---|---|---|---|
| P1 | 9 | 17 | 0 |
| P2 | 0 | 4 | 0 |
| P3 | 15 | 24 | 15 |
| P4 | 2 | 7 | 2 |
| **Avg** | **6.50** | **13.00** | **4.25** |

## Priority (non-preemptive)

```
| P1        | P2    | P4     | P3          |
0           8       12       17            26
```

Averages: WT **7.75**, TAT **14.25**, RT **7.75**. The chart is the same as SJF here, because the priorities happen to rank the jobs in the same order.

## Round Robin (q = 3), from the Complete-the-Code program

```
| P1 | P2 | P3 | P4 | P1 |P2| P3 | P4 | P1 | P3 |
0    3    6    9    12   15 16   19   21   23   26
```

| Job | TAT | WT | RT |
|---|---|---|---|
| P1 | 23 | 15 | 0 |
| P2 | 15 | 11 | 2 |
| P3 | 24 | 15 | 4 |
| P4 | 18 | 13 | 6 |
| **Avg** | **20.00** | **13.50** | **3.00** |

## Summary

| Policy | Avg WT | Avg TAT | Avg RT |
|---|---|---|---|
| FCFS | 8.75 | 15.25 | 8.75 |
| SJF | 7.75 | 14.25 | 7.75 |
| **SRTF** | **6.50** | **13.00** | 4.25 |
| Priority | 7.75 | 14.25 | 7.75 |
| RR q=3 | 13.50 | 20.00 | 3.00 |
| MLFQ (2, 4, FCFS) | 13.00 | 19.50 | **1.50** |

SRTF is optimal for average waiting time. RR and MLFQ give the best **response** time.

## Complete the Code: TODO 1 of `rr.c`

```
int run = left[p] < quantum ? left[p] : quantum;
printf(" %s(%d-%d)", name[p], time, time + run);
time += run;
left[p] -= run;
for (int i = 0; i < N; i++)
    if (!added[i] && arrival[i] <= time) { queue[tail++] = i; added[i] = 1; }
if (left[p] > 0) queue[tail++] = p;
else { finish[p] = time; done++; }
```

The output is identical to the expected output (Gantt chart, TAT 23/15/24/18, WT 15/11/15/13, averages 20.00 and 13.50). New arrivals are queued **before** the preempted process is put back. That ordering produces P2 at 3–6 instead of P1 again.

## Quantum study (`rr_quantum.c`)

| Quantum | Avg WT | Avg TAT | Avg RT | Context switches |
|---|---|---|---|---|
| 1 | 12.50 | 19.00 | **0.75** | **23** |
| 3 | 13.50 | 20.00 | 3.00 | 9 |
| 5 | 11.00 | 17.50 | 5.50 | 5 |
| 20 | **8.75** | **15.25** | 8.75 | **3** |

- **Small quantum (1):** best response time (0.75), but 23 switches. At ~2 µs per switch plus cache refills, a real system with a 1 ms quantum would lose a noticeable share of CPU time to switching.
- **Large quantum (20 ≥ every burst):** RR becomes **FCFS** (identical chart and averages). Only 3 switches, but a short job stuck behind a long one waits for all of it.
- Average waiting time is **not monotonic** in q (13.5 at q=3, 11.0 at q=5). The rule of thumb is to pick q so that about 80 % of CPU bursts finish within one quantum.

## Starvation and ageing (`ageing.c`)

A low-priority job L (priority 10) competes with a stream of high-priority jobs arriving every 3 time units:

```
ageing off           : L never ran while high-priority jobs kept arriving (300 units) -> STARVED
ageing every  5 units: L waited  48 units, started at t=48, finished at t=53
ageing every 10 units: L waited  93 units, started at t=93, finished at t=98
```

- **SJF/SRTF** starve long jobs: a 20-minute film never runs while thumbnails keep arriving.
- **Priority** scheduling starves low-priority jobs in the same way.
- **Ageing** raises a waiting job's priority over time, so its wait is **bounded**: about (priority gap × ageing interval). Here that is 10 levels × 5 units ≈ 48 units.

## How a multilevel feedback queue treats short and long jobs

New jobs enter the top queue (Q0, q = 2). A job that uses up its whole quantum is CPU-bound, so it drops to Q1 (q = 4) and then to Q2 (FCFS). Higher queues always run first. **Short jobs (thumbnails) finish in Q0 almost immediately. Long jobs (films) sink to the bottom and run in long efficient slices whenever the top queues are empty.** Periodically boosting every job back to Q0 (a form of ageing) prevents starvation.

## Video-service mix (2 films × 1,200 s and 8 thumbnails × 2 s, arriving 0–9 s)

| Policy | Avg WT (all) | Thumbnails avg WT | Films avg WT |
|---|---|---|---|
| FCFS | 1801.5 | **2101.8** | 600.5 |
| SJF | 1082.7 | 1201.8 | 606.5 |
| SRTF | 125.1 | **2.8** | 614.5 |
| RR q=3 | 250.0 | 9.3 | 1213.0 |
| RR q=20 | 270.3 | 36.8 | 1204.5 |
| **MLFQ (2, 4, FCFS)** | **127.7** | **5.3** | **617.5** |

## Design decision for the video service

**Recommend MLFQ** (or, equivalently, two queues: a short-job queue and a batch queue for films).

- Its thumbnail wait is **5.3 s**, against 2,101.8 s under FCFS: 400× better, and close to the theoretical best (SRTF, 2.8 s).
- Its film wait (617.5 s) is the same as SRTF/SJF and half that of RR.
- Unlike SRTF it **does not need to know the burst length in advance**. The encoder cannot know a job's duration exactly; MLFQ learns it from behaviour.
- With periodic priority boosting it has no starvation, while the CPUs stay 100 % busy because films soak up all idle time.

## Alternative concept: MLFQ vs a single RR queue

| | Single RR queue | MLFQ (3 levels) |
|---|---|---|
| Short-job avg WT (video mix) | 9.3 s (q=3) – 36.8 s (q=20) | **5.3 s** |
| Long-job avg WT | ≈ 1,205–1,213 s | **617.5 s** (fewer switches, runs in long slices) |
| Scheduling cost per decision | O(1): dequeue the head | O(1) per queue: scan k levels for the first non-empty one = O(k), k = 3. SJF/SRTF need O(n) to find the shortest job (or O(log n) with a heap). |
| Space | One queue, O(n) | k queues, O(n + k) |
| Starvation | None (every job gets a turn each round) | Possible for the bottom queue under endless short jobs, so a **priority boost every S seconds** is required |

Pseudocode:

```
on arrival(j):          j.level = 0; enqueue(Q[0], j)
pick():                 for L in 0..2: if Q[L] not empty: return dequeue(Q[L])
after running j:        if j finished: done
                        else if j used its full quantum[L] and L < 2: j.level++ ; enqueue(Q[L+1], j)
                        else: enqueue(Q[L], j)
every S seconds:        move every job back to Q[0]     # ageing / boost
```

<!-- pagebreak -->

# Q3 – Threads (6 marks) – Traffic-Camera Number-Plate Reader

## What threads share and what each has of its own

| Shared by all threads of a process | Private to each thread |
|---|---|
| Code (text) segment | Thread ID |
| Global and static data (e.g. the plate results table, counters) | Program counter |
| Heap (frames allocated with `malloc`) | Register set |
| Open files, sockets (camera streams) | **Stack** (local variables, call frames) |
| Signal handlers, current directory, user ID | Signal mask, `errno`, thread-local storage |
| Address space / page tables | Scheduling state and priority |

## User-level vs kernel-level threads and the models

- **User-level threads** are managed by a library in user space. Switching is very fast and needs no system call, but the kernel sees one process: **one blocking call blocks all threads**, and they cannot run on several cores.
- **Kernel-level threads** are known to and scheduled by the OS. They run truly in parallel on multiple cores, and a blocking call blocks only that thread. Creating and switching them costs more (a system call each).
- **Many-to-one**: many user threads map to one kernel thread (old Green Threads, GNU Portable Threads). No parallelism.
- **One-to-one**: each user thread is a kernel thread. **Linux NPTL, Windows and macOS pthreads use this**, so it is what this program uses. Full parallelism, but there is a per-thread kernel cost.
- **Many-to-many**: M user threads are multiplexed onto N kernel threads (Solaris before 9, Go goroutines, Java virtual threads). Cheap threads plus parallelism, but a more complex scheduler.

## Measured speed-up (`Q3/plates.c`, 64 frames of 640×480, edge filter + histogram, 10-core M4)

| Method | Time (ms) | Speed-up | Same result |
|---|---|---|---|
| Sequential | 61.3 | 1.00 | yes |
| One thread per camera (64) | 8.1 | 7.55 | yes |
| Pool of 2 threads | 23.0 | 2.66 | yes |
| Pool of 4 threads | 12.6 | 4.85 | yes |
| Pool of 8 threads | 8.8 | 6.97 | yes |
| Pool of 16 threads | 8.0 | 7.69 | yes |

**Why the speed-up stops increasing:**

1. **Core count.** There are only 10 cores, and only 4 of them are fast (P-cores). Beyond 8–10 threads, extra threads just queue for cores.
2. **Amdahl's law.** The sequential part (creating and joining threads, setting up frames) limits speed-up to 1/(s + (1−s)/N).
3. **Shared memory bandwidth and cache.** 64 × 300 KB frames compete for the same memory system.
4. **Overhead.** 64 threads cost creation and scheduling time but give no more parallelism than 16. They tie (8.1 ms vs 8.0 ms) because each thread does a large chunk of work.

The pool is better than one thread per camera because it gives the same speed with 16 instead of 64 threads, and it scales to hundreds of cameras without hundreds of stacks.

## Shared counter without protection

```
shared counter, 2 threads x 1000000 increments (expected 2000000):
  run 1 unprotected : 1000000  (lost 1000000 updates)
  run 2 unprotected : 1009992  (lost 990008 updates)
  run 3 unprotected : 1014698  (lost 985302 updates)
  mutex-protected   : 2000000
  atomic_fetch_add  : 2000000
```

`counter++` is really **load, add, store**. When both threads load the same value, both store value + 1, and one increment is **lost** (a race condition). Here almost half of all updates were lost, and the result changes from run to run. A mutex or an atomic increment makes the update indivisible.

## Alternative concept: processes instead of threads (`plates_proc.c`)

| | Threads | Processes |
|---|---|---|
| Time, pool of 4 / 8 / 16 | 12.6 / 8.8 / 8.0 ms | 20.9 / 13.2 / 11.6 ms |
| Creation | ≈ 4–12 µs each | ≈ 100–400 µs each (fork copies page tables), O(size of parent) |
| Switching | Same address space (measured 1.08 µs, Q1) | Address-space switch (1.59 µs, Q1) |
| Memory | One copy of the 64 frames (19.7 MB) shared by all | Frames shared copy-on-write after fork, but each process has its own page tables and stack; frames written after fork are duplicated |
| Combining results | Write directly into the shared `plate[]` array | **IPC**: each worker `write()`s (camera, plate) to a pipe and the parent `read()`s them, so every result is copied twice |
| Fault isolation | One crash kills all cameras | One crash loses only that worker's cameras |

Processes were 1.4–1.7× slower here, mostly because of fork cost and result IPC. They are worth it only if isolation of untrusted or crash-prone decoding code matters more than speed.

<!-- pagebreak -->

# Q4 – Inter-Process Communication (6 marks) – Weather Station Data Pipeline

## Complete the Code: TODO 1 of `pipe_ipc.c`

```
while (read(fd[0], &r, sizeof r) == (ssize_t)sizeof r) {
    count++;
    sum += r;
    if (r > max) max = r;
}
```

Output (identical to the expected output):

```
analyser: 6 readings, average 35.5, max 41
reader: done
```

(31 + 33 + 36 + 34 + 41 + 38 = 213, and 213 / 6 = 35.5.)

## How the pipe signals end of data

`read()` on a pipe returns **0 (end of file) only when the pipe is empty AND every write end is closed**. The parent's `close(fd[1])` is the end-of-data signal. Until then, an empty pipe makes `read()` **block**, waiting for more data.

## The experiment: remove `close(fd[1])` in the parent (`pipe_noclose.c`)

```
reader: all 6 readings sent, now waiting for the analyser...
analyser: got reading 31, waiting for more...
  ... 33, 36, 34, 41, 38 ...
[killed by 3 s alarm, exit status 142] - the program never finished
```

The child reads all six readings, then blocks in `read()`. The pipe is empty, but a write end is still open (the parent's), so the kernel cannot report EOF. More data could still arrive. Meanwhile the parent blocks in `waitpid()` waiting for the child. **Each waits for the other forever**: a deadlock caused by a missing close. This is also why the child must close its own copy of `fd[1]`. If it did not, it would hold a write end itself and never see EOF either.

## Shared-memory vs message-passing models

| | Message passing (pipe, message queue, socket) | Shared memory (`mmap`/`shmget`) |
|---|---|---|
| How | The kernel carries each message: `write()` → kernel buffer → `read()` | Both processes map the same physical pages; data is just read and written in memory |
| Copies | **2 copies per byte** (user → kernel, kernel → user) plus a system call per transfer | **0 copies through the kernel.** The producer writes directly into the shared area. |
| Synchronisation | **Built in**: `read()` blocks when empty, `write()` blocks when full | **Extra synchronisation is needed** (semaphores / mutex), otherwise the reader may read a half-written slot or the writer may overwrite unread data |
| Works across machines | Yes (sockets) | No, same machine only |
| Best for | Small messages, simplicity | Large volumes, high rates |

## Two implementations and growth investigation (`ipc_bench.c`, best of 3)

The pipe uses 64 KB writes. Shared memory uses a ring of 8 × 64 KB slots with two POSIX semaphores (`empty` = free slots, `full` = filled slots). The analyser sums every reading on both paths, and the sums matched in every run.

| Data | Pipe (ms) | Shared memory + semaphores (ms) | Pipe / shm |
|---|---|---|---|
| 1 MB | 1.47 | 1.22 | 1.21× |
| 10 MB | 3.00 | 2.36 | 1.27× |
| 100 MB | 17.93 | 9.32 | **1.92×** |

Evaluation:

- Both grow **linearly, O(n)**, once start-up cost (fork, semaphore creation) is amortised. At 1 MB the fixed costs dominate, so the two are close.
- The pipe pays **2 kernel copies + 2 system calls per 64 KB**, so its slope is steeper. At 100 MB shared memory is ≈ **2× faster**, and the gap widens with size.
- Shared memory pays only **2 semaphore operations per 64 KB slot**, O(n / slot size) synchronisation, and no data copying through the kernel.

## Alternative concept: shared memory with a semaphore instead of a pipe

```
shared: int ring[SLOTS][K]; sem empty = SLOTS, full = 0
sensor reader:                         analyser:
  for each block c:                      for each block c:
    wait(empty)                            wait(full)
    write readings into ring[c%SLOTS]      read readings from ring[c%SLOTS]
    signal(full)                           signal(empty)
```

| | Pipe | Shared memory + semaphore |
|---|---|---|
| Time for n readings | O(n) with 2 copies per reading through the kernel + O(n/b) system calls | O(n) with **0 kernel copies** + O(n/K) semaphore operations |
| Space | Kernel pipe buffer (64 KB on Linux) + user buffers on both sides | Shared ring SLOTS × K, allocated once |
| Synchronisation | Implicit: the kernel blocks reader and writer | **Explicit**: two counting semaphores (and a mutex if there are several producers or consumers) |
| Failure modes | Missing `close` → reader never sees EOF (shown above) | Missing `signal` → deadlock; missing `wait` → lost or overwritten readings |

**Justified choice for the weather station:** a **pipe** at the station's real data rate. That is a few readings per second, far below the 1 MB/1.5 ms that a pipe already handles. It needs no hand-written synchronisation and cannot lose readings. **Shared memory** would be chosen only if a high-rate sensor (radar, lidar, ≥ 100 MB/s) is added, where the measured 2× gain matters.

<!-- pagebreak -->

# Q5 – Synchronisation: Critical Sections, Semaphores and Monitors (11 marks) – Railway Ticket Booking

## From the principle to a design

The principle requires three things:

1. **Mutual exclusion**: at most one agent is inside the critical section for a seat at a time.
2. **Progress**: if no one is inside, a waiting agent must be allowed in, and the choice cannot be postponed forever by agents who are not trying.
3. **Bounded waiting**: there is a limit on how many times others can enter before a waiting agent gets its turn.

The design that follows: every shared item (seat counter, coach seat map) gets a lock. Every booking does `entry → check seat free → mark sold → exit` **inside** that lock. Lock granularity is chosen to minimise contention (decided below).

## Peterson's solution (two processes)

```
flag[i] = true;  turn = j;              // entry section
while (flag[j] && turn == j) ;          // wait
   critical section
flag[i] = false;                        // exit section
```

- **Mutual exclusion:** if both are inside, then flag[0] = flag[1] = true, so each must have seen turn equal to itself. But `turn` holds one value, and the last writer gave it to the other process. Contradiction.
- **Progress:** if Pj does not want to enter, flag[j] = false and Pi enters immediately.
- **Bounded waiting:** Pi waits at most one entry of Pj. Pj sets turn = i before re-entering, so Pi gets in next.

**Why it may fail on modern processors:** CPUs and compilers **reorder memory operations**. A store buffer lets a thread's `flag[i] = true` sit unseen while it already reads `flag[j]`. Both threads then read false and both enter. Measured on the ARM M4 (`peterson.c`, 2 threads × 2,000,000 entries):

```
plain Peterson, run 1 : 3999994  (6 lost -> mutual exclusion VIOLATED)
plain Peterson, run 2 : 4000000  (0 lost -> mutual exclusion held)
with seq_cst fence, run 1..3: 4000000  (0 lost -> mutual exclusion held)
```

The failure is rare (6 in 4 million), which makes it dangerous: it passes most tests. A **memory fence** (`atomic_thread_fence(seq_cst)`) or hardware atomic instructions (test-and-set, compare-and-swap) are needed. In practice we use OS locks built on those instructions.

## Counting and binary semaphores for the seat counter

A semaphore S is an integer with two atomic operations. `wait(S)` waits while S ≤ 0, then decrements S. `signal(S)` increments S and wakes a waiter.

- **Counting semaphore** `seats = 72` per coach. Each booking does `wait(seats)`, so the 73rd agent blocks instead of overselling. A cancellation does `signal(seats)`.
- **Binary semaphore / mutex** `m = 1`. It protects *which* seat is taken: `wait(m); find free seat; mark sold; signal(m)`. This prevents two agents picking the same seat number.

## Classic problems with semaphores

**Bounded buffer (producer–consumer):** `empty = N`, `full = 0`, `mutex = 1`.

```
producer: wait(empty); wait(mutex); add item; signal(mutex); signal(full)
consumer: wait(full);  wait(mutex); remove item; signal(mutex); signal(empty)
```

**Readers–writers** (many agents reading availability, few writing bookings):

```
reader: wait(mutex); readcount++; if (readcount==1) wait(rw); signal(mutex)
        ... read seat map ...
        wait(mutex); readcount--; if (readcount==0) signal(rw); signal(mutex)
writer: wait(rw); ... book seat ...; signal(rw)
```

This version favours readers, so writers can starve. A writer-preference version or an OS rwlock fixes that.

**Dining philosophers** (5 forks = 5 semaphores): the naive `wait(left); wait(right)` deadlocks when all take their left fork. The solutions are:

- allow at most 4 philosophers to sit (`seats = 4` counting semaphore), or
- use asymmetric order (odd philosophers take the right fork first), or
- pick up both forks only if both are free, inside a monitor.

## How a monitor makes it easier to get right

A **monitor** is an abstract data type whose procedures run with **implicit mutual exclusion**: the compiler or runtime inserts the lock. Waiting is done with **condition variables** (`wait(c)` releases the monitor lock and sleeps; `signal(c)` wakes one waiter). The programmer cannot forget an unlock, swap the order of `wait(empty)` and `wait(mutex)` (a classic semaphore deadlock), or touch the data outside the lock. Java `synchronized` and pthread mutex + condition variable are monitors. The macOS semaphore shim in this repo (`Q5/macos_sem.h`) is itself a tiny monitor: a mutex, a condition variable and a `while (value == 0) wait` loop.

```
monitor Coach {
    int free = 72; condition seatFreed;
    book()   { while (free == 0) seatFreed.wait(); free--; ...mark seat... }
    cancel() { free++; seatFreed.signal(); }
}
```

## Complete the Code: `bb.c`

```
// TODO 1 (producer)                         // TODO 2 (consumer)
sem_wait(&empty);                             sem_wait(&full);
pthread_mutex_lock(&lock);                    pthread_mutex_lock(&lock);
buffer[in] = item;                            int item = buffer[out];
in = (in + 1) % SIZE;                         out = (out + 1) % SIZE;
pthread_mutex_unlock(&lock);                  pthread_mutex_unlock(&lock);
sem_post(&full);                              sem_post(&empty);
                                              consumed_sum += item;
```

Output: `items consumed: 20, sum = 210 (expected 210)`, which is identical to the expected output.

**100 runs:** `for i in $(seq 100); do ./bb; done | sort | uniq -c` gives
`100 items consumed: 20, sum = 210 (expected 210)`. All 100 runs were correct.

## Test the design: 1 and 4 producers/consumers (`bb_multi.c`, each item has a unique id)

| Configuration | Items | Consumed exactly once | Missing | Duplicates | Result |
|---|---|---|---|---|---|
| 1 producer, 1 consumer, mutex ON | 20,000 | 20,000 | 0 | 0 | CORRECT |
| 4 producers, 4 consumers, mutex ON | 80,000 | 80,000 | 0 | 0 | CORRECT |
| 4 producers, 4 consumers, **mutex OFF** (run 1) | 80,000 | 57,790 | 11,109 | 11,109 | **WRONG** |
| 4 producers, 4 consumers, **mutex OFF** (run 2) | 80,000 | 57,418 | 11,294 | 11,294 | **WRONG** |

**Removing the mutex:** with one producer and one consumer, the semaphores alone are enough. `in` is touched only by the producer and `out` only by the consumer, and the semaphores keep them apart. With **several** producers, two of them can read the same `in`, write into the same slot and both advance `in`. One item is overwritten (missing) and an empty or stale slot is consumed (a duplicate). Several consumers can likewise take the same slot. The semaphores count slots correctly but do not protect the **indices**. That needs the mutex, which is the critical section.

## Deployment decision: lock granularity (`booking.c`, 8 agents, 10 trains × 20 coaches × 72 seats)

| Locking | Time (ms) | Bookings/s | Double-sold |
|---|---|---|---|
| One global lock | 51.2 | 6.3 million | 0 |
| One lock per train | 17.9 | 17.9 million | 0 |
| **One lock per coach** | **5.2** | **61.4 million** | **0** |
| No lock (reference only) | 1.9 | 165.9 million | 0 seen in this run, but **not guaranteed** (Q3 lost ~50 % of updates without a lock) |

**Decision: one lock per coach.** It gives **9.8× the throughput of a global lock** and 3.4× that of per-train locks, with the same correctness. With 200 independent locks, two agents rarely want the same coach at the same moment, so they rarely wait. A booking never needs two coaches at once, so there is no lock-ordering or deadlock risk. If a booking does span several coaches (a group booking), take the coach locks in a fixed order (train, then coach number) to prevent deadlock (Q6). Locks per **seat** would add memory (14,400 locks) for little extra gain, because the critical section (finding a free seat in a coach) needs the whole coach anyway.

## Alternative concept: atomics or spin lock instead of semaphore and mutex (`counter_bench.c`)

**Part 1. Short critical section (`counter++`), 2,000,000 updates:**

| Method | 1 thread (ns/update) | 4 threads | 16 threads | CPU / wall at 16 threads |
|---|---|---|---|---|
| Mutex | 7.6 | 114.7 | 14.7 | 9.65 |
| Semaphore | 8.0 | 35.5 | 2,412.7 | 4.92 |
| **atomic_fetch_add** | **1.6** | **6.7** | **45.6** | 9.67 |
| Spin lock | 1.0 | 31.4 | 606.9 | **9.83** (all cores busy spinning) |

**Part 2. Long critical section (20 µs), 16 threads, 3,200 updates:**

| Method | Wall (ms) | CPU time (ms) | CPU wasted (ms) |
|---|---|---|---|
| **Mutex** (sleeps when busy) | **66.4** | **71.1** | **7.1** |
| Spin lock | 104.3 | 914.8 | **850.8** |

Evaluation:

- **Atomic operations** are a single hardware instruction (LDADD/CAS), O(1), with no lock and no system call. They are the fastest for a single counter, but they only work when the critical section is one variable (e.g. "seats left").
- **Spin locks** are fastest uncontended (1 ns) and fine on multicore when the critical section is a few instructions. Under contention every waiter burns a whole core. With a 20 µs section they wasted **851 ms of CPU to do 64 ms of work** (12×) and were slower in wall time too.
- **Mutex / semaphore** block and sleep, so they waste almost no CPU on long sections. Each contended operation costs a system call and a context switch (the semaphore at 16 threads costs 2.4 µs per update, because the macOS shim is a condition-variable monitor).
- **For a long critical section** (a seat booking that checks quotas and writes the database), use a **blocking mutex / monitor**. Use atomics for simple counters, and spin locks only for very short sections inside the kernel or on dedicated cores.

<!-- pagebreak -->

# Q6 – Deadlock Handling and the Banker's Algorithm (13 marks) – Shared Laboratory Equipment Scheduler

## A. Characterise deadlock (2 marks)

The four necessary conditions, which must **all** hold at once:

1. **Mutual exclusion**: a GPU or a licence seat can be used by only one job at a time.
2. **Hold and wait**: a job holds the GPU it already has while waiting for a licence.
3. **No preemption**: the scheduler cannot take a GPU away from a running job without losing its work.
4. **Circular wait**: Job A waits for B's resource, B waits for C's, …, and the last waits for A's.

**Resource-allocation graph for the cluster.** Circles are jobs and boxes are resources (dots = instances). An arrow from a resource to a job means *allocated*; an arrow from a job to a resource means *requested*.

```
             holds                    requests
   [GPU  ●] ---------> ( Job A ) ------------------> [LICENCE ●]
       ^                                                  |
       | requests                                         | holds
       |                                                  v
   ( Job C ) <------- [STORAGE ●] <-------------------- ( Job B )
              holds                     requests

   Cycle:  A -> LICENCE -> B -> STORAGE -> C -> GPU -> A
```

- **Mutual exclusion:** each box has a single instance (●).
- **Hold and wait:** every job holds one resource (incoming arrow) and requests another (outgoing arrow).
- **No preemption:** no arrow can be removed except by the holding job finishing.
- **Circular wait:** the cycle A → LICENCE → B → STORAGE → C → GPU → A. With **one instance per resource, a cycle means deadlock.** With several instances a cycle is necessary but not sufficient.

**Real run** (`deadlock_demo.c`, two jobs, GPU and LICENCE taken in opposite orders):

```
  job A: holds GPU, requests LICENCE
  job B: holds LICENCE, requests GPU
  DETECTOR: no progress for 2 s - A waits for LICENCE held by B,
            B waits for GPU held by A -> cycle -> DEADLOCK. Aborting job B.
  job B: ABORTED by recovery, released LICENCE (its 0.1 s of work is lost)
  job A: holds GPU + LICENCE -> running simulation
  job A: finished                                total time 2.07 s
== prevention: every job locks GPU before LICENCE ==
  job A: ... finished   job B: ... finished      total time 0.32 s
```

## Complete the Code: TODO 1 of `banker.c`

```
int ok = 1;
for (int r = 0; r < R; r++)
    if (maxm[p][r] - alloc[p][r] > work[r]) { ok = 0; break; }
if (ok) {
    for (int r = 0; r < R; r++) work[r] += alloc[p][r];
    finish[p] = 1;
    seq[count++] = p;
    found = 1;
}
```

Output (identical to the expected output):

```
initial state safe: P1 P3 P4 P0 P2
P1 request: granted, safe sequence P1 P3 P4 P0 P2
P4 request: must wait (not available)
P0 request: denied (unsafe)
```

## By hand: why P0's request (0, 2, 0) is unsafe

State **after** P1's request (1, 0, 2) was granted: Available = (2, 3, 0). Need = Max − Allocation:

| Process | Allocation | Max | Need |
|---|---|---|---|
| P0 | 0 1 0 | 7 5 3 | 7 4 3 |
| P1 | 3 0 2 | 3 2 2 | 0 2 0 |
| P2 | 3 0 2 | 9 0 2 | 6 0 0 |
| P3 | 2 1 1 | 2 2 2 | 0 1 1 |
| P4 | 0 0 2 | 4 3 3 | 4 3 1 |

**Request checks:** Request (0, 2, 0) ≤ Need P0 (7, 4, 3) ✔, and Request ≤ Available (2, 3, 0) ✔. Pretend to grant it:

- Available = (2, 3, 0) − (0, 2, 0) = **(2, 1, 0)**
- Allocation P0 = (0, 3, 0)
- Need P0 = (7, 2, 3)

**Safety check with Work = (2, 1, 0):**

| Process | Need | Need ≤ (2, 1, 0)? |
|---|---|---|
| P0 | 7 2 3 | no (7 > 2) |
| P1 | 0 2 0 | no (2 > 1 for resource B) |
| P2 | 6 0 0 | no (6 > 2) |
| P3 | 0 1 1 | no (1 > 0 for resource C) |
| P4 | 4 3 1 | no |

No process can finish, so no safe sequence exists and the state is **unsafe**. The request is **denied** and the state is rolled back to Available = (2, 3, 0). Granting it would not cause deadlock immediately, but it would allow one if every job then asked for its maximum.

(For comparison, P4's request (3, 3, 0) fails the first check, 3 > Available A = 2, so it **must wait**: the resources simply are not there.)

## Alternative concept: detection and recovery instead of Banker's algorithm

| | Banker's (avoidance) | Detection + recovery |
|---|---|---|
| Work per request | Safety algorithm on **every** request: O(m × n²) | **None**: grant if available, O(m) |
| Periodic cost | – | Detection algorithm (same structure as the safety algorithm, using Request instead of Need) every T minutes: O(m × n²) per run, amortised over all requests in T |
| Needs maximum demands in advance | **Yes** | No |
| Deadlocks | Never occur | Occur, then are found within T |
| Work lost | None | When a victim job is aborted, everything it computed since its last checkpoint. In the demo, job B lost its 0.1 s; on the cluster a GPU job aborted after 6 h with hourly checkpoints loses up to 1 h × its GPUs |
| Utilisation | Lower: it refuses safe-looking requests (P0 above) and keeps resources idle | Higher: it grants every available request |

Using the measurements in E: with n = 1,000 jobs, a Banker's check costs up to **0.78 ms per request**. At 10 requests/s that is ≈ 0.8 % of a core. A detection pass every 5 minutes costs 0.78 ms per 5 minutes, which is negligible. The real cost of detection is the **lost work** of aborted jobs.

## B. Compare the four strategies for the cluster (3 marks)

| Strategy | How, on the cluster | Cost | Suitability |
|---|---|---|---|
| **Prevention** | Break one condition. **Circular wait:** number the resource types (storage < licence < GPU) and always request in that order. **Hold and wait:** request every resource at job submission (all-or-nothing). Mutual exclusion cannot be removed for a GPU; preemption is possible for storage bandwidth but not for a running GPU job. | Ordering costs nothing at run time (demo: 0.32 s vs 2.07 s). All-at-once allocation leaves GPUs idle while a job does its CPU pre-processing. | **Good**: the batch scheduler controls the order of acquisition |
| **Avoidance (Banker's)** | Jobs declare max GPUs, licences and storage. Every request is checked with the safety algorithm. | O(m n²) per request (0.78 ms at n = 1,000). Needs exact maximum demands, and is conservative, so resources sit idle. | Possible, because cluster jobs *do* declare resources (e.g. `--gres=gpu:4`), but over-declaration wastes capacity |
| **Detection + recovery** | Grant freely, run detection every few minutes, abort or roll back a victim (fewest checkpointed hours lost). | Detection is cheap; recovery loses work. | Acceptable if jobs checkpoint |
| **Ignore (ostrich)** | Do nothing and let the administrator reboot. | Free until it happens, then the **whole cluster stops** (the current problem). | **Rejected**: deadlocks are frequent here, not rare |

## C. Apply the Banker's algorithm (4 marks)

The implementation is `Q6/banker.c`: the safety algorithm in `is_safe()` plus the resource-request algorithm in `request()`.

- **Initial state:** safe sequence **P1 → P3 → P4 → P0 → P2**. Work goes (3,3,2) →P1→ (5,3,2) →P3→ (7,4,3) →P4→ (7,4,5) →P0→ (7,5,5) →P2→ (10,5,7).
- **P1 requests (1,0,2):** ≤ Need (1,2,2) and ≤ Available (3,3,2). The new state is safe (P1 P3 P4 P0 P2), so it is **granted**.
- **P4 requests (3,3,0):** 3 > Available A = 2, so it **must wait**.
- **P0 requests (0,2,0):** the state would be unsafe (hand proof above), so it is **denied**.

## D. Predict before implementing (1 mark)

The safety algorithm makes up to n passes. Each pass scans up to n unfinished processes and compares m resources. **Worst case T(n, m) = O(m × n²)**: in each pass only one process can finish, and it is the last one scanned. Best case: everyone can finish in the first pass, so **O(m × n)**.

**Testable prediction:** with m fixed, 10× more processes means about **100× more time** in the worst case and about 10× in the best case. With n fixed, going from m = 3 to m = 10 changes the worst case by **at most 3.3×**, and less if the inner loop exits early.

## E. Measure (1 mark) – `banker_bench.c`

| n | m | Case | Time (µs) | Need comparisons |
|---|---|---|---|---|
| 10 | 3 | worst | 0.27 | 75 |
| 100 | 3 | worst | 11.28 | 5,250 |
| 1,000 | 3 | worst | **777.60** | 502,500 |
| 2,000 | 3 | worst | 2,686.00 | 2,005,000 |
| 10 | 3 | best | 0.12 | 30 |
| 100 | 3 | best | 0.47 | 300 |
| 1,000 | 3 | best | 3.40 | 3,000 |
| 1,000 | 10 | worst | 579.00 | 509,500 |
| 1,000 | 10 | best | 5.60 | 10,000 |

**Comparison with the prediction:**

- **Worst case, 100 → 1,000 processes:** comparisons ×95.7 (5,250 → 502,500, close to n²/2 + …) and time ×69. From 1,000 → 2,000: comparisons ×3.99 and time ×3.45. Both match **O(n²)**. Time grows slightly slower than the count because small n is dominated by fixed overhead (malloc of work/finish).
- **Best case:** comparisons are exactly n × m (×10 per 10× n) and time ×7–8. This matches **O(m n)**.
- **m = 3 → 10:** worst-case time did *not* grow (777 → 579 µs). The inner loop stops at the **first** resource that fails the test, so the effective work is O(n²) comparisons plus O(m) only for the n processes that succeed. The prediction "at most 3.3×" holds, but in practice the cost is nearly independent of m.

## F. Defend the final design (2 marks)

**Recommendation: prevention by resource ordering as the base rule, plus detection and recovery as a safety net. The Banker's algorithm is not used for the whole cluster.**

1. **Resource ordering** (always storage → licence → GPU, enforced by the job scheduler's allocation API) removes circular wait for free. Measured: 0.32 s with ordering vs a 2 s hang plus an aborted job without it.
2. **The Banker's algorithm is affordable but impractical here.** It costs 0.78 ms per request at 1,000 jobs, which is affordable. But it needs every job's **maximum demand in advance**, and research jobs routinely over-declare, so the algorithm keeps GPUs idle to stay "safe" (it denied P0 even though deadlock was not certain). Under-declaration breaks its guarantee entirely. Its O(m n²) cost also becomes 2.7 ms per request at 2,000 jobs, and it would be paid on *every* request.
3. **Detection** every 5 minutes (O(m n²) ≈ 1 ms per run) catches any deadlock the ordering misses, for example from software licences acquired inside user code. Recovery aborts the job with the **least work since its last checkpoint**. Mandatory hourly checkpoints bound the lost work to at most 1 hour.
4. **The ostrich approach is rejected**, because the problem statement says the cluster already stops regularly.

This keeps utilisation high (no idle reserved GPUs), keeps per-request cost at O(m), and turns "the whole cluster stops until an administrator intervenes" into "at most one job restarts from a checkpoint".
