#!/usr/bin/env bash
set -e

echo "--- Running Clang-Tidy (ESP32) ---"

# -------- Sketch selection --------
SKETCH_NAME="${1:-QuantumDice}"
SKETCH_DIR="Arduino/$SKETCH_NAME"
BUILD_DIR="$SKETCH_DIR/build"

if [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
    echo "ERROR: compile_commands.json not found for $SKETCH_NAME"
    echo "Run Arduino/setup_arduino_esp32.sh [$SKETCH_NAME] first."
    exit 1
fi

# -------- Sanitize compile_commands.json --------

if [ ! -f "$BUILD_DIR/compile_commands_cleaned.json" ]; then
	python3 scripts/sanitise_compile_commands.py
fi

mv "$BUILD_DIR/compile_commands.json" "$BUILD_DIR/compile_commands_tmp.json"
mv "$BUILD_DIR/compile_commands_cleaned.json" "$BUILD_DIR/compile_commands.json"

# -------- Find files --------
FILES=$(git ls-files \
  | grep -E '\.(cpp|cxx|cc|hpp|h)$' \
  | grep -v -E 'build/|ImageLibrary/')

if [ -z "$FILES" ]; then
    echo "No C++ files found."
    exit 0
fi

# -------- Build clang-tidy command --------
CLANG_TIDY_CMD=(
    clang-tidy
    -p "$BUILD_DIR"
    -header-filter=^Arduino/
	-system-headers=false
)

# -------- Run clang-tidy --------
echo "$FILES" | xargs -r -P 4 "${CLANG_TIDY_CMD[@]}"

# -------- Restore compile_commands.json --------
mv "$BUILD_DIR/compile_commands.json" "$BUILD_DIR/compile_commands_cleaned.json"
mv "$BUILD_DIR/compile_commands_tmp.json" "$BUILD_DIR/compile_commands.json"

echo "--- Clang-Tidy completed for $SKETCH_NAME ---"
