#!/bin/sh

echo "Checking for arguments..."
if [ $# -lt 2 ]; then
    echo "Error : Arguments not specified correctly!!!"
    exit 1
else
    echo "Pass"
fi

filesdir=$1
searchstr=$2

filecount=0
linecount=0

echo "Checking if file exists..."
if [ -e $filesdir ]; then
    echo "Pass"
else
    echo "File not found"
    exit 1
fi

filecount=$(find $filesdir -type f | wc -l)

linecount=$(grep -r $searchstr $filesdir | wc -l)

echo "The number of files are $filecount and the number of matching lines are $linecount"