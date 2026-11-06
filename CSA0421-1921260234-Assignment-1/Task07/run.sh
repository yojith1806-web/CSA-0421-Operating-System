#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC shell_jobs.c -o shell_jobs
$CC shell_jobs_dup.c -o shell_jobs_dup
$CC mysh.c -o mysh
$CC zombie_orphan.c -o zombie_orphan
$CC spawn_bench.c -o spawn_bench
./shell_jobs | tee outputs/shell_jobs.txt
echo "## original program, stdout redirected to a file" > outputs/redirect_compare.txt
./shell_jobs > tmp.txt; cat tmp.txt >> outputs/redirect_compare.txt
echo "## variant: 2nd fflush removed, _exit(3) -> exit(3), stdout redirected to a file" >> outputs/redirect_compare.txt
./shell_jobs_dup > tmp.txt; cat tmp.txt >> outputs/redirect_compare.txt
echo "## variant on a terminal-like (line-buffered) stdout: script -qc" >> outputs/redirect_compare.txt
script -qc ./shell_jobs_dup /dev/null | tr -d '\r' >> outputs/redirect_compare.txt
rm -f tmp.txt; cat outputs/redirect_compare.txt
printf 'expr 6 * 7\nls /etc/hostname\nls /no/such/file\nnosuchcmd\nsh -c "exit 5"\nexit\n' | ./mysh | tee outputs/mysh.txt
strace -f -e trace=clone,clone3,execve,wait4,exit_group ./shell_jobs 2>&1 | tee outputs/strace_shell_jobs.txt
./zombie_orphan | tee outputs/zombie_orphan.txt
./spawn_bench 1000 | tee outputs/spawn_bench.txt
