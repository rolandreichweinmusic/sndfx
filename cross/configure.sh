#!/bin/sh
# Detect ARM toolchain and generate cross file

TOOLCHAIN_NAME="arm-gnu-toolchain-14.3.rel1-x86_64-arm-none-eabi"

for prefix in "$HOME" "/usr/local"; do
    candidate="$prefix/$TOOLCHAIN_NAME"
    if [ -x "$candidate/bin/arm-none-eabi-gcc" ]; then
        TOOLCHAIN_PATH="$candidate"
        break
    fi
done

if [ -z "$TOOLCHAIN_PATH" ]; then
    echo "Error: ARM toolchain not found in $HOME or /usr/local" >&2
    exit 1
fi

echo "Using toolchain: $TOOLCHAIN_PATH"

sed "s|@TOOLCHAIN_PATH@|$TOOLCHAIN_PATH|g" cross/rp2350.ini.in > cross/rp2350.ini
