#!/usr/bin/env bash
# Flash both H745 ELFs over SWD and hardware-reset. No GDB.
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
CM7="${CM7_ELF:-$ROOT/CM7/build/stm32h745-disco_CM7.elf}"
CM4="${CM4_ELF:-$ROOT/CM4/build/stm32h745-disco_CM4.elf}"

die()
{
    echo "flash: $*" >&2
    exit 1
}

find_cli()
{
    local c

    if [[ -n "${STM32_PROGRAMMER_CLI:-}" && -x "${STM32_PROGRAMMER_CLI}" ]]; then
        echo "${STM32_PROGRAMMER_CLI}"
        return 0
    fi
    if c="$(command -v STM32_Programmer_CLI 2>/dev/null)"; then
        echo "$c"
        return 0
    fi
    # shellcheck disable=SC2086
    for c in \
        "$HOME"/ST/STM32CubeCLT_*/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
        "$HOME"/.local/share/stm32cube/bundles/programmer/*/bin/STM32_Programmer_CLI \
        /opt/st/stm32cubeclt_*/STM32CubeProgrammer/bin/STM32_Programmer_CLI \
        /usr/local/STMicroelectronics/STM32Cube/STM32CubeProgrammer/bin/STM32_Programmer_CLI; do
        if [[ -x "$c" ]]; then
            echo "$c"
            return 0
        fi
    done
    return 1
}

CLI="$(find_cli)" || die "STM32_Programmer_CLI not found. Install STM32CubeCLT / CubeProgrammer, or set STM32_PROGRAMMER_CLI."
[[ -f "$CM7" ]] || die "missing $CM7 — build the Debug preset first"
[[ -f "$CM4" ]] || die "missing $CM4 — build the Debug preset first"

echo "flash: $CLI"
echo "flash: $CM7"
echo "flash: $CM4"
echo "flash: stop any debug session that owns the ST-LINK"

"$CLI" -c port=SWD mode=UR -w "$CM7" -w "$CM4" -v -hardRst
echo "flash: ok (NRST). Board should run without the debugger."
