#!/bin/sh

echo "Checking for arguments..."
if [ $# -lt 2 ]; then
    echo "Error : Arguments not specified correctly!!!"
    echo "Usage: $ ./writer.sh <writefile> <writestr>"
    exit 1
else
    echo "Pass"
fi

writefile=$1
writestr=$2
writedir=$(dirname "$writefile")

# Create directory if it doesn't exists
mkdir -p "$writedir"

echo "Writing to file..."
echo $writestr > "$writefile"
if [ -e "$writefile" ]; then
    echo "Pass"
else
    echo "File not found"
    exit 1
fi