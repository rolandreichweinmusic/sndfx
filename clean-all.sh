#!/bin/bash

rm -rf build-linux
rm -rf build-rp2350
meson subprojects purge --confirm --include-cache
