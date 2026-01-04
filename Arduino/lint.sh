#!/bin/bash
# Script: lint_report.sh
# Purpose: Prints all clang-tidy errors and warnings to stdout without applying fixes.

echo "--- Running Clang-Tidy linter ---"

# --- Configuration (Adjust as needed) ---
FILE_EXTENSIONS='\.cc|\.cpp|\.cxx|\.ino|\.h|\.hh|\.hpp|\.hxx'
EXCLUDE_DIRS="build/|ImageLibrary/"
COMPILER_ARGS="-std=c++17 -Iinclude"

# --- Execution ---

# 1. Find all relevant files not ignored by Git
echo "Finding C++ files..."
# Regex of directories to exclude (separated by pipes |)
# Tip: Include the trailing slash (e.g., 'lib/') to avoid accidental partial matches (like excluding 'library.cpp')
FILES_TO_CHECK=$(git ls-files | grep -v -E "$EXCLUDE_DIRS" | grep -E "$FILE_EXTENSIONS")

if [ -z "$FILES_TO_CHECK" ]; then
    echo "No C++ files found. Exiting."
    exit 0
fi

# 2. Run clang-tidy
# -p . : Placeholder for the compilation database.
# --: Separates clang-tidy flags from compiler flags.
# -checks: Forces clang-tidy to use the checks defined in the .clang-tidy file.
# The script output is the full list of warnings/errors.
echo "$FILES_TO_CHECK" | xargs -r -P 4 sh -c 'clang-tidy "$@" -- '"$COMPILER_ARGS" _

EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then
    echo "--- Linting found issues. Please review the output above. ---"
else
    echo "--- Linting complete. No issues found. ---"
fi

exit $EXIT_CODE
