#!/bin/bash
set -e

mkdir -p build traces

MEM=src/Memory/Memory.cpp
FLAGS="-std=c++17 -O2 -I src"
SIZES="2 4 8 16 32 64"
TAGS="2 4"
WORK="blur int_matrix float_matrix random_float scan_zero scan_ramp scan_stride scan_random"

echo "[SH] 1. Compiling and running Tracer..."
clang++ $FLAGS src/Trace/Tracer.cpp -o build/tracer

echo "[SH] 2. Build CacheSim..."
for t in $TAGS; do
  for s in $SIZES; do
    clang++ $FLAGS -DSETS=$s -DTAGF=$t src/main.cpp src/Cache/Cache.cpp $MEM -o build/cachesim_${t}_${s}
  done
done

echo "Workload,Sets,TagFactor,Algorithm,Hit_Rate,Avg_Size,Ratio,Pct_le32,Pct_le16,Eff_Capacity" > results.csv

echo "[SH] 3. Tracing and simulating..."
for w in $WORK; do
  echo " workload: $w"
  ./build/tracer $w traces/trace_$w.txt

  for t in $TAGS; do
    for s in $SIZES; do
      ./build/cachesim_${t}_${s} traces/trace_$w.txt $w > /dev/null
    done
  done
done
echo "[SH] 4. Done!"