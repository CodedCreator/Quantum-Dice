#!/usr/bin/env python3
import json
import os
import sys
import shlex

# --- CONFIGURATION ---
BUILD_DIR = "Arduino/QuantumDice/build"
INPUT_DB = os.path.join(BUILD_DIR, "compile_commands.json")
OUTPUT_DB = os.path.join(BUILD_DIR, "compile_commands_cleaned.json")

# 1. Flags that crash x86 Clang (GCC/Xtensa specifics)
BAD_FLAGS = {
    "-mlongcalls",
    "-fno-tree-switch-conversion",
    "-fstrict-volatile-bitfields",
    "-mdisable-hardware-atomics",
    "-mtext-section-literals",
    "-mfix-esp32-psram-cache-issue",
    "-mbss-section-header",
    "-mno-target-align",
    "-mno-serialize-volatile",
}

# 2. Set a Host Compiler to fool Clang into thinking this is a PC build
HOST_COMPILER = "c++"
HOST_TARGET_FLAG = "--target=x86_64-pc-linux-gnu"


def expand_and_clean_args(args):
    """
    Iterates through args. If an arg starts with @, it reads the file,
    sanitizes content, and inlines it.
    """
    expanded_args = []

    i = 0
    while i < len(args):
        arg = args[i]

        if arg.startswith("@"):
            response_file_path = arg[1:]
            if os.path.exists(response_file_path):
                try:
                    with open(response_file_path, 'r') as rf:
                        content = rf.read()
                        file_args = shlex.split(content)
                        expanded_args.extend(expand_and_clean_args(file_args))
                except Exception:
                    expanded_args.append(arg)
            else:
                expanded_args.append(arg)
            i += 1
            continue

        if arg == "-target" or arg == "--target":
            i += 2
            continue
        if arg.startswith("--target="):
            i += 1
            continue

        if arg in BAD_FLAGS:
            i += 1
            continue

        expanded_args.append(arg)
        i += 1

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

        cleaned_args = expand_and_clean_args(entry['arguments'])

        if cleaned_args:
            cleaned_args[0] = HOST_COMPILER

        cleaned_args.append(HOST_TARGET_FLAG)

        entry['arguments'] = cleaned_args
        cleaned_entries.append(entry)

    print(f"• Saving sanitized database to {OUTPUT_DB}...")
    with open(OUTPUT_DB, 'w') as f:
        json.dump(cleaned_entries, f, indent=2)

    print("✔ Done. CI Ready.")


if __name__ == "__main__":
    main()
