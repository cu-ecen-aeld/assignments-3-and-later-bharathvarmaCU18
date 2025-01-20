#!/bin/bash

# write file(arg 1)  and string(arg 2) to be written in the file

WRITEFILE=$1
WRITESTR=$2



# If both the required arguments are not specified, exit
if [ -z $WRITEFILE ] || [ -z $WRITESTR ]
then
    echo "Required Arguments not specified"
    exit 1
fi

# Extract the Write File Directory
WRITEFILEDIR=$(dirname $WRITEFILE)

# If the directory is not present, create one. If it fails, exit!
if [ ! -d $WRITEFILEDIR ]
then
    mkdir -p $WRITEFILEDIR
    if [ ! $? ]
    then
        echo "Couldn't create the Directory Path for the Provided WriteFile"
	exit 1
    fi
fi


# write the string to the specified file by WRITEFILE path using echo
echo $WRITESTR > $WRITEFILE

if [ ! $? ]
then
    echo "File could not be written!"
    exit 1
fi
