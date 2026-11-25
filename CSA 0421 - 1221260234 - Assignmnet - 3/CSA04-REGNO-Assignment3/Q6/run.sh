#!/bin/bash
cd "$(dirname "$0")"; exec > output.txt 2>&1
echo "=== lsblk ==="; lsblk 2>&1 || cat /proc/partitions
echo; echo "=== fdisk -l ==="; (fdisk -l 2>&1 || { echo "(fdisk not installed in this sandbox; /proc/partitions instead)"; cat /proc/partitions; }) | head -20
echo; echo "=== swap ==="; cat /proc/swaps; swapon --show 2>&1; free -h
echo; echo "=== mounted filesystems ==="; df -hT / /tmp 2>&1; echo; mount | grep -E ' / | /boot' | head
echo; echo "=== Boot method / firmware ==="; [ -d /sys/firmware/efi ] && echo "UEFI" || echo "BIOS or container (no /sys/firmware/efi)"
echo; echo "=== Try to create and enable a 64 MB swap file (demonstration) ==="
dd if=/dev/zero of=/tmp/swapfile bs=1M count=64 status=none && chmod 600 /tmp/swapfile && mkswap /tmp/swapfile 2>&1 && swapon /tmp/swapfile 2>&1 && { echo "swapon OK"; swapon --show; swapoff /tmp/swapfile; } ; rm -f /tmp/swapfile
echo; echo "=== Read speed (only the VM's virtual disk is available; no separate SSD/HDD to compare) ==="
gcc -std=c11 -O2 -Wall -Wextra -o readspeed readspeed.c && ./readspeed
echo; echo "=== Parity cost ==="; gcc -std=c11 -O2 -Wall -Wextra -o raid_parity raid_parity.c && ./raid_parity
echo; python3 analysis.py
