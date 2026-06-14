#!/bin/bash

set -e

./cross/configure.sh 

meson setup build-linux --native-file cross/linux-clang18.ini
meson compile -C build-linux

meson setup build-rp2350 --cross-file cross/rp2350.ini
meson compile -C build-rp2350

echo "Build successful."
