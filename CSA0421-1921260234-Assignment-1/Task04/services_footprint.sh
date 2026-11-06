#!/bin/sh
# Task 4 evidence: what a full general-purpose OS (Ubuntu server, no desktop)
# runs and uses at idle. This is the baseline for "every service enabled".
echo "== distribution / kernel =="
lsb_release -ds 2>/dev/null; uname -srm
echo "== running processes (all) =="
ps -e --no-headers | wc -l
echo "== kernel threads =="
ps -e --no-headers -o comm | grep -c '^\(kworker\|ksoftirqd\|migration\|rcu\|kthreadd\|cpuhp\|idle_inject\)'
echo "== active systemd services =="
systemctl list-units --type=service --state=running --no-legend 2>/dev/null | wc -l
echo "== loaded kernel modules =="
lsmod | tail -n +2 | wc -l
echo "== memory in use at idle (MB) =="
free -m | awk '/Mem:/ {print "total="$2" used="$3" buff/cache="$6}'
echo "== size of kernel image + modules on disk =="
du -sh /boot/vmlinuz-$(uname -r) /lib/modules/$(uname -r) 2>/dev/null
echo "== installed packages =="
dpkg -l | grep -c '^ii'
echo "== boot time =="
systemd-analyze 2>/dev/null | head -1
echo "== top 8 services by memory (RSS MB) =="
ps -eo rss,comm --sort=-rss --no-headers | head -8 | awk '{printf "%7.1f  %s\n", $1/1024, $2}'
