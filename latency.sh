#!/bin/sh

# static files
# $1 is the file name, e.g. 1KB, 1MB, etc
SUM1=0
SUM2=0
SUM3=0
N=50
n=0
while [ $n -lt $N ]; do
    SUM1=$(($SUM1 + $(./thor.py -r 1 -p 5 http://student01.cse.nd.edu:9005/text/lyrics.txt | tail -n 1 | cut -d ' ' -f 5)))
    SUM2=$(($SUM2 + $(./thor.py -r 1 -p 5 http://student01.cse.nd.edu:9005/text/ | tail -n 1 | cut -d ' ' -f 5)))
    SUM3=$(($SUM3 + $(./thor.py -r 1 -p 5 http://student01.cse.nd.edu:9005/scripts/cowsay | tail -n 1 | cut -d ' ' -f 5)))
    n=$(($n + 1))
done

AVG1=$(($SUM1 / $N))
AVG2=$(($SUM2 / $N))
AVG3=$(($SUM3 / $N))

echo $AVG1
echo $AVG2
echo $AVG3


#./thor.py -r 50 -p 5 http://student01.cse.nd.edu:9005/text/$1 | tail -n 1 | cut -d ' ' -f 5 | cat
#./thor.py -r 50 -p 5 http://student01.cse.nd.edu:9005/text/ | tail -n 1 | cut -d ' ' -f 5 | cat
#./thor.py -r 50 -p 5 http://student01.cse.nd.edu:9005/scripts/cowsay | tail -n 1 | cut -d ' ' -f 5 | cat
