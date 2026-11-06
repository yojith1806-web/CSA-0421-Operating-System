#!/bin/sh
# Task 4 evidence: how the OS protects one patient's record from another
# user's programs (file permissions = protection service; users = security).
# Must be run with sudo (it creates two test users).
set -u
D=/srv/records
useradd -M nurse_ward3 2>/dev/null; useradd -M clerk_billing 2>/dev/null
mkdir -p $D
echo "Patient P-1042: BP 118/76, allergy: penicillin" > $D/P-1042.rec
chown nurse_ward3:nurse_ward3 $D/P-1042.rec
chmod 600 $D/P-1042.rec                  # owner read/write only
ls -l $D/P-1042.rec
echo "-- nurse_ward3 (owner) reads the record:"
su -s /bin/sh nurse_ward3 -c "cat $D/P-1042.rec"
echo "-- clerk_billing (another user's program) tries to read it:"
su -s /bin/sh clerk_billing -c "cat $D/P-1042.rec" 2>&1
echo "exit status: $?"
echo "-- the kernel refuses inside open(): strace of the attempt"
su -s /bin/sh clerk_billing -c "strace -e trace=openat cat $D/P-1042.rec" 2>&1 | grep P-1042
rm -rf $D; userdel nurse_ward3; userdel clerk_billing
