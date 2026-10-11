#!/bin/sh
# Compile the whole sketch for the host with AddressSanitizer + UBSan and fuzz every game with scripted touches.
cd "$(dirname "$0")/../.." || exit 1
g++ -std=gnu++11 -O1 -g -fsanitize=address,undefined -fno-sanitize-recover=undefined -Wall -Wno-unused-function \
    -Wno-unused-variable -Itools/sim -Ifirmware/CydGameHub -include Arduino.h tools/sim/sim_main.cpp -o /tmp/cydsim || exit 1
timeout 600 /tmp/cydsim "${1:-4000}"
