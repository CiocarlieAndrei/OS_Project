#!/bin/bash

# Check if the file exists
file_path="$1"
if [ ! -f "$file_path" ]; then
    echo "File does not exist or is not a regular file: $file_path"
    exit 2
fi

# Get original permissions
original_permissions=$(stat -c "%a" "$file_path")

# Change file permissions to allow reading
chmod +r "$file_path"

# Get the number of lines, words, and characters in the file
num_lines=$(wc -l < "$file_path")
num_words=$(wc -w < "$file_path")
num_chars=$(wc -m < "$file_path")

# Condition 1: Check if the file meets the specified conditions
if [ "$num_lines" -lt 3 ] && [ "$num_words" -gt 1000 ] && [ "$num_chars" -gt 2000 ]; then
    # Condition 2: Check for non-ASCII characters or keywords
    if grep -qP '[^\x00-\x7F]' "$file_path" || grep -qE 'risk|corrupted' "$file_path"; then
        echo "$file_path"
        chmod "$original_permissions" "$file_path"
        exit 1
    else
        echo "SAFE"
        chmod "$original_permissions" "$file_path"
        exit 0
    fi
else
    echo "SAFE"
    chmod "$original_permissions" "$file_path"
    exit 0
fi


