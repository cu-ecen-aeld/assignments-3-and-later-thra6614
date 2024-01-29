#!/bin/sh

file_name=$1
input_str=$2
argc=$#

if [ $argc -ne 2 ]; then #fail
	echo "ERROR: Invalid Number of Arguments. Expected: 2, Received: $argc"
	exit 1
fi

file_dir=$(dirname ${file_name}) #directory without file name
mkdir -p "$file_dir" #create path if doesn't exist
echo "$input_str" > "$file_name" #create/overwrite file

if [ $? -eq 1 ]; then 
	"ERROR: Invalid Directory: $file_dir" #fail to create/overwrite file and input string
	exit 1
fi
