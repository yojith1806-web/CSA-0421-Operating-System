#!/bin/bash
# Compile and run every program for this task on Linux; outputs go to outputs/.
set -u
cd "$(dirname "$0")"
mkdir -p outputs
CC="gcc -std=c11 -Wall -Wextra -O2 -pthread"

sh services_footprint.sh 2>&1 | tee outputs/services_footprint.txt
sudo sh record_protection.sh 2>&1 | tee outputs/record_protection.txt
