#!/bin/bash

access=($(grep "LLC TOTAL" $1 | cut -d " " -f9))
miss=($(grep "LLC TOTAL" $1 | cut -d " " -f21))
instr=($(grep "CPU . cumulative" $1 | cut -d " " -f7))
ipc=($(grep "CPU . cumulative" $1 | cut -d " " -f5))
if [ -z "$miss" ]; then
    miss=($(grep "LLC TOTAL" $1 | cut -d " " -f22))
fi
if [ -z "$access" ]; then
    access=($(grep "LLC TOTAL" $1 | cut -d " " -f10))
fi
if [ -z "$miss" ]; then
    miss=($(grep "LLC TOTAL" $1 | cut -d " " -f23))
fi
if [ -z "$access" ]; then
    access=($(grep "LLC TOTAL" $1 | cut -d " " -f11))
fi
echo ${#miss[@]}
echo ${#access[@]}
for i in `seq 0 7`; do
    if [ "$i" -gt "3" ]; then
        echo "CPU" $((i-4))
        missrate=$(echo "${miss[i]}/${access[i]}*100" | bc -l)
        mpki=$(echo "${miss[i]}/${instr[i]}*1000" | bc -l)
        printf "Miss Rate: %0.3f\n" $missrate
        printf "MPKI: %0.3f\n" $mpki
    fi
done
