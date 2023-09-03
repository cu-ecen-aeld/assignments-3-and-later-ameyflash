#!/bin/bash

if [ $# -lt 2 ]; then
    echo "Error : Arguments not specified correctly!!!"
    exit 1
fi

writefile=$1
writestr=$2
n=0

#echo $writefile
#echo $writestr

# Set comma as delimiter
IFS='/'
read -a strarr <<< "$writefile"

# Print each value of the array by using loop
for (( n=0; n < ${#strarr[*]}-1; n++))
do
  writedir+="${strarr[n]}/"
  #writefile+="${strarr[n]}/"
done

#echo writedir         --------       "$writedir"
#echo writefile        --------       "$writefile"

mkdir -p "$writedir"

if [ -e "$writefile" ]; then
    echo "File exists"
    echo $writestr > "$writefile"
else
    touch "$writefile" #2> /dev/null
    if [ -e "$writefile" ]; then
        echo $writestr > "$writefile"
    else
        echo "File not found"
        exit 1
    fi
fi