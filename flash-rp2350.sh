#!/bin/bash

set -e

# Firmware to flash (override by passing a path as the first argument).
FILE="${1:-build-rp2350/platforms/rp2350/sndfx}"

if [ ! -f "$FILE" ]; then
    echo "Firmware not found: $FILE" >&2
    echo "Build it first: meson compile -C build-rp2350" >&2
    exit 1
fi

# OpenOCD invocation shared by detection and flashing. Using the same interface
# and target configs for both means a successful detection guarantees the flash
# step can connect too.
OPENOCD_CMD=(openocd/openocd -s openocd/scripts \
    -f interface/cmsis-dap.cfg -f target/rp2350.cfg \
    -c "adapter speed 5000")

# Detect a CMSIS-DAP debugger by letting OpenOCD try to initialise it. If no
# probe is attached, init fails and we fall back to the BOOTSEL/picotool path.
if "${OPENOCD_CMD[@]}" -c "init" -c "exit" >/dev/null 2>&1; then
    echo "CMSIS-DAP debugger detected: flashing via OpenOCD..."
    "${OPENOCD_CMD[@]}" -c "program $FILE verify reset exit"
else
    echo "No debugger detected: flashing via picotool."
    echo "Hold BOOTSEL and connect the board, then press Enter..."
    read -r
    picotool load "$FILE"
    picotool reboot
fi