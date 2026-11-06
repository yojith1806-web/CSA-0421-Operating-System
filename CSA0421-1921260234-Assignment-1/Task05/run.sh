#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC read_demo.c -o read_demo
$CC raw_syscall.c -o raw_syscall
$CC stdio_vs_syscall.c -o stdio_vs_syscall
./read_demo patient_record.txt | tee outputs/read_demo.txt
strace -o outputs/strace_read_demo.txt ./read_demo patient_record.txt > /dev/null
strace -T -e trace=openat,read,write,close ./read_demo patient_record.txt 2>&1 >/dev/null | tail -n 12 | tee outputs/strace_read_calls.txt
./raw_syscall | tee outputs/raw_syscall.txt
strace -e trace=write ./raw_syscall 2>&1 >/dev/null | tee -a outputs/raw_syscall.txt
head -c 1048576 /dev/urandom > n1MB.dat
{
for b in 16 512 4096; do
  echo "## direct read()/write(), b=$b, n=1048576"
  strace -c -e trace=read,write ./stdio_vs_syscall sys n1MB.dat out.dat $b 2>&1 | grep -E "read|write|total|library"
  echo "## fread()/fwrite(), b=$b, n=1048576"
  strace -c -e trace=read,write ./stdio_vs_syscall stdio n1MB.dat out.dat $b 2>&1 | grep -E "read|write|total|library"
done
} | tee outputs/stdio_vs_syscall.txt
rm -f n1MB.dat out.dat
