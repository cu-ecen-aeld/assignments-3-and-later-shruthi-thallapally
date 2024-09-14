#!/bin/bash

# finder script for assignment 1
# Author: Shruthi Thallapally
# references: https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php
#	      https://stackoverflow.com/questions/6121091/how-to-extract-directory-path-from-file-path
#	      https://stackoverflow.com/questions/793858/how-to-mkdir-only-if-a-directory-does-not-already-exist

#checking for 2 arguments and exiting with error if not present
if [ $# -ne 2 ] ; then
	echo "Two parameters are not specified"
	exit 1
fi

writefile=$1
writestr=$2

#extracting file path from the input argument
file_path=$(dirname "$writefile")

if [ ! -d "$file_path" ] ; then
	mkdir -p "$file_path"
	if [ $? -ne 0 ] ; then
		echo "Error: could not create file path"
		exit 1
	fi
fi

#creating file if it doesn't exit. if exists, changing the timestamp
touch "$writefile"
if [ $? -ne 0 ] ; then
	echo "Error: could not create file "
	exit 1
fi

#writing the string to the file
echo "$writestr" > "$writefile"
 
