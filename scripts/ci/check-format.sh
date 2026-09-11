#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

files=()
while IFS= read -r f; do
    case "$f" in
    third_party/*) continue ;;
    firmware/src/port/cube/stm32h7xx_hal_conf.h) continue ;;
    firmware/src/port/cube/ft5336_conf.h) continue ;;
    firmware/src/port/fatfs/ffconf.h) continue ;;
    firmware/src/port/lvgl/lv_conf.h) continue ;;
    firmware/src/port/lvgl_sim/lv_conf.h) continue ;;
    esac
    if [[ -f "$f" ]]; then
        files+=("$f")
    fi
done < <(git ls-files '*.c' '*.h')

if [[ ${#files[@]} -eq 0 ]]; then
    echo "format: no C files"
    exit 0
fi

clang-format --dry-run --Werror "${files[@]}"
echo "format: ok"
