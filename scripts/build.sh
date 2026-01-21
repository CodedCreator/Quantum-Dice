#!/usr/bin/env bash
set -e

echo "=== Arduino ESP32-S3 QuantumDice build ==="

# -------- Config --------
ESP32_CORE="esp32:esp32@3.3.2"
BOARD_FQBN="esp32:esp32:esp32s3:\
	USBMode=default,\
	CDCOnBoot=cdc,\
	FlashSize=16M,\
	PartitionScheme=app3M_fat9M_16MB,\
	PSRAM=opi
"

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

# -------- Build --------
echo "Building ESP32-S3 sketch: $SKETCH_NAME"
mkdir -p "$BUILD_DIR"

BUILD_RES=arduino-cli compile \
  --fqbn "$BOARD_FQBN" \
  --build-path "$BUILD_DIR" \
  --export-binaries \
  "$SKETCH_DIR"

if [[ $? -ne 0 ]]; then
	echo "ERROR: Build failed"
	echo "Make sure you have run setup_deps.sh to install dependencies."
	exit 1
fi

echo
echo "=== Build complete ==="
echo "Generated firmware files:"
ls -lh "$BUILD_DIR"/*.bin
