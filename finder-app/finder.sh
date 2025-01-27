#!/bin/sh
# Two (2) arguments must be provided.
# Provide a path to a directory on the filesystem as the first argument
# and a text string as the second argument.
# Usage:
# ./finder.sh <path_to_directory> <string>
# Author: Panagiotis Mousouliotis

if [ $# -ne 2 ]
then
    exit 1
fi

FILESDIR=$1
SEARCHSTR=$2

if [ ! -d "$FILESDIR" ]
then
    echo "Directory ${FILESDIR} does not exist on the filesystem."
    exit 1
fi

NUMFILES=$(find -L "$FILESDIR" -type f | wc -l)
NUMMATCHES=$(grep -s -r "$SEARCHSTR" "$FILESDIR" | wc -l)

echo "The number of files are ${NUMFILES} and the number of matching lines are ${NUMMATCHES}"

exit 0