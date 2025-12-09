#!/bin/bash
# Script: format_fix.sh
# Purpose: Applies auto-fixes and formatting, then reports remaining unfixable issues.

echo "--- Running Clang-Tidy & Clang-Format for Fixing ---"

# --- Configuration (Adjust as needed) ---
FILE_EXTENSIONS='\.cpp|\.hpp|\.cxx|\.cc|\.h|\.hxx'
EXCLUDE_DIRS="build/|ImageLibrary/"
COMPILER_ARGS="-std=c++17 -Iinclude" # IMPORTANT: Must match the report script's args!

# --- Execution ---

# 1. Find all relevant files
echo "Finding C++ files..."
# Regex of directories to exclude (separated by pipes |)
# Tip: Include the trailing slash (e.g., 'lib/') to avoid accidental partial matches (like excluding 'library.cpp')
FILES_TO_CHECK=$(git ls-files | grep -v -E "$EXCLUDE_DIRS" | grep -E "$FILE_EXTENSIONS")

if [ -z "$FILES_TO_CHECK" ]; then
    echo "No C++ files found. Exiting."
    exit 0
fi

# 2. Run clang-tidy with --fix to apply auto-fixable errors/warnings.
echo "Running clang-tidy --fix to apply auto-fixes..."
# The --fix flag modifies the files in place.
# We suppress the normal output to focus on unfixable errors later.
echo "$FILES_TO_CHECK" | xargs -r -P 4 sh -c 'clang-tidy --quiet --fix "$@" -- '"$COMPILER_ARGS" _

# 3. Run clang-format for pure style fixes (faster and more reliable for formatting)
echo "Running clang-format -i for final formatting pass..."
# The -i flag modifies the files in place.
echo "$FILES_TO_CHECK" | xargs -r clang-format -i -style=file

echo "--- Reporting Unfixable Errors/Warnings ---"
# 4. Re-run clang-tidy *without* --fix to capture only the remaining, unfixable issues.
# We use the -header-filter='.*' flag to ensure checks run on headers too.
#
UNFIXABLE_OUTPUT=$(echo "$FILES_TO_CHECK" | xargs -r -P 4 sh -c 'clang-tidy "$@" -- '"$COMPILER_ARGS" _)

if [ -n "$UNFIXABLE_OUTPUT" ]; then
    echo "** The following errors/warnings could NOT be automatically fixed and require manual intervention: **"
    echo "$UNFIXABLE_OUTPUT"
    # Exit with an error code to signal unfixable issues in CI
    exit 1 
else
    echo "All auto-fixable issues were resolved and no further unfixable issues were found."
    exit 0
fi
