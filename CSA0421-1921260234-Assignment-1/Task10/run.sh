#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

$CC create_bench.c -o create_bench
$CC ../Task06/copy_cli.c -o copy_cli
head -c 16777216 /dev/urandom > big.dat
{
echo "## strace -c system-call counts for a 16 MB copy"
printf "%-10s %12s %12s\n" buffer "read() calls" "write() calls"
for b in 1 64 512 4096 65536 1048576; do
  strace -c -e trace=read,write -o sc.txt ./copy_cli big.dat dst.dat $b 2>/dev/null
  r=$(awk '$NF=="read"{print $4}' sc.txt); w=$(awk '$NF=="write"{print $4}' sc.txt)
  printf "%-10s %12s %12s\n" $b $r $w
done
echo
echo "## /usr/bin/time: user vs kernel (system) time for the same copy"
printf "%-10s %8s %8s %8s\n" buffer "real(s)" "user(s)" "sys(s)"
for b in 1 64 512 4096 65536 1048576; do
  /usr/bin/time -f "%e %U %S" -o tm.txt ./copy_cli big.dat dst.dat $b 2>/dev/null
  printf "%-10s %8s %8s %8s\n" $b $(cat tm.txt)
done
} | tee outputs/copy_measurements.txt
rm -f big.dat dst.dat sc.txt tm.txt
{
for r in 1 2 3; do ./create_bench proc 1000; ./create_bench thread 1000; done
echo
echo "## /usr/bin/time -v (max resident set of the creating process)"
/usr/bin/time -f "processes: elapsed %e s, user %U s, sys %S s, maxRSS %M KB" ./create_bench proc 1000
/usr/bin/time -f "threads:   elapsed %e s, user %U s, sys %S s, maxRSS %M KB" ./create_bench thread 1000
} 2>&1 | tee outputs/create_bench.txt
