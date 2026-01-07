#!/usr/bin/env bash
set -e

echo "--- Running Clang-Tidy & Clang-Format (ESP32) ---"

# -------- Sketch selection --------
SKETCH_NAME="${1:-QuantumDice}"
SKETCH_DIR="Arduino/$SKETCH_NAME"
BUILD_DIR="$SKETCH_DIR/build"

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "ERROR: compile_commands.json not found for $SKETCH_NAME"
    echo "Run Arduino/setup_arduino_esp32.sh [$SKETCH_NAME] first."
    exit 1
fi

# -------- Find files --------
FILES=$(git ls-files \
  | grep -E '\.(cpp|cxx|cc|hpp|h)$' \
  | grep -v -E 'build/|ImageLibrary/')

if [ -z "$FILES" ]; then
    echo "No C++ files found."
    exit 0
fi

# -------- Auto-fix --------
echo "Applying clang-tidy auto-fixes..."
echo "$FILES" | xargs -r -P 4 \
  clang-tidy -p "$BUILD_DIR" --quiet --fix

echo "Running clang-format..."
echo "$FILES" | xargs -r \
  clang-format -i -style=file

# -------- Report remaining issues --------
echo "--- Reporting remaining issues ---"
sh scripts/lint.sh "$SKETCH_NAME"
