#!/bin/bash

set -e

clang-18 --version >/dev/null 2>&1 || {
    echo "Error: clang-18 not found. Please install it and try again." >&2
    exit 1
}
