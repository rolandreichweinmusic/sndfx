#!/bin/bash

set -e

meson setup build-linux
meson compile -C build-linux
meson setup build-rp2350 --cross-file cross/rp2350.ini
meson compile -C build-rp2350

echo "Build successful."