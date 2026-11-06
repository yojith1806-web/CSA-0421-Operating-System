# TITLE: Operating Systems – Assignment 1 (CO1)

::center **Evaluate and Justify the Operating-System Design of a Smart Hospital**

::center Course: CSA04 – Operating Systems   |   Total Marks: 50

::center **Name:** [Student Name]     **Register Number:** [Register Number]

::center **GitHub Repository:** https://github.com/Nithish250707/CSA04-[Register Number]-Assignment1

::center Test machine: Apple M4 (4P + 6E cores), 16 GB RAM, macOS 27.0.1 (XNU, POSIX), Apple clang 21 (`gcc -std=c11 -Wall -Wextra -O2 -pthread`)

<!-- pagebreak -->

# Task 1 – Requirements of Each Computing Environment (4 marks)

The hospital has six computing environments. For each, the requirements are ranked 1 (most important) to 5. The ranking rule is: **anything whose failure can injure a patient ranks above anything that only costs time or money.**

## Ranked requirement tables

**Bedside patient monitors (500)**

| Rank | Requirement | Target | Justification |
|---|---|---|---|
| 1 | Response time (deadline) | alarm ≤ 10 ms, guaranteed | A late alarm on cardiac arrest or low SpO2 is a direct patient-safety failure; the deadline is a hard constraint, not an average. |
| 2 | Reliability | runs for years, watchdog restart < 1 s | A crashed monitor is the same as no monitor. |
| 3 | Security | signed firmware, no user code | A tampered monitor could suppress alarms. |
| 4 | Throughput | ~500 samples/s per sensor | Small and constant, so easy to meet. |
| 5 | Battery | mains powered, backup battery only | Only matters during transport. |

**Nurses' handheld tablets (120)**

| Rank | Requirement | Target | Justification |
|---|---|---|---|
| 1 | Security | encryption, remote wipe | Tablets get lost and they carry patient data. |
| 2 | Battery | ≥ 12 h shift | A dead tablet means no alarm notifications for that nurse. |
| 3 | Response time (interactive) | < 200 ms UI response | This is a soft deadline: a slow screen is annoying but not dangerous. |
| 4 | Reliability | app crash must not lose data | Data is stored on the server, so the tablet can be replaced. |
| 5 | Throughput | low | Only small records and notifications. |

**Doctors' desktop workstations (80)**

| Rank | Requirement | Justification |
|---|---|---|
| 1 | Security (login, role-based access) | They give full access to records and orders. |
| 2 | Response time (interactive) | Doctors' time is expensive and the response time is a soft target. |
| 3 | Reliability | Another workstation can be used if one fails. |
| 4 | Throughput (image viewing) | Needed for viewing MRI images. |
| 5 | Battery | Not applicable (mains powered). |

**Central patient-records database server**

| Rank | Requirement | Justification |
|---|---|---|
| 1 | Reliability / availability ("must never stop") | Without records, wrong drugs or doses can be given, so it is safety-critical. |
| 2 | Security and integrity | It holds the legal medical record and is protected by law. |
| 3 | Throughput (transactions/s) | 700+ devices query it at the same time. |
| 4 | Response time (< 1 s per query) | A soft deadline. |
| 5 | Battery | Not applicable (UPS only). |

**MRI imaging server**

| Rank | Requirement | Justification |
|---|---|---|
| 1 | Throughput (parallel processing) | A 500 MB scan must be reconstructed in minutes. |
| 2 | Reliability | A job can be re-run, so it is not safety-critical in real time. |
| 3 | Security | It holds patient images. |
| 4 | Response time | Batch-like work: minutes are acceptable. |
| 5 | Battery | Not applicable. |

**Link to two partner hospitals**

| Rank | Requirement | Justification |
|---|---|---|
| 1 | Security (encryption, authentication, audit) | Data crosses the public internet. |
| 2 | Reliability / consistency | Both sites must see the same allergy list. |
| 3 | Throughput | Transfers of records and images. |
| 4 | Response time | Seconds are acceptable. |

## Conflicting requirements

- **Fast response vs many users on one machine.** Time-sharing gives every user a fair share of the CPU, so a 10 ms alarm task competes with everything else. This is why the monitors must not share an OS with general workloads.
- **Battery vs response time (tablets).** Deep sleep saves power but adds wake-up latency. Interrupt or push wake-up resolves this better than polling (Task 8).
- **Availability vs updates (database).** Patching normally needs a reboot. A cluster with rolling updates resolves this (Task 2).
- **Security vs response time.** Encryption and checks add cost to every request. This is acceptable everywhere except inside the monitor's alarm path.
- **Throughput vs response time (imaging).** Batch scheduling maximises throughput and is right for MRI reconstruction, but it would be wrong for interactive or alarm work.

## Safety-critical requirements

Three requirements are safety-critical: the monitor alarm deadline, monitor reliability, and database availability and integrity. Everything else is about cost, comfort or convenience.

## Lessons from OS history

| Era | Lesson | Where it applies |
|---|---|---|
| Batch (1950s–60s) | Maximise throughput with no interaction. | The MRI reconstruction queue on the imaging server. |
| Time-sharing (1960s–70s) | Many users get a fair CPU share and short quanta, but no deadline guarantee. | The database server and workstations. Never the monitors. |
| Personal computing (1980s) | One user gets a rich GUI, and convenience comes before protection. Early PC OSes had no memory protection. | Workstations, but with modern protection added. |
| Mobile (2000s–) | Power management, sandboxed apps, wireless networking. | Nurses' tablets. |
| Real-time / embedded | Predictability matters more than average speed. | Bedside monitors. |

## Alternative concept: one general-purpose OS everywhere

**Measured evidence** (`Task01/alarm_latency.c`, a 1 ms periodic task on a general-purpose time-sharing OS, 5,000 wake-ups):

| Condition | avg | 99th pct | 99.9th pct | **worst** | wake-ups late > 1 ms |
|---|---|---|---|---|---|
| idle system | 214 µs | 244 µs | 3,156 µs | **7,155 µs** | 8 |
| 20 CPU-hog processes | 206 µs | 256 µs | 1,793 µs | **2,246 µs** | 18 |

- **Worst-case alarm response.** A general-purpose OS gives no bound. In 5,000 samples the worst wake-up was already 7.2 ms late on an idle machine, using 72 % of the 10 ms budget before any alarm work had started. The tail is set by other activity (disk flushes, interrupts, the GUI, updates), so worst case = **unbounded** (formally O(number of runnable tasks × quantum) plus non-preemptible kernel sections). A hard RTOS with fixed-priority preemption has worst-case latency = interrupt latency + context switch + the highest-priority task's own run time. That is **bounded and analysable**, typically 1–20 µs on a microcontroller (QNX and FreeRTOS publish such bounds).
- **Memory per device.** A general-purpose OS needs about 1–2 GB of RAM and 8+ GB of storage (Ubuntu server boots in ~300–500 MB of RAM). FreeRTOS needs about 6–12 KB of ROM and a few KB of RAM; QNX needs about 1–2 MB. For 500 monitors that is roughly 500 × 1 GB = 500 GB of RAM against about 1 GB. **Space O(n × large) vs O(n × tiny).**
- **Administration effort.** One OS is simpler to administer: one patch process and one skill set, O(1) platforms. However, every one of the 700+ devices then runs a large attack surface that needs monthly patches, so patching work becomes O(devices × patches). The multi-OS plan has about 4 platforms. The monitors' RTOS is rarely patched because it has almost no services.
- **Verdict.** One general-purpose OS is **rejected for the monitors** (no deadline guarantee) and accepted for workstations and servers.

<!-- pagebreak -->

# Task 2 – Type of Operating System for Each Environment (3 marks)

## Types compared

| Type | Key property | Strength | Weakness |
|---|---|---|---|
| Mainframe | Huge I/O, batch + time-sharing, very high reliability | Throughput, RAS features | Cost, vendor lock-in |
| Desktop | Single user, rich GUI | Usability | No deadlines, large footprint |
| Multiprocessor (SMP / AMP) | Many CPUs share memory | Parallel throughput | Scaling limited by the serial part and memory bandwidth |
| Distributed | Separate computers cooperate over a network | Resource sharing across sites | Network failure, consistency |
| Clustered | Several machines share storage with fail-over | High availability | Complexity, split-brain risk |
| Hard real-time | Every deadline is guaranteed | Predictability | Few services |
| Soft real-time | Critical tasks get priority, no guarantee | Good for media and UI | Can miss deadlines |
| Handheld | Small, power-aware, wireless | Battery life | Limited CPU and memory |

## Chosen type per environment (with rejected alternatives)

| Environment | Chosen type | Rejected alternatives and reasons |
|---|---|---|
| Bedside monitors | **Hard real-time embedded OS** (QNX / FreeRTOS) | Soft real-time: may miss 10 ms. Desktop: unbounded latency (Task 1). |
| Nurses' tablets | **Handheld OS** (Android Enterprise / iPadOS) | Desktop OS: battery and start-up cost (Task 4). RTOS: no apps or UI. |
| Doctors' workstations | **Desktop OS** (Windows 11 / Ubuntu Desktop) | Handheld: too limited for imaging. Mainframe terminals: dated and costly. |
| Records database | **Clustered server OS with fail-over** (Linux + Pacemaker) | A single large multiprocessor is a single point of failure (see below). Mainframe meets availability but costs 5–10× more. |
| MRI imaging server | **Symmetric multiprocessor (SMP) OS** | Clustered: image data would have to cross the network. AMP: wastes cores (see below). |
| Partner link | **Distributed system** | Central mainframe for 3 hospitals: one site failure stops all three. |

## SMP vs AMP for the imaging server (measured)

`Task02/imaging_smp.c` runs a 3×3 smoothing filter over a 2048×2048 MRI-style slice, 6 passes, with rows split across threads that the OS may place on any core (SMP):

| Threads | Time (ms) | Speed-up |
|---|---|---|
| 1 | 19.7 | 1.00 |
| 2 | 4.3 | 4.54 |
| 4 | 2.8 | 7.08 |
| 8 | 2.4 | 8.18 |
| 16 | 2.4 | 8.20 |

The 1-thread run was often scheduled on an efficiency core, which is why 2 threads show a super-linear 4.5×. The speed-up flattens at about 8× because the M4 has 10 cores of two speeds and memory bandwidth becomes the limit. **SMP is chosen** because any idle core takes the next block of image rows, so the load balances itself. In **AMP**, one master core runs the OS and hands out work, so only N − 1 cores compute and the master becomes a bottleneck. AMP suits small embedded devices with dedicated cores (e.g. a DSP in the monitor), not the imaging server.

## Alternative concept: one big multiprocessor vs a fail-over cluster for the database

| Factor | Single large SMP machine | 3-node cluster with fail-over |
|---|---|---|
| Availability | Single point of failure. With ~99.9 % per machine, about 8.8 h of downtime per year, and every OS patch needs a reboot. | Survives 1 node failure. With 3 nodes at 99.9 % each, about 99.9999 % (≈ 30 s/yr in theory). Fail-over takes about 10–30 s and updates roll one node at a time. |
| Cost | One expensive high-end box (large SMP machines cost more than linearly). | 3 commodity servers plus shared or replicated storage. |
| Throughput (time) | Amdahl: S(p) = 1 / ((1−f) + f/p). With f = 0.95, 64 CPUs give only 15.4×, and lock and memory contention grow with p. | Read queries scale over replicas (≈ linear for reads). Writes are limited by the primary. |
| Memory (space) | One large shared memory: O(M), with no duplication. | Each node holds its own cache and replica: O(k × M) memory for k nodes. That costs space but buys availability. |

Decision: the **cluster** wins because the requirement "must never stop" ranks above raw throughput.

<!-- pagebreak -->

# Task 3 – Operating-System Structures (4 marks)

## Comparison

| Structure | Description | Reliability | Performance | Ease of updating |
|---|---|---|---|---|
| Simple / monolithic | All services in one kernel address space (MS-DOS, original UNIX) | Low: any bug crashes everything | Highest: one trap per request | Hard: rebuild and reboot |
| Layered | Layer n uses only layer n−1 (THE OS) | Good: each layer can be verified | Lower: a request crosses many layers | Moderate |
| Microkernel | Kernel does only IPC, scheduling and memory. Drivers and file systems are user-space servers (QNX, Mach, seL4). | Highest: a server crash is isolated and restartable | Lower: every service is a message round trip | Easy: replace a server without a reboot |
| Modular (LKM) | Core kernel plus loadable modules (Linux, Solaris) | Medium: modules still run in kernel space | High | Good: load or unload drivers at run time |
| Hybrid | Microkernel ideas with performance-critical parts kept in the kernel (Windows NT, macOS XNU) | Medium–high | High | Good |

## Measured evidence

`Task03/syscall_vs_ipc.c` compares one service request:

| Model | Cost per request |
|---|---|
| Monolithic: one system call (`getppid`) | **155 ns** |
| Microkernel: request message + reply message between two processes | **3,075 ns (20× slower)** |

`Task03/faulty_driver.c` injects the same NULL-pointer bug into a "display driver":

```
== monolithic structure ==
  alarm service: tick 1, vitals checked ... tick 3
  KERNEL PANIC: signal 11 in driver - alarm service died with it
== microkernel structure ==
  alarm service: tick 1 ... tick 3
  supervisor: display driver crashed (signal 11) - restarting it, alarms unaffected
  alarm service: tick 4 ... tick 6
  finished all 6 ticks, driver restarted 2 time(s)
```

## Real systems

- **Linux**: a monolithic kernel with loadable modules. Drivers run in kernel space and are loaded with `insmod`/`modprobe`.
- **Windows (NT)**: hybrid. The NT microkernel plus the Executive both run in kernel mode, with user-mode subsystems; many drivers moved to user mode (UMDF).
- **Android**: a Linux kernel (monolithic + modules), plus the HAL, plus the ART runtime. Each app runs in its own Linux UID sandbox.
- **QNX Neutrino**: a true microkernel. Drivers, file systems and the network stack are restartable user processes. It is used in medical devices and certified to IEC 62304 / IEC 61508.
- **FreeRTOS**: a tiny kernel (scheduler, queues, timers) linked into one image with the application. There is no user/kernel split unless the MPU port is used.

## Defended choices

- **Bedside monitors: microkernel (QNX) or minimal RTOS.** The 20× IPC cost is about 3 µs per request. Against the 10 ms deadline that is 0.03 %, which is negligible. In return, a buggy display or network driver cannot stop the alarm task. Reliability ranks 2 and the deadline is still met, so the **microkernel wins**.
- **Database server: monolithic + modular (Linux).** It makes millions of I/O requests per second, so a 20× cost per request would cut throughput badly. Reliability comes from the **cluster** (Task 2) rather than from the kernel structure, and modules plus live patching (`kpatch`) allow updates without downtime.

## Alternative concept: monolithic kernel on the monitors

| Factor | Monolithic | Microkernel |
|---|---|---|
| Time per service request | 1 trap ≈ 0.15 µs, O(1) | 2 messages + 2 context switches ≈ 3 µs, O(1) but 20× larger |
| Kernel memory | Larger kernel image, since all drivers are inside (Linux minimal ≈ 1–2 MB, typical ≈ 10+ MB) | Very small kernel (QNX ≈ 100 KB, seL4 ≈ 10 K lines); servers are added separately |
| Faulty driver | Crashes the whole system, so all alarms stop (demonstrated above) | Only that server dies and is restarted, so alarms continue |

Monolithic is faster but turns every driver bug into a patient-safety failure, so it is rejected for monitors.

<!-- pagebreak -->

# Task 4 – Operating-System Services (4 marks)

## Service table per environment (✔ include, ✖ exclude, ◐ reduced)

| Service | Monitor | Tablet | Workstation | Database | Imaging | Partner link |
|---|---|---|---|---|---|---|
| User interface | ◐ fixed display + buttons | ✔ touch GUI | ✔ full GUI | ◐ CLI / admin only | ◐ CLI | ✖ |
| Program execution | ◐ fixed tasks, no loading | ✔ sandboxed apps | ✔ | ✔ | ✔ batch jobs | ✔ |
| I/O operations | ✔ sensors (interrupt/DMA) | ✔ wifi, camera | ✔ | ✔ NVMe DMA | ✔ DMA from scanner | ✔ network |
| File system | ✖ (flash log only) | ✔ encrypted | ✔ | ✔ journaling, RAID | ✔ large files | ◐ |
| Communication | ✔ alarm to nurses' station | ✔ | ✔ | ✔ | ✔ | ✔ TLS |
| Error detection | ✔ watchdog, sensor checks | ✔ | ✔ | ✔ ECC, checksums | ✔ | ✔ |
| Resource allocation | ◐ static priorities | ✔ power-aware | ✔ | ✔ | ✔ SMP scheduler | ✔ |
| Accounting / audit | ✖ | ◐ | ◐ | ✔ who read which record (legal) | ◐ | ✔ access log |
| Protection & security | ✔ signed firmware | ✔ sandbox, MDM | ✔ logins | ✔ RBAC, encryption | ✔ | ✔ mutual TLS |

**Removed from the bedside monitor to make it smaller and faster:** the windowing GUI, multi-user accounts and logins, the general file system, the shell, dynamic program loading, accounting, printing, and package management. Each one removed means less memory and fewer interrupts or locks in the alarm path. It also removes attack surface and code that would need certification.

**Essential for the database server:** a journaling file system (crash consistency), error detection (ECC memory, checksums, SMART), protection and security (users, ACLs, encryption), accounting and audit (the law requires a log of who viewed each record), communication (network stack) and resource allocation. A GUI is not needed.

**How the OS protects one patient's record from another user's program.** Each process runs with a user ID. Every `open()` traps to the kernel, which checks the file's owner, permission bits and ACL against the caller's UID **before** returning a file descriptor. Memory protection (Task 8) stops a program from reading another process's buffers directly. `Task04/record_protection.sh` (run with sudo in the Linux lab) demonstrates this: user `nurse_ward3` can `cat` the record, while `clerk_billing` gets `Permission denied`. strace shows `openat(...) = -1 EACCES`. In the database, row-level security and audit logs add a second layer.

## Alternative concept: full desktop OS on the tablets

| Factor | Full desktop OS, all services | Reduced handheld OS |
|---|---|---|
| Memory at idle (space) | Windows 11 ≈ 3–4 GB RAM in use, ≈ 30+ GB disk; Ubuntu Desktop ≈ 1.5 GB RAM, ≈ 10 GB disk | Android ≈ 1–1.5 GB RAM, ≈ 6–8 GB system partition |
| Start-up time | 20–60 s cold boot (every service starts) | Normally never cold-boots; resumes from sleep in < 1 s |
| Battery (time) | Background services and timers keep waking the CPU, so a typical laptop-class OS on a tablet lasts about 4–6 h | Doze/App Standby batch the wake-ups, so a full 12 h shift is achievable |

Each extra service costs memory and storage (space, O(services)). Worse, it adds periodic wake-ups, and every wake-up costs energy, so battery life falls roughly in proportion to the number of wake-ups per second. **Reduced handheld OS chosen.**

<!-- pagebreak -->

# Task 5 – System Calls (4 marks)

## Categories with hospital examples

| Category | Example calls (POSIX / Linux) | Hospital use |
|---|---|---|
| Process control | `fork`, `execve`, `exit`, `wait4`, `kill` | Shell on the database server starts a backup job |
| File management | `open`, `read`, `write`, `close`, `unlink`, `stat` | Reading a patient record |
| Device management | `ioctl`, `read`/`write` on `/dev/*`, `mmap` | Monitor reads a sensor; driver controls the scanner |
| Information maintenance | `getpid`, `time`, `clock_gettime`, `uname` | Timestamping vitals |
| Communication | `pipe`, `socket`, `send`, `recv`, `shmget` | Alarm sent to the nurses' station |
| Protection | `chmod`, `chown`, `setuid`, `umask` | Restricting record files to the ward's nurses |

## What happens step by step when a program calls `read()`

1. **Library call.** The program calls `read(fd, buf, 64)`. This is an ordinary function call into the C library (glibc / libSystem) wrapper.
2. **Parameters into registers.** The wrapper puts the system-call number and arguments into registers. On Linux ARM64 the number goes in `x8` and fd, buf, count in `x0`, `x1`, `x2`. On x86-64 they go in `rax`, `rdi`, `rsi`, `rdx`. On macOS ARM64 the number goes in `x16`.
3. **Trap.** It executes the trap instruction (`svc #0` / `syscall`). The CPU switches the **mode bit from user to kernel**, saves the user PC and status, and jumps to the fixed system-call entry vector.
4. **Dispatch.** The kernel saves the registers and uses the number to index the **system-call table**, which leads to `ksys_read()`.
5. **Kernel work.** It checks that `fd` is open for reading and that `buf` is a valid user address. It then calls the file system's read routine. If the data is in the page cache it is copied straight to user memory. If not, the kernel starts a disk I/O (DMA), **blocks the process**, schedules another one, and wakes this one on the completion interrupt.
6. **Return.** The byte count (or `-errno`) is placed in `x0`/`rax`. The `eret`/`sysret` instruction restores user mode and the saved PC.
7. **Back in the library.** The wrapper sets `errno` if the result is negative and returns −1, otherwise it returns n.

## Parameter passing

- **Registers.** Up to 6 arguments; this is the fastest method and the one Linux uses.
- **Block in memory.** The address of a parameter table goes in one register. Used when there are more arguments than registers (e.g. old `mmap` on x86-32, `ioctl` structs).
- **Stack.** Pushed by the program and popped by the OS (classic x86-32 `int 0x80` conventions).

Evidence: `Task05/raw_syscall.c` issues `write()` with **no C library**, loading the registers itself and executing the trap:

```
macOS arm64 : syscall number in x16, arguments in x0,x1,x2, trap = svc #0x80
ALARM: SpO2 below 90% on bed 17 (written with a raw trap)
kernel returned 58 (bytes written)
```

## strace evidence

`strace ./read_demo patient_record.txt` — **to be captured in the Linux lab** (strace does not exist on macOS, where these programs were run). The program's own calls, after the loader, will have this form:

```
openat(AT_FDCWD, "patient_record.txt", O_RDONLY) = 3
read(3, "PATIENT RECORD  (synthetic test "..., 64) = 64
write(1, "PATIENT RECORD  (synthetic test "..., 64) = 64
read(3, "d: 17\nName: TEST PATIENT ALPHA\nV"..., 64) = 64
write(1, "d: 17\nName: TEST PATIENT ALPHA\nV"..., 64) = 64
...
read(3, "ilising.\n", 64)                = 9   (265 = 4*64 + 9)
write(1, ...9 bytes...)                = 9
read(3, "", 64)                         = 0
close(3)                                = 0
exit_group(0)                           = ?
```

Each line is one trap: the call name, its arguments (decoded by strace from the registers) and the return value. Before these come the loader's `execve`, `mmap`, `openat("libc.so.6")` and so on. `strace -c` gives a count table per call. The 265-byte record needs ⌈265/64⌉ + 1 = **6 `read()` calls** (the last returns 0 for EOF) and 5 `write()` calls. *(Run `Task05/run.sh` in the Linux lab; it saves the full trace to `outputs/strace_read_demo.txt`.)*

## Alternative concept: `fopen`/`fread`/`fwrite` instead of `open`/`read`/`write`

| | Direct `read`/`write` with b bytes | stdio `fread`/`fwrite` |
|---|---|---|
| System calls for n bytes | ≈ 2 × n/b (each read paired with a write), O(n/b) | ≈ 2 × n/4096, O(n/BUFSIZ), whatever b the program uses |
| Extra space | b bytes (the program's buffer) | + one 4 KB buffer **per open `FILE*`** (BUFSIZ; macOS uses the block size) |

Measured with an 8 MB file (`Task06/copy_bench.c`): **b = 1** byte took **13,526 ms** with direct `read`/`write` (8.4 million read calls) but **271 ms** with `fread`/`fwrite` (still 8.4 million library calls, but only about 2,048 real `read()`s). That is a **50× gain** from library buffering. At b = 4096 both take about 5.4 ms, because the library buffer gives no extra benefit once the program's buffer is already that large.

<!-- pagebreak -->

# Task 6 – Cost of System Calls (4 marks)

## Why a system call costs more than a function call

A function call is a jump plus a return: about 1–2 ns. A system call must also execute the trap, switch mode, save and restore the full register state, validate arguments, go through the dispatch table, and may run speculative-execution mitigations or flush pipelines and TLB/cache state. Measured (`Task06/syscall_cost.c`):

```
ordinary function call :    2.28 ns
getppid() system call  :   70.33 ns
system call / function :      31x more expensive
```

## Complete the Code: `copy_file.c` (TODO 1)

```
    if (bufsize > sizeof buf) bufsize = sizeof buf;      /* never overflow buf */
    for (;;) {
        n = read(in, buf, bufsize);
        calls++;                                         /* the EOF read counts too */
        if (n <= 0) break;                               /* 0 = end of file, -1 = error */
        ssize_t done = 0;
        while (done < n) {                               /* write() may be partial */
            ssize_t w = write(out, buf + done, (size_t)(n - done));
            if (w < 0) { n = -1; break; }
            done += w;
        }
        if (n < 0) break;
    }
    if (n < 0) calls = -1;
```

Output (identical to the expected output):

```
buffer    1 bytes: 10001 read() calls
buffer  512 bytes:    21 read() calls
buffer 4096 bytes:     4 read() calls
```

Why: reads = ⌈n/b⌉ + 1 (the final read returns 0). That gives 10000 + 1, then ⌈10000/512⌉ + 1 = 20 + 1, then ⌈10000/4096⌉ + 1 = 3 + 1. When run under `strace -c ./copy_file` on Linux, the `read` row is expected to show 10001 + 21 + 4 = 10,026 calls plus a few made by the dynamic loader at start-up. Any difference from the program's own counts is only those start-up reads.

## Measured buffer-size sweep (8 MB backup file, `copy_bench.c`)

| Buffer (bytes) | read() calls | write() calls | Time (ms) | MB/s |
|---|---|---|---|---|
| 1 | 8,388,609 | 8,388,608 | 13,526.0 | 0.6 |
| 16 | 524,289 | 524,288 | 891.4 | 9.0 |
| 64 | 131,073 | 131,072 | 211.0 | 37.9 |
| 512 | 16,385 | 16,384 | 28.3 | 282.6 |
| 1,024 | 8,193 | 8,192 | 15.2 | 526.5 |
| 4,096 | 2,049 | 2,048 | 5.4 | 1,475.2 |
| 8,192 | 1,025 | 1,024 | 3.6 | 2,200.8 |
| 16,384 | 513 | 512 | 3.1 | 2,614.4 |
| **65,536** | **129** | **128** | **2.5** | **3,215.4** |
| 131,072 | 65 | 64 | 2.1 | 3,761.2 |
| 1,048,576 | 9 | 8 | 2.1 | 3,815.0 |
| fread/fwrite b=1 | (≈2,049 real) | | 271.3 | 29.5 |
| mmap + memcpy | 0 | 0 | 2.3 | 3,530.5 |

Time is almost exactly proportional to the number of calls until b ≈ 4 KB: each 4× drop in calls gives about a 4× drop in time. After 64 KB the gain is under 20 %, because the cost is now the memory copy itself (O(n)), not the number of calls (O(n/b)).

**Recommendation for the backup program: a 64 KB buffer.** It gets within about 15 % of the best throughput, makes about 500× fewer calls than 128 B, and uses little memory. 1 MB is only slightly faster, and its 1 MB per copy stream adds up when the server runs many backups at once.

## Alternative concept: `mmap` instead of `read`/`write`

| | read/write with buffer b | mmap |
|---|---|---|
| System calls | ≈ 2n/b, O(n/b) | O(1): `open`×2, `fstat`, `ftruncate`, `mmap`×2, `munmap`×2, `close`×2 (about 10, independent of n) |
| Kernel↔user copying | 2 copies: page cache → user buffer → page cache | 1 copy: `memcpy` between the two mapped page-cache pages. Pages are loaded by page faults (≈ n/4096 faults, which are cheaper than syscalls) |
| Memory for very large files | Constant, b bytes | Virtual address space = file size. Physical memory is managed by paging, but a 100 GB image needs 100 GB of address space (fine on 64-bit), and a write-fault storm can thrash. Mapping in chunks of e.g. 1 GB keeps it bounded. |

Measured: mmap took 2.3 ms against 2.5 ms for the 64 KB buffer, about the same. For the backup program, mmap's advantage is the near-zero syscall count. Its drawbacks are error handling (I/O errors arrive as SIGBUS) and the address-space size for huge files.

<!-- pagebreak -->

# Task 7 – Process Control System Calls (4 marks)

## What each call does

- **`fork()`**: creates a child that is a copy of the caller. It returns 0 in the child and the child's PID in the parent. Memory is shared **copy-on-write**: pages are copied only when one side writes.
- **`exec()`** (`execve`, `execlp`, …): replaces the calling process's program with a new one. The PID and open files are kept. It returns only on failure.
- **`wait()` / `waitpid()`**: blocks the parent until a child ends, then collects its exit status and removes its process-table entry.
- **`exit()`**: ends the process. The C-library `exit()` first flushes stdio buffers and runs `atexit` handlers. `_exit()` goes straight to the kernel.

## How the shell runs a command and gets its status

The shell reads a line and calls `fork()`. The child calls `exec(cmd)`. The parent calls `waitpid(pid, &status, 0)`. The status word encodes either a normal exit (`WIFEXITED`, with `WEXITSTATUS` = 0–255) or death by a signal (`WIFSIGNALED`, `WTERMSIG`). The shell stores this in `$?`.

```
    shell (pid P)
       |
       |-- fork() ---------------------> child (pid C, copy of shell)
       |                                     |
       | waitpid(C) ... blocked              | execlp("expr","6","*","7")
       |                                     |   -> becomes /usr/bin/expr
       |                                     | prints 42, exit(0)
       |<-- SIGCHLD / status 0 --------------+  (zombie until reaped)
       | prints "job 1 exit status 0"
```

## Complete the Code: `shell_jobs.c` (TODO 1)

```
    int status;
    pid_t pid = fork();
    if (pid < 0) { perror("fork"); return 1; }
    if (pid == 0) {                                      /* child */
        execlp("expr", "expr", "6", "*", "7", (char *)NULL);
        exit(127);                                       /* only reached if exec failed */
    }
    waitpid(pid, &status, 0);                            /* parent */
    printf("job 1 exit status %d\n", WEXITSTATUS(status));
```

Output (identical to the expected output):

```
shell: running job 1
42
job 1 exit status 0
shell: running job 2
job 2 exit status 3
```

`pid` and `status` are declared in the TODO because the given second half of the program uses them.

## Why stdout is flushed before each `fork()`

`printf` writes into a user-space stdio buffer, not to the kernel. `fork()` copies the **whole address space, including that buffer**. If unflushed text is in the buffer at fork time, parent and child each hold a copy, and both write it out when they flush. To a terminal stdout is line-buffered, so the `\n` flushes it. To a **file or pipe it is fully buffered** (4 KB), so text stays in the buffer.

## The experiment: remove the 2nd `fflush`, change `_exit(3)` to `exit(3)`, redirect to a file

```
## variant, stdout redirected to a file
shell: running job 1
42
job 1 exit status 0
shell: running job 2
job 1 exit status 0        <- duplicated
shell: running job 2       <- duplicated
job 2 exit status 3
```

Explanation: with output going to a file, "job 1 exit status 0\nshell: running job 2\n" is still in the parent's buffer when the second `fork()` happens. The child inherits a copy. `exit(3)` **flushes stdio buffers** before ending, so the child writes the copied text. Later the parent writes its own copy. My run shows that **"job 1 exit status 0" is duplicated too**, not only "shell: running job 2", because both lines were in the buffer at fork time. With `_exit(3)` the child skips the stdio flush, which is why the original program was correct.

## Zombie and orphan (measured, `zombie_orphan.c`)

```
== zombie: child 6026 has exited, parent 6024 has not waited ==
  PID  PPID STAT COMM
 6026  6024 Z    <defunct>
after waitpid():   (entry gone)
== orphan ==
orphan-to-be 6030: my parent is 6029
parent 6029 exits without waiting for 6030
orphan 6030: my parent is now 1 (adopted, will be reaped by it)
```

- A **zombie** is a child that has ended but has not been waited for. It keeps only its PID and exit status in the process table, so that the parent can still read them. Many zombies exhaust PIDs. The OS removes a zombie when the parent waits for it or when the parent itself dies (it is then adopted by init and reaped).
- An **orphan** is a running child whose parent has exited. The kernel re-parents it to PID 1 (`init`/`systemd`/`launchd`, or a subreaper), which calls `wait()` for it when it ends.

`mysh.c` is a small working shell. It ran `expr 6 * 7` (status 0), `ls /no/such/file` (status 1), `nosuchcmd` (127, exec failed) and `false` (1).

## Why `fork()` + `exec()` instead of one "create process" call

The split leaves a window in the child between fork and exec. In that window the shell can set up the new program's environment using ordinary system calls: redirect I/O (`dup2` for `>` and `|`), change directory, drop privileges (`setuid`), set resource limits, or close files. A single `CreateProcess` call would need a parameter for every one of these. Copy-on-write makes the fork cheap, because pages are not really copied before the exec discards them.

## Alternative concept: `posix_spawn()` or threads (measured, 1,000 starts, `spawn_bench.c`, parent holding 64 MB)

| Method | Total (ms) | Per start (µs) | Memory copied/shared | Isolation |
|---|---|---|---|---|
| `fork()` + `exec()` | 505.5 | 505.5 | Page tables for 64 MB copied (COW), O(parent size), then discarded by exec | Full: separate address space |
| `posix_spawn()` | 21.8 | 21.8 | No page-table copy (vfork-style), O(1) in parent size | Full: separate address space |
| `pthread_create()` | 12.6 | 12.6 | Shares everything; only a new stack, O(1) | **None.** A thread cannot run a different program, and a crash in one kills all. |

`posix_spawn` is **23× faster** than fork+exec for a large parent, with the same isolation, so it is the better choice for a database server that launches many helper commands. Threads are faster still but give no isolation, so they are unsuitable for running untrusted or separate commands.

<!-- pagebreak -->

# Task 8 – CPU States and Protection (3 marks)

## User mode, kernel mode and the mode bit

The CPU has a **mode bit** (x86 CPL ring 0/3, ARM exception level EL0/EL1). In **kernel mode** every instruction is allowed. In **user mode**, **privileged instructions** trap to the kernel instead of running. Examples: disabling interrupts, changing page tables, I/O port access, halting, and writing the timer or control registers. The only way from user to kernel mode is a trap or interrupt, which enters at a kernel-chosen address. The kernel returns to user mode with `eret`/`iret`.

Measured (`Task08/privileged.c`, running as a normal user program):

```
  disable interrupts                     -> trapped, kernel sent Illegal instruction
  write system control register          -> trapped, kernel sent Illegal instruction
  read a kernel-space address            -> trapped, kernel sent Segmentation fault
  read an unmapped address (NULL+8)      -> trapped, kernel sent Segmentation fault
The program is still alive: every violation was caught by hardware + kernel.
```

## Interrupts, traps and the timer

- An **interrupt** is asynchronous and comes from hardware: a sensor ready, a DMA transfer complete, a network packet. The CPU finishes the current instruction, saves state, switches to kernel mode and runs the handler found in the interrupt vector table.
- A **trap / exception** is synchronous and caused by the running instruction: a system call (`svc`), divide by zero, a page fault, or an illegal instruction (as above).
- The **timer interrupt** is set by the kernel (a privileged operation) to fire every few ms. It guarantees the OS gets control back even if a program loops forever. That is how preemptive scheduling works, and how a monitor's watchdog catches a stuck task.

## If any program could disable interrupts (monitors)

A buggy or malicious program could execute "disable interrupts" and then loop. The sensor interrupt reporting an abnormal reading would never be delivered, and the timer interrupt would never fire. The scheduler could not preempt the program and the alarm task would never run. **The patient alarm would be silently lost**, and even the software watchdog would be blind; only a hardware watchdog would eventually reset the device. This is why disabling interrupts must be privileged. The demo shows that the hardware refuses it in user mode (SIGILL).

## How the hardware stops one program reading another's memory

The **MMU** translates every virtual address through the current process's **page table**. The kernel loads it (a privileged register: CR3 on x86, TTBR0 on ARM) at each context switch. Another process's physical pages simply **do not appear** in this table, so they cannot even be named. Pages marked kernel-only cause a fault from user mode. Older or simpler machines use **base and limit registers**, where every address is checked against the range and a violation traps. Small MCUs in monitors use an **MPU** with per-task regions. Reading `/proc/<pid>/mem` of another process is allowed only with ptrace rights. On macOS that interface does not exist at all (the run reported "No such file or directory"), and `task_for_pid` requires entitlements.

## Alternative concept: polling instead of interrupts (measured, `poll_vs_interrupt.c`)

The sensor raises an abnormal reading at a random time 50–100 ms in. Each method was run for 10 trials:

| Method | Avg delay | Worst delay | CPU used per trial | Meets 10 ms? |
|---|---|---|---|---|
| Interrupt-style (blocked in `read`, kernel wakes it) | 30 µs | 44 µs | 1.08 ms | ✔ |
| Busy polling (no sleep) | 3 µs | 6 µs | **64.21 ms** (≈ 100 % of a core) | ✔ but wastes the CPU |
| Poll every 0.1 ms | 75 µs | 130 µs | 1.43 ms | ✔ |
| Poll every 1 ms | 610 µs | 1,272 µs | 1.29 ms | ✔ |
| Poll every 5 ms | 2,729 µs | 6,266 µs | 1.12 ms | ✔ (little margin) |
| Poll every 20 ms | 11,943 µs | **24,472 µs** | 1.08 ms | **✖ misses the deadline** |

Analysis: with polling interval P, worst delay ≈ P + scheduling jitter, average ≈ P/2. CPU cost is O(T/P) wake-ups over a waiting time T. Polling fast enough to be safe either burns a whole core (busy polling, which is also the worst for power) or wakes up thousands of times per second. Interrupts give worst delay = interrupt latency (tens of µs here, about 1–10 µs on an RTOS) with **zero CPU spent while waiting**. **Interrupts are chosen for monitors.**

<!-- pagebreak -->

# Task 9 – Memory Hierarchy and I/O Systems (4 marks)

## Levels of the hierarchy

| Level | Typical size | Typical access time | **Measured on M4** (`mem_hierarchy.c`) |
|---|---|---|---|
| Registers | ~ 32 × 64-bit | < 0.3 ns (1 cycle) | – |
| L1 cache | 64–128 KB / core | ~1 ns | 4–64 KB: **1.0–2.0 ns** |
| L2 / last-level cache | 4–16 MB (shared) | 3–10 ns | 256 KB–16 MB: **3.0–9.5 ns** |
| Main memory (DRAM) | 8–512 GB | 60–100 ns | 64–256 MB: **70–90 ns** |
| SSD (NVMe) | 0.5–8 TB | 20–100 µs | – |
| Hard disk | 4–20 TB | 5–10 ms | – |
| Network storage (NAS/SAN/cloud) | PB | 0.5–50 ms | – |

The measured curve shows clear steps where the working set outgrows each cache: about 1 ns, then 3–9 ns, then a jump to 70–90 ns once it exceeds the 16 MB cache. DRAM is about **70× slower than L1**.

## Data-placement plan

| Data | Where kept | Justification |
|---|---|---|
| Live monitor readings (last seconds) | Monitor RAM/registers, ring buffer | The alarm decision must use 1–100 ns memory, never a disk or network. |
| Monitor history (24 h) | Monitor flash + database | Needs persistence. Write rarely, in batches, to limit flash wear. |
| Active patient records (current admissions) | Database server RAM (page cache + buffer pool) on SSD | Read constantly; a µs lookup is better than a ms one. 500 beds × ~1 MB ≈ 0.5 GB fits in RAM. |
| All records (historical) | NVMe SSD with RAID + replicas | Large, accessed by index. |
| MRI images (recent) | Imaging-server NVMe SSD | 500 MB each; needs high sequential bandwidth. |
| MRI archive | HDD / network storage (PACS) or cloud | Rarely read; cheapest per TB. |

## Programmed I/O, interrupt-driven I/O and DMA

- **Programmed I/O (polling).** The CPU checks a device status register and moves every word itself. Simple, but it wastes CPU time (Task 8 table).
- **Interrupt-driven I/O.** The device interrupts when ready and the CPU moves the data in the handler. Good for slow, occasional events (keyboard, monitor alarm sensor).
- **DMA.** The CPU programs the DMA controller with an address, a count and a direction. The controller moves the whole block between device and memory and raises **one** interrupt at the end. **Best for the MRI scanner**: large blocks at high bandwidth while the CPU stays free for reconstruction.

## Effect of caching on record lookup (measured, `record_lookup.c`, 100,000 records, 1,000 random lookups)

| Path | Time per record |
|---|---|
| File read with caching disabled (`F_NOCACHE`) | 0.627 µs |
| OS page cache (`pread` system call, data in RAM) | 0.245 µs |
| Application in-memory table (no system call) | **0.004 µs** (157× faster) |

On macOS, `F_NOCACHE` could not evict pages already cached, so the first row is still mostly served from RAM. A true SSD read is 20–100 µs, about 100–400× slower than the page cache. The measurements show the two cache layers clearly: the OS page cache removes the device access, and an application cache also removes the system-call cost (Task 6). The database therefore keeps its hot index and records in its buffer pool.

## Alternative concept: interrupt per word instead of DMA for a 500 MB image

| | Interrupt-driven (1 interrupt per 4-byte word) | DMA (e.g. 64 KB blocks) |
|---|---|---|
| Number of interrupts | 500 × 2^20 / 4 = **131,072,000** | 500 MB / 64 KB = **8,000** (or 1 with scatter-gather) |
| CPU time | Each interrupt ≈ 1–2 µs (entry, save, handler, return; our system call alone measured 0.07–0.15 µs, and interrupts cost more), so 131 M × 1 µs ≈ **131–262 s of CPU** | 8,000 × 2 µs ≈ **16 ms**, plus setup |
| Memory buffers | A 1-word device register; the CPU copies each word into the 500 MB destination. No extra buffer, but the CPU does everything. | Destination buffer in RAM (500 MB, or a ring of DMA blocks), pinned and contiguous or scatter-gather. |
| Complexity | Time O(n) CPU work | CPU work O(n/block), transfer done by hardware |

At 131 million interrupts the CPU would spend minutes doing nothing but copying, with nothing left for image reconstruction. **DMA is required for the MRI scanner.**

<!-- pagebreak -->

# Task 10 – Measure and Compare (4 marks)

All measurements come from the programs in this repository, run on the test machine described on the cover page.

## 1. System-call counts vs buffer size

The counts follow exactly reads = ⌈n/b⌉ + 1 and writes = ⌈n/b⌉. They were counted inside the program (Tasks 6 and 10); `Task10/run.sh` repeats them under `strace -c` in the Linux lab. For a 16 MB file:

| Buffer b | read() calls | write() calls |
|---|---|---|
| 1 | 16,777,217 | 16,777,216 |
| 64 | 262,145 | 262,144 |
| 512 | 32,769 | 32,768 |
| 4,096 | 4,097 | 4,096 |
| 65,536 | 257 | 256 |
| 1,048,576 | 17 | 16 |

## 2. User-mode vs kernel-mode time (`/usr/bin/time`, 16 MB copy)

| Buffer | real (s) | user (s) | sys (s) | % in kernel |
|---|---|---|---|---|
| 1 | 28.07 | 1.90 | **25.70** | 92 % |
| 64 | 0.42 | 0.02 | 0.39 | 93 % |
| 512 | 0.05 | 0.00 | 0.05 | ≈ 100 % |
| 4,096 | 0.01 | 0.00 | 0.00 | – |
| 65,536 | 0.00 | 0.00 | 0.00 | – |
| 1,048,576 | 0.00 | 0.00 | 0.00 | – |

Almost all of the time is spent in **kernel mode**, in trap handling and per-call work. User time is tiny because the program only loops. This confirms Task 6: the cost is the **number** of system calls.

## 3. Creating 1,000 processes vs 1,000 threads (`create_bench.c`, all alive at once)

| Run | 1,000 processes (`fork`) | 1,000 threads (`pthread_create`) |
|---|---|---|
| 1 | 322.6 ms | 3.5 ms |
| 2 | 376.0 ms | 3.5 ms |
| 3 | 379.6 ms | 3.8 ms |
| **Mean** | **359 ms (359 µs each)** | **3.6 ms (3.6 µs each)** |

`/usr/bin/time -l`: the process run took 0.59 s of **system** time against 0.06 s user, so the work is in the kernel (copying page tables, creating process structures). The thread run's creator reached an 18 MB maximum resident set, because all 1,000 thread stacks and thread structures live inside **one** process. The process run's creator stayed at 1.7 MB, because each child's memory is counted separately in the child.

## 4. Do the measurements support the earlier judgements?

| Earlier judgement | Measurement | Verdict |
|---|---|---|
| A general-purpose OS cannot guarantee the 10 ms alarm (Task 1) | Worst wake-up 7.2 ms on an idle machine; 99.9th percentile up to 3.2 ms | **Supported.** No bound, and the tail is large. |
| A system call costs much more than a function call (Task 6) | 31× (70 ns vs 2.3 ns) | **Supported.** |
| A bigger buffer means fewer calls and faster (Task 6) | 13.5 s → 2.5 ms (5,400×), flat after 64 KB | **Supported.** Diminishing returns as predicted. |
| Microkernel IPC is slower than a system call (Task 3) | 20× | **Supported**, but 3 µs is still negligible against 10 ms. |
| posix_spawn is cheaper than fork+exec for a big parent (Task 7) | 23× | **Supported.** |
| Interrupts beat polling (Task 8) | Same delay as polling every 0.1 ms, with 60× less CPU than busy polling | **Supported.** |
| *Surprise* | SMP speed-up was super-linear at 2 threads | **Contradicted** the simple model. Caused by heterogeneous P/E cores: the 1-thread baseline ran on a slow core. |

## Alternative concept: threads instead of processes

| | 1,000 processes | 1,000 threads |
|---|---|---|
| Creation time | 359 ms, O(n × size of parent page tables) | 3.6 ms, O(n), **≈ 100× faster** |
| Memory | Each needs its own page tables, kernel task structure, file table, signal table and COW-shared pages (≈ 100–200 KB of kernel memory each on Linux, more once pages are written) | Each needs only a stack (8 MB virtual, a few KB actually touched) and a small thread structure (≈ 10–20 KB each) |
| What is shared | Nothing is writable-shared. Code pages are shared read-only and data is copy-on-write. | **Everything**: code, heap, globals, open files, signal handlers. Only the stack, registers and thread-local storage are private. |

The difference follows directly from sharing. A process needs a new address space, so the kernel must build and copy page tables. A thread re-uses the existing one. The price is isolation: one faulty thread can corrupt or crash all the others.

<!-- pagebreak -->

# Task 11 – Recommended Operating-System Plan (4 marks)

(Full plan as a configuration file: `Task11/hospital_os_plan.yaml`.)

| Environment | OS type | Structure | Key services | Protection | I/O method | Evidence |
|---|---|---|---|---|---|---|
| Bedside monitors (500) | Hard real-time embedded (QNX / FreeRTOS) | Microkernel / minimal kernel | Fixed tasks, sensor I/O, watchdog, alarm network | MPU regions, signed firmware, no user code | Interrupt-driven sensors, DMA for waveforms | T1: GP OS worst case 7.2 ms; T3: driver fault isolation; T8: polling misses deadline |
| Nurses' tablets (120) | Handheld (Android Enterprise / iPadOS) | Linux kernel + sandboxed apps | Touch UI, wifi, push notifications, encrypted storage | Per-app UID sandbox, MDM, remote wipe | Interrupt / push, no polling | T4: battery and start-up; T8: polling cost |
| Doctors' workstations (80) | Desktop (Windows 11 / Ubuntu) | Hybrid / modular | Full GUI, file system, printing | Domain login, RBAC, disk encryption | Standard DMA disk/network | T4 |
| Records database | Clustered server OS, 3 nodes, fail-over | Monolithic + LKM, live patching | Journaling FS, audit, RBAC, networking | File permissions, DB row security, TLS | DMA NVMe, page cache, 64 KB buffered backup, posix_spawn for jobs | T2: availability; T6: 64 KB buffer; T7: spawn 23× faster; T9: caching 157× |
| MRI imaging server | SMP multiprocessor OS | Monolithic + LKM | Parallel threads, batch queue | Standard users / ACLs | **DMA** from scanner | T2: 8× SMP speed-up; T9: DMA vs 131 M interrupts |
| Partner link | Distributed system | – | Replication, TLS, audit | Mutual TLS, OAuth scopes | Network DMA | T1 security ranking |

## Limitations and risks

- **Four OS platforms.** More skills and more patch processes are needed. Mitigation: central MDM and configuration management (Ansible), and the RTOS is almost never patched.
- **RTOS certification and cost.** QNX licences, and IEC 62304 certification takes time. FreeRTOS is cheaper but needs more in-house safety work.
- **Cluster risks.** Split-brain and fail-over bugs. Needs fencing (STONITH) and regular fail-over drills.
- **Measured on macOS/XNU, not on the final hardware.** Absolute numbers will differ on Linux servers and microcontrollers, but the ratios (31×, 20×, 100×) are architectural and carry over.
- **Network dependency.** If the hospital LAN fails, monitors still alarm locally, but the central station view is lost.

## Suitability for the group of three hospitals

The plan scales: 1 ward (50 devices) → 1 hospital (700) → 3 hospitals (≈ 2,100+). Monitors, tablets and workstations scale linearly and independently, at O(n) with no shared bottleneck. The database cluster becomes the bottleneck. Each hospital keeps its own cluster for local availability, and the partner link becomes a **distributed records system** with replication between sites. This is the change taken further in Task 12.

## Alternative concept: everything as VMs on two large servers

| Factor | Effect |
|---|---|
| Monitor response time | The hypervisor adds its own scheduler. A vCPU can be descheduled for milliseconds when the host is busy (CPU steal). Interrupts must be virtualised (+1–10 µs normally, with unbounded tails). The monitors also need physical sensors at the bed, so **they cannot be virtualised at all**. Only their central station could be. |
| Memory / CPU overhead | ≈ 2–10 % CPU for the hypervisor, plus 0.5–2 GB of RAM per guest OS. 700 guests would need ≈ 700 GB just for the OS images. |
| Reliability | Two hosts become two very large failure domains: one host failure takes down half the hospital. Live migration and HA help but add complexity. One hypervisor bug affects everything. |
| Where it **is** a good idea | Database nodes, imaging and admin servers. Consolidation, snapshots and live migration help availability there. |

Verdict: **virtualise the server tier only**. Monitors and tablets stay physical with their own OS types.

<!-- pagebreak -->

# Task 12 – Day-4 Design Challenge: Re-evaluate and Adapt (8 marks)

**Changed requirement:** remote monitoring devices in patients' homes send readings over the internet, and patient records move to a cloud service shared with the partner hospitals.

## A. Impact analysis (2 marks)

| Area | Affected? | How |
|---|---|---|
| OS **types** | **Yes** | A new environment (home device) needs a battery-powered embedded real-time OS. Records change from a **clustered** system inside one hospital to a **distributed** system across cloud regions and three hospitals. |
| Response time | **Yes** | The path from home to hospital crosses the public internet: 50–250 ms round trip, with outages. The 10 ms alarm guarantee **cannot** be met over the WAN. |
| Communication service | **Yes** | Needs store-and-forward, retries and message queues (MQTT/HTTPS), not a LAN socket. |
| Error detection | **Yes** | Must detect a lost link, a dead battery or a device off-body. Missing data is now an event in itself. |
| Protection and security | **Yes, strongly** | Devices sit in untrusted homes on untrusted networks. Needs device identity (certificates), TLS 1.3, secure boot, signed over-the-air updates, and multi-tenant access control in the cloud with audit across organisations. |
| File system / storage | **Yes** | Records live in cloud storage. The hospital needs a local replica or cache for when the WAN is down. |
| Bedside monitors, tablets, workstations, imaging | **No** | Unchanged. |

## B. Evaluation of the original design (2 marks)

| Criterion | Original plan applied unchanged | Evidence |
|---|---|---|
| Response time | **Fails** if the alarm decision is made centrally: WAN delay is 80–250 ms, and **unbounded** during an outage. | Simulation `Task12/home_monitor.c`: cloud-decided alarms caught **1 of 4** desaturation episodes, because 3 occurred during outages; average delay was 215 ms when they did work. |
| Reliability | The on-premise cluster survives server failure but not the loss of the hospital site or WAN. Partner hospitals lose access when the link drops. | Task 2 availability analysis. |
| Security | The original plan assumed a trusted hospital LAN. Home devices and a shared cloud break this assumption. | Task 4 protection model relies on local UIDs only. |
| Battery | No battery requirement existed for monitors. Home devices must last days. | Task 8: polling costs CPU and power. |

Conclusion: the core of the plan (an RTOS for anything that raises alarms, a handheld OS for staff, SMP for imaging) is **still correct**. The **records architecture and the security model are not**, and the home device is a new environment with no design yet.

## C. Adaptation (2 marks): decision = **MODIFY**

(Configuration: `Task12/revised_plan.yaml`.)

- **Retain** the bedside monitors (RTOS, microkernel), tablets, workstations and imaging server.
- **Add home monitors:** an **embedded real-time OS** (FreeRTOS / Zephyr) on a low-power microcontroller.
  - The **alarm decision is made on the device** in < 1 ms and sounds a local alert. It never waits for the network.
  - **Store-and-forward ring buffer:** 1,024 readings × 4 B = 4 KB covers 17 minutes at 1 reading per second. Data is uploaded when the link returns.
  - **Interrupt-driven** sensors and **tickless idle** for battery life (Task 8). Readings are sent in batches.
  - Secure boot, signed OTA updates, and TLS 1.3 with a per-device certificate.
- **Modify records into a distributed system:** a cloud records service replicated across ≥ 2 availability zones, with a **local read replica / cache in each hospital** so wards keep working if the WAN fails. Access goes through mutual TLS with OAuth2 scopes per hospital, and the audit log is shared.
- **Modify the partner link:** it becomes the cloud API, replacing the point-to-point VPN.
- **Add a cloud gateway:** a device registry, a message queue, and escalation of home alarms to the on-call nurse's tablet (soft real time, seconds).

## D. Evidence-based defence (2 marks)

Simulated hour (`Task12/home_monitor.c`): 4 desaturation episodes, and 3 internet outages of 300 s, 30 s and 600 s:

| Design | Alarms raised | Missed | Alarm delay | Readings lost |
|---|---|---|---|---|
| Cloud decides (original-style central decision) | 1 | **3** | 215 ms | **930** |
| Edge decides (revised: RTOS on the device) | **4** | **0** | < 1 ms (local) | **0**. Peak buffer use was 600 of 1,024 slots, and 930 readings were forwarded after reconnect. |

Trade-offs accepted:

- **More code on each device** (alarm logic, buffering, crypto), which needs about 64–256 KB of flash. In return the alarm works with no network, which is the only way to satisfy a safety-critical requirement over an unreliable link (Task 1 ranking: response time and reliability above everything).
- **Cloud dependency** for records. This is mitigated by local replicas: reads continue during an outage and writes are queued. The cost is eventual consistency between sites, handled by keeping a single primary per record and conflict-free logs.
- **Larger attack surface.** Mitigated by per-device certificates and least-privilege API scopes. The security ranking (rank 1 for the partner link and tablets) justifies the extra complexity.
- **Why not REDESIGN?** Every measurement behind the original choices still holds: the 31× system-call cost, the 20× IPC cost, the 100× process/thread gap, the polling-vs-interrupt result and the SMP scaling. The new requirements add environments and change the records topology; they do not invalidate the existing ones. **MODIFY** is the evidence-supported decision.
