#! /bin/bash
file_dir=$1
search_str=$2
argc=$#

if [ $argc -eq 2 ] #check if valid argument count
then
	if [ -d "$file_dir" ] #check if valid directory
	then
		count=$(grep -r "$search_str" $file_dir | wc -l)
		file_cnt=$(find "$file_dir" | wc -l)
		file_cnt=$(($file_cnt-1)) #
		echo "The number of files are $file_cnt and the number of matching lines are $count"
		exit 0
	else
		echo "ERROR: $file_dir is not a valid directory on the filesystem"
		exit 1
	fi
else
	echo "ERROR: Invalid Number of Arguments. Expected 2, Received $argc"
	exit 1
fi
