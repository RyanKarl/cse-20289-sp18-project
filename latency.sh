#!/bin/sh

# static files
# $1 is the file name, e.g. 1KB, 1MB, etc
SUM1=0
SUM2=0
SUM3=0
N=50
n=0
while [ $n -lt $N ]; do
    SUM1=$(echo $SUM1 + $(./thor.py -r 1 -p 5 http://student02.cse.nd.edu:9005/text/file.txt | head -n 1 | cut -d ' ' -f 5) | bc)
    SUM2=$(echo $SUM2 + $(./thor.py -r 1 -p 5 http://student02.cse.nd.edu:9005/text/ | head -n 1 | cut -d ' ' -f 5) | bc)
    SUM3=$(echo $SUM3 + $(./thor.py -r 1 -p 5 http://student02.cse.nd.edu:9005/scripts/cowsay | head -n 1 | cut -d ' ' -f 5)| bc)
    n=$(($n + 1))
done

AVG1=$(echo $SUM1 / $N | bc -l)
AVG2=$(echo $SUM2 / $N | bc -l)
AVG3=$(echo $SUM3 / $N | bc -l)

echo $AVG1
echo $AVG2
echo $AVG3


#./thor.py -r 50 -p 5 http://student01.cse.nd.edu:9005/text/$1 | tail -n 1 | cut -d ' ' -f 5 | cat
#./thor.py -r 50 -p 5 http://student01.cse.nd.edu:9005/text/ | tail -n 1 | cut -d ' ' -f 5 | cat
#./thor.py -r 50 -p 5 http://student01.cse.nd.edu:9005/scripts/cowsay | tail -n 1 | cut -d ' ' -f 5 | cat
