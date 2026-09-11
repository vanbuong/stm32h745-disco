#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/../.." && pwd)"
cd "$ROOT"

mapfile -t files < <(git ls-files '*.c' '*.h')
if [[ ${#files[@]} -eq 0 ]]; then
    echo "format: no C files"
    exit 0
fi

clang-format --dry-run --Werror "${files[@]}"
echo "format: ok"
