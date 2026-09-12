#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

patterns='lvgl\.h|lv_[A-Za-z0-9_]+\.h|touchgfx|FreeRTOS\.h|<task\.h>|<semphr\.h>|<zephyr/|zephyr\.h|ff\.h|fatfs|stm32h7xx\.h|stm32h7xx_hal|mqtt\.h|lwip/'

fail=0
mapfile -t files < <(find firmware/src/app firmware/src/shell firmware/src/game -type f \( -name '*.c' -o -name '*.h' \) 2>/dev/null || true)

if [[ ${#files[@]} -eq 0 ]]; then
    echo "layering: no app/shell sources yet (ok)"
    exit 0
fi

for f in "${files[@]}"; do
    if grep -nE "$patterns" "$f"; then
        echo "layering: forbidden include in $f"
        fail=1
    fi
done

if [[ "$fail" -ne 0 ]]; then
    exit 1
fi
echo "layering: ok"
