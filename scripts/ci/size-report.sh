#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 1 ]]; then
    echo "usage: size-report.sh <elf>..." >&2
    exit 2
fi

arm-none-eabi-size "$@"
