#!/bin/bash

# Check if the required arguments were provided
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments required: <file path> <string to write>"
    exit 1
fi

writefile=$1
writestr=$2

# Create the directory if it doesn't exist
mkdir -p "$(dirname "$writefile")"

# Attempt to write to the file
if ! echo "$writestr" > "$writefile"; then
    echo "Error: Could not write to $writefile"
    exit 1
fi

exit 0
