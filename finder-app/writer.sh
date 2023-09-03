#!/bin/bash

echo "Checking for arguments..."
if [ $# -lt 2 ]; then
    echo "Error : Arguments not specified correctly!!!"
    exit 1
else
    echo "Pass"
fi

writefile=$1
writestr=$2

# Set / as delimiter to split string
IFS='/'
read -a strarr <<< "$writefile"

# To create directory if it doesn't exists
for (( n=0; n < ${#strarr[*]}-1; n++))
do
  writedir+="${strarr[n]}/"
done

mkdir -p "$writedir"

echo "Writing to file..."
if [ -e "$writefile" ]; then
    echo "Pass"
    echo $writestr > "$writefile"
else
    touch "$writefile" 2> /dev/null
    if [ -e "$writefile" ]; then
        echo "Pass"
        echo $writestr > "$writefile"
    else
        echo "File not found"
        exit 1
    fi
fi