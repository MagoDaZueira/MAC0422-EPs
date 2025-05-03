#!/bin/bash

PROGRAM="./ep2"
ARGS="$1 $2 $3"
OUTFILE=$4

echo "tempo_segundos,memoria_kbytes" > "$OUTFILE"

for i in {1..30}; do
    /usr/bin/time -v $PROGRAM $ARGS 2> temp.txt 1> /dev/null
    tempo=$(grep "Elapsed (wall clock) time" temp.txt | awk '{print $8}' | awk -F: '{print ($1 * 60) + $2}')
    memoria=$(grep "Maximum resident set size" temp.txt | awk '{print $6}')
    echo "$tempo,$memoria" >> "$OUTFILE"
done
