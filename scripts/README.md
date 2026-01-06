# Scripts

## setup_arduino_esp32.sh

This script sets up the Arduino IDE for ESP32 development, for mac and linux computers.

To run the script, use the following command in your terminal:

```bash
cd path/to/your/repo
sh scripts/setup_arduino_esp32.sh
```

## format.sh

This scripts formats the selected sketch files using `clang-format`. It recursively searches for all `.cpp` and `.h` files in the selected sketch and applies the formatting rules defined in the `.clang-format` configuration file.

### options

`<sketch_path>`: The path to the sketch directory you want to format. If not provided, the script will default to the `QuantumDice` sketch.


To run the script, use the following command in your terminal:

```bash
cd path/to/your/repo
sh scripts/format.sh
```

To run the script with a specific sketch path, use:

```bash
cd path/to/your/repo
sh scripts/format.sh path/to/sketch
```

## lint.sh

This script checks the codebase for style issues using `clang-tidy`. It recursively searches for all `.cpp` and `.h` files in the current directory and its subdirectories and runs `clang-tidy` on each file to identify potential issues.

### options

`<directory_path>`: The path to the directory you want to lint. If not provided, the script will default to the `QuantumDice` sketch.

`--suppress-warnings`: If this flag is provided, the script will suppress warning messages during formatting.

To run the script, use the following command in your terminal:

```bash
cd path/to/your/repo
sh scripts/lint.sh
```

To run the script with a specific directory path, use:

```bash
cd path/to/your/repo
sh scripts/lint.sh path/to/directory
```

To run the script while suppressing warnings, use:

```bash
cd path/to/your/repo
sh scripts/lint.sh --suppress-warnings
```

