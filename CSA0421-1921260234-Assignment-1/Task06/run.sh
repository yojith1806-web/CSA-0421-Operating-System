#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC copy_file.c -o copy_file
$CC copy_cli.c -o copy_cli
$CC copy_bench.c -o copy_bench
$CC mmap_copy.c -o mmap_copy
$CC -O1 syscall_cost.c -o syscall_cost
./copy_file | tee outputs/copy_file.txt
strace -c -e trace=read,write ./copy_file 2>&1 | tee outputs/copy_file_strace_c.txt
./syscall_cost | tee outputs/syscall_cost.txt
./copy_bench backup_src.dat 8 | tee outputs/copy_bench.txt
{
for b in 1 512 4096 65536; do
  echo "## read()/write() buffer $b on 8 MB file"
  strace -c -e trace=read,write ./copy_cli backup_src.dat dst.dat $b 2>&1 | grep -E "buffer|read|write|total"
done
echo "## mmap copy on 8 MB file"
strace -c ./mmap_copy backup_src.dat dst.dat 2>&1 | tail -n 25
} | tee outputs/strace_buffer_sizes.txt
rm -f backup_src.dat dst.dat
