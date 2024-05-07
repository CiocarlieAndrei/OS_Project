#!/bin/bash

# Check if the file exists
file_path="$1"
if [ ! -f "$file_path" ]; then
    echo "File does not exist or is not a regular file: $file_path"
    exit 2;
fi

# Get original permissions
original_permissions=$(stat -c "%a" "$file_path")

# Change file permissions to allow reading
chmod +r "$file_path"

# Check for non-ASCII characters
if grep -qP '[^\x00-\x7F]' "$file_path"; then
    echo "File contains non-ASCII characters: $file_path"
    chmod "$original_permissions" "$file_path"
    exit 1;
fi

# Check for keywords "risk" or "corrupted"
if grep -qE 'risk|corrupted' "$file_path"; then
    echo "File contains keyword 'risk' or 'corrupted': $file_path"
    chmod "$original_permissions" "$file_path"
    exit 1;
fi

# Change permissions back to original state
chmod "$original_permissions" "$file_path"
exit 0;

