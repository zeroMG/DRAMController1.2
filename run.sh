#!/bin/bash

PROGRAM=automated
#4KB to 2MB
SIZES="4096 16384 32768 65536 131072 262144 524288 1048576 2097152"
#SIZES="4096"
REPETITIONS=2

for i in ${SIZES};
do
    echo "Starting DMA for size=$i"
    for rep in `seq 1 ${REPETITIONS}`;
    do        
        ./$PROGRAM $i | grep duration | grep -Eo '[0-9]*'
    done
done
