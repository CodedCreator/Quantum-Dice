#!/usr/bin/env bash
set -e

echo "--- Running Clang-Tidy (ESP32) ---"

# -------- Sketch selection --------
SKETCH_NAME="${1:-QuantumDice}"
SKETCH_DIR="Arduino/$SKETCH_NAME"
BUILD_DIR="$SKETCH_DIR/build"

# -------- Parse optional flags --------
SUPPRESS_WARNINGS=false
if [[ "$2" == "--suppress-warnings" ]]; then
    SUPPRESS_WARNINGS=true
    echo "⚠ Suppressing warnings; only errors will be shown."
fi

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

# -------- Build clang-tidy command --------
CLANG_TIDY_CMD=(
    clang-tidy
    -p "$BUILD_DIR"
    "-header-filter=^Arduino/"
)

if [ "$SUPPRESS_WARNINGS" = true ]; then
    CLANG_TIDY_CMD+=("-warnings=none")
fi

# -------- Run clang-tidy --------
echo "$FILES" | xargs -r -P 4 "${CLANG_TIDY_CMD[@]}"

echo "--- Clang-Tidy completed for $SKETCH_NAME ---"
