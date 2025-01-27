#!/bin/sh
# Two (2) arguments must be provided.
# Please provide a full path to a file (including filename) and
# a text string which will be written within this file as the second argument.
# Usage:
# ./writer.sh <path_to_file> <string>
# Author: Panagiotis Mousouliotis

if [ $# -ne 2 ]
then
    exit 1
fi

WRITEFILE=$1
WRITESTR=$2

mkdir -p "$(dirname "$WRITEFILE")"

touch "$WRITEFILE"

if [ ! -e "$WRITEFILE" ]
then
    echo "File ${WRITEFILE} could not be created."
    exit 1
else
    echo "$WRITESTR" > "$WRITEFILE"
fi

exit 0