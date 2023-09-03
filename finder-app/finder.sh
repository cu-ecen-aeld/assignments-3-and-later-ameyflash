#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Error : Arguments not specified correctly!!!"
    exit 1
fi

filesdir=$1
searchstr=$2

filecount=0
linecount=0

echo "Finder Script"
echo $filesdir
echo $searchstr

if [ -e $filesdir ]; then
    echo "File exists"
else
    echo "File not found"
    exit 1
fi

filecount=$(find $filesdir -type f | wc -l)

for f in $filesdir
do
    linecount=$(grep -r $searchstr $filesdir | wc -l)
done

echo "The number of files are $filecount and the number of matching lines are $linecount"