#!/bin/bash

# directory(arg 1)  and string(arg 2) be searched

FILESDIR=$1
SEARCHSTR=$2



# If both the required arguments are not specified, exit
if [ -z $FILESDIR ] || [ -z $SEARCHSTR ]
then
    echo "Required Arguments not specified"
    exit 1
fi

# If the first argument specified is not a directory
if [ ! -d $FILESDIR ]
then
    echo "$FILESDIR is not a directory"
    exit 1
fi


# Search the FILESDIR recursively and find the matching files and pipe it to wc to print the number of lines. This gives file count
NUMFILESMATCH=$(grep -lr $SEARCHSTR $FILESDIR | wc -l)

# Search the FILESDIR recursively and find all the matching strings line by line and pipe to wc to print the number of line matches. This gives line count
NUMLINESMATCH=$(grep -r -a  $SEARCHSTR $FILESDIR | wc -l)


echo "The number of files are $NUMFILESMATCH and the number of matching lines are $NUMLINESMATCH"


