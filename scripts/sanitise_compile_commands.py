#!/usr/bin/env python3
import json
import os
import sys
import shlex

# --- CONFIGURATION ---
# Adjust these paths to match your CI/Local setup
BUILD_DIR = "Arduino/QuantumDice/build"
INPUT_DB = os.path.join(BUILD_DIR, "compile_commands.json")
OUTPUT_DB = os.path.join(BUILD_DIR, "compile_commands_cleaned.json")

# Flags that crash x86 Clang
BAD_FLAGS = {
    "-mlongcalls",
    "-fno-tree-switch-conversion",
    "-fstrict-volatile-bitfields",
    "-mdisable-hardware-atomics",
    "-mtext-section-literals",
    "-mfix-esp32-psram-cache-issue",
    "-mbss-section-header",
    "-mno-target-align"
}


def expand_and_clean_args(args):
    """
    Iterates through args. If an arg starts with @, it reads the file,
    sanitizes content, and inlines it.
    """
    expanded_args = []

    for arg in args:
        if arg.startswith("@"):
            response_file_path = arg[1:]
            if os.path.exists(response_file_path):
                try:
                    with open(response_file_path, 'r') as rf:
                        content = rf.read()
                        file_args = shlex.split(content)

                        clean_file_args = [x for x in file_args if x not in BAD_FLAGS]

                        expanded_args.extend(clean_file_args)
                except Exception as e:
                    print(f"⚠ Warning: Could not read response file {response_file_path}: {e}")
                    expanded_args.append(arg)
            else:
                print(f"⚠ Warning: Response file not found: {response_file_path}")
                expanded_args.append(arg)
        elif arg in BAD_FLAGS:
            # Filter explicit bad flags (not in response files)
            continue
        else:
            expanded_args.append(arg)

    return expanded_args


def main():
    if not os.path.exists(INPUT_DB):
        print(f"❌ Error: {INPUT_DB} not found.")
        sys.exit(1)

    with open(INPUT_DB, 'r') as f:
        data = json.load(f)

    cleaned_entries = []

    for entry in data:
        if 'command' in entry:
            entry['arguments'] = shlex.split(entry['command'])
            del entry['command']

        entry['arguments'] = expand_and_clean_args(entry['arguments'])

        cleaned_entries.append(entry)

    with open(OUTPUT_DB, 'w') as f:
        json.dump(cleaned_entries, f, indent=2)


if __name__ == "__main__":
    main()
