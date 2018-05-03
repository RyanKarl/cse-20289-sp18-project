#!/bin/sh
SUM1=0
SUM2=0
SUM3=0
N=2
n=0
while [ $n -lt $N ]; do
    SUM1=$(echo $SUM1 + $(curl http://student02.cse.nd.edu:9005/text/1KB | tail -n 1 | cut -d ' ' -f 7) | bc)
    SUM2=$(echo $SUM2 + $(curl http://student02.cse.nd.edu:9005/text/1MB | tail -n 1 | cut -d ' ' -f 7) | bc)
    SUM3=$(echo $SUM3 + $(curl http://student02.cse.nd.edu:9005/text/1GB | tail -n 1 | cut -d ' ' -f 7) | bc)
    n=$(($n + 1))
done

AVG1=$(echo $SUM1 / $N | bc -l)
AVG2=$(echo $SUM2 / $N| bc -l)
AVG3=$(echo $SUM3 / $N | bc -l)

echo $AVG1
echo $AVG2
echo $AVG3

curl http://student02.cse.nd.edu:9005/text/1KB 2 > &file
cat file | tail -n 1
rm file
