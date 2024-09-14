#!/bin/bash

# finder script for assignment 1
# Author: Shruthi Thallapally
# references: https://ryanstutorials.net/bash-scripting-tutorial/bash-if-statements.php
#	https://www.geeksforgeeks.org/how-to-count-files-in-directory-recursively-in-linux/
#	https://superuser.com/questions/339522/counting-total-number-of-matches-with-grep-instead-of-just-how-many-lines-match

#checking for 2 arguments and exiting with error if not present
if [ $# -ne 2 ] ; then
	echo "Two parameters are not specified"
	exit 1
fi

filesdir=$1
searchstr=$2


#check if filesdir is present,exit with error if not

if [ ! -d "$filesdir" ] ; then
	echo "$filesdir does not represent a directory"
	exit 1
fi

#counting the number of files in that directory
files=$(find "$filesdir" -type f | wc -l)

#counting number of matching lines
matching_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

echo "The number of files are $files and the number of matching lines are $matching_lines "




	

