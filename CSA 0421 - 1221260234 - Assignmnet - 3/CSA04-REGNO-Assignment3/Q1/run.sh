#!/bin/bash
# Q1 experiments. Usage: bash run.sh   (output -> output.txt)
cd "$(dirname "$0")"; exec > output.txt 2>&1
set -x
gcc -std=c11 -Wall -Wextra -fPIC -shared -o libenc.so libenc.c
gcc -std=c11 -Wall -Wextra -fPIC -shared -o libenc_v2.so libenc_v2.c
gcc -std=c11 -Wall -Wextra -c libenc.c -o libenc_static.o && ar rcs libenc.a libenc_static.o
gcc -std=c11 -Wall -Wextra -D_GNU_SOURCE -o bank_dynamic bank.c -L. -lenc -Wl,-rpath,'$ORIGIN'
gcc -std=c11 -Wall -Wextra -D_GNU_SOURCE -static -o bank_static bank.c libenc.a
gcc -std=c11 -Wall -Wextra -o bank_dl bank_dl.c -ldl
set +x
echo "=== 1. Sizes (bytes) ==="; ls -l bank_static bank_dynamic libenc.so | awk '{print $5, $9}'
echo "=== 2. ldd ==="; echo "-- dynamic:"; ldd bank_dynamic; echo "-- static:"; ldd bank_static
echo "=== 3. Run both ==="; ./bank_static; ./bank_dynamic
echo "=== 4. Load addresses (/proc/self/maps) of dynamic build ==="; ./bank_dynamic maps
echo "=== 5. dlopen/dlsym demo ==="; ./bank_dl noenc; ./bank_dl encrypt ./libenc.so
echo "=== 6. Start-up time: 2000 runs each ==="
t(){ local s=$(date +%s.%N); for i in $(seq 2000); do "$@" >/dev/null; done; local e=$(date +%s.%N); echo "$1: $(echo "($e-$s)/2000*1000" | bc -l | cut -c1-6) ms per run"; }
t ./bank_static; t ./bank_dynamic; t ./bank_dl noenc
echo "=== 7. Memory (max RSS KB) ==="
for p in ./bank_static ./bank_dynamic; do ../tools/runstat $p >/dev/null; done 2>&1
echo "=== 8. Replace library WITHOUT recompiling ==="
echo "before:"; ./bank_dynamic | head -2; cp libenc.so libenc_v1_backup.so; cp libenc_v2.so libenc.so
echo "after swapping libenc.so with the fixed v2:"; ./bank_dynamic | head -2
echo "static build is unchanged (still old code):"; ./bank_static | head -2
cp libenc_v1_backup.so libenc.so
echo "=== 9. Two copies: which pages are shared? (Pss vs Rss from /proc/PID/smaps) ==="
./bank_dynamic wait >/dev/null & P1=$!; ./bank_dynamic wait >/dev/null & P2=$!; sleep 1
for P in $P1 $P2; do echo "PID $P"; awk '/^[0-9a-f]+-[0-9a-f]+ /{n=$6} /^(Rss|Pss|Shared_Clean|Private_Dirty):/{a[n" "$1]+=$2} END{for(k in a) if(k ~ /libc.so|libenc|bank_dynamic/) print "  "k, a[k]" kB"}' /proc/$P/smaps | sort; done
wait
echo "=== 10. Missing library ==="; mv libenc.so libenc.so.hidden
./bank_dynamic; echo "exit code: $?"; ./bank_dl encrypt ./libenc.so; echo "exit code (dlopen version handles it): $?"
mv libenc.so.hidden libenc.so
echo "=== 11. PIE / PIC check ==="; file bank_dynamic | cut -c1-120; readelf -h bank_dynamic | grep Type
