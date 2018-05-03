#!/bin/sh
SUM1=0
SUM2=0
SUM3=0
N=50
n=0
while [ $n -lt $N ]; do
    SUM1=$(($SUM1 + $(curl http://student01.cse.nd.edu:9005/text/1KB | tail -n 1 | cut -d ' ' -f 7)))
    SUM2=$(($SUM2 + $(curl http://student01.cse.nd.edu:9005/text/1MB | tail -n 1 | cut -d ' ' -f 7)))
    SUM3=$(($SUM3 + $(curl http://student01.cse.nd.edu:9005/text/1GB | tail -n 1 | cut -d ' ' -f 7)))
    n=$(($n + 1))
done

AVG1=$(($SUM1 / $N))
AVG2=$(($SUM2 / $N))
AVG3=$(($SUM3 / $N))

echo $AVG1
echo $AVG2
echo $AVG3
