#!/bin/bash
# Build and run every program of Assignment 2; outputs are saved in Qn/outputs/.
# Works on Linux (gcc) and macOS (clang). Usage: ./run_all.sh
set -u
cd "$(dirname "$0")"
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"
SEM=""
if [ "$(uname)" = "Darwin" ]; then CC="$CC -D_DARWIN_C_SOURCE"; SEM="-include Q5/macos_sem.h"; fi
for d in Q1 Q2 Q3 Q4 Q5 Q6; do mkdir -p $d/outputs; done
for f in Q1/fork_states Q1/ctxswitch Q1/sessions_bench Q2/rr Q2/rr_quantum Q2/sched_sim Q2/ageing \
         Q3/plates Q3/plates_proc Q4/pipe_ipc Q4/pipe_noclose Q4/ipc_bench Q6/banker Q6/banker_bench Q6/deadlock_demo; do
  $CC $f.c -o $f || exit 1
done
for f in Q5/bb Q5/bb_multi Q5/booking Q5/counter_bench; do $CC $SEM $f.c -o $f || exit 1; done
$CC -O1 Q5/peterson.c -o Q5/peterson

(cd Q1 && ./fork_states > outputs/fork_states.txt && ./ctxswitch > outputs/ctxswitch.txt && ./sessions_bench > outputs/sessions_bench.txt)
(cd Q2 && ./rr > outputs/rr.txt && for q in 1 3 5 20; do ./rr_quantum $q; done > outputs/rr_quantum.txt && ./sched_sim > outputs/sched_sim.txt && ./ageing > outputs/ageing.txt)
(cd Q3 && ./plates > outputs/plates.txt && for p in 4 8 16; do ./plates_proc $p; done > outputs/plates_proc.txt)
(cd Q4 && ./pipe_ipc > outputs/pipe_ipc.txt && { perl -e 'alarm 3; exec @ARGV' ./pipe_noclose; echo "[killed after 3 s, status $?]"; } > outputs/pipe_noclose.txt 2>&1; pkill -f pipe_noclose; ./ipc_bench > outputs/ipc_bench.txt)
(cd Q5 && ./bb > outputs/bb.txt && for i in $(seq 100); do ./bb; done | sort | uniq -c > outputs/bb_100runs.txt \
   && { ./bb_multi 1 1; ./bb_multi 4 4; ./bb_multi 4 4 nomutex; } > outputs/bb_multi.txt \
   && ./booking 8 > outputs/booking.txt && ./counter_bench > outputs/counter_bench.txt && ./peterson > outputs/peterson.txt)
(cd Q6 && ./banker > outputs/banker.txt && ./banker_bench > outputs/banker_bench.txt && ./deadlock_demo > outputs/deadlock_demo.txt)
echo "done - see Q*/outputs/"
