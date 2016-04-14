#!/bin/bash

QEMU_PATH="./build.qemu/i386-linux-user/qemu-i386";
COREMARK_PATH="./benchmark/coremark_v1.0";

testcase=( \
    "coremark.exe  0 0 0x66 0 7 1 2000"  \
    "coremark.exe  0x3415 0x3415 0x66 0 7 1 2000"  \
);


for test in "${testcase[@]}";
do
    cmd="$QEMU_PATH $COREMARK_PATH/$test";
    echo -e $cmd;
    time $cmd;
done

