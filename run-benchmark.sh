#!/bin/bash

QEMU_PATH="./build.qemu/i386-linux-user/qemu-i386";
BENCHMARK_PATH="./benchmark/automotive";

testcase=( \
    "basicmath/basicmath_large" \
    "bitcount/bitcnts 10000000" \
    "qsort/qsort_large $BENCHMARK_PATH/qsort/input_large.dat" \
    "susan/susan $BENCHMARK_PATH/susan/input_large.pgm /dev/null -s"
);


for test in "${testcase[@]}";
do
    cmd="$QEMU_PATH $BENCHMARK_PATH/$test";
    echo -e $cmd;
    time $cmd 1>/dev/null;
done

