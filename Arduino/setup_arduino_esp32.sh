#!/usr/bin/env bash
set -e

echo "=== Arduino ESP32 setup ==="

# -------- Config --------
ESP32_CORE="esp32:esp32"
BOARD_FQBN="esp32:esp32:esp32"

# -------- Sketch selection --------
SKETCH_NAME="${1:-QuantumDice}"
SKETCH_DIR="Arduino/$SKETCH_NAME"
INO_FILE="$SKETCH_DIR/$SKETCH_NAME.ino"
BUILD_DIR="$SKETCH_DIR/build"

# -------- Validate sketch --------
if [ ! -f "$INO_FILE" ]; then
    echo "ERROR: Sketch not found:"
    echo "Expected: $INO_FILE"
    exit 1
fi

# -------- Arduino CLI --------
if ! command -v arduino-cli >/dev/null 2>&1; then
    echo "Installing Arduino CLI..."
    curl -fsSL https://raw.githubusercontent.com/arduino/arduino-cli/master/install.sh | sh
    sudo mv bin/arduino-cli /usr/local/bin/arduino-cli
else
    echo "Arduino CLI already installed"
fi

arduino-cli config init >/dev/null 2>&1 || true
arduino-cli config set board_manager.additional_urls \
  https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json

arduino-cli core update-index
arduino-cli core install "$ESP32_CORE"

# -------- Install required libraries --------
LIBRARIES=(
    "Adafruit Unified Sensor"
    "Adafruit BNO055"
	"Adafruit GC9A01A"
	"Button2"
    # Add more libraries here if the project requires them:
    # "Adafruit LSM6DSOX"
    # "Adafruit GFX"
)

echo "Installing required Arduino libraries..."
for lib in "${LIBRARIES[@]}"; do
    echo "Installing $lib..."
    arduino-cli lib install "$lib"
done

# -------- Generate compile_commands.json --------
echo "Generating compile_commands.json for $SKETCH_NAME"
mkdir -p "$BUILD_DIR"

arduino-cli compile \
  --fqbn "$BOARD_FQBN" \
  --build-path "$BUILD_DIR" \
  --only-compilation-database \
  "$SKETCH_DIR"

echo "=== ESP32 setup complete ==="
