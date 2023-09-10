#!/bin/sh

echo "Checking for arguments..."
if [ $# -lt 2 ]; then
    echo "Error : Arguments not specified correctly!!!"
    echo "Usage: $ ./finder.sh <filedir> <searchstr>"
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

# count number of files in the directory
filecount=$(find $filesdir -type f | wc -l)

# count number of lines with given word in the directory
linecount=$(grep -r $searchstr $filesdir | wc -l)

echo "The number of files are $filecount and the number of matching lines are $linecount"