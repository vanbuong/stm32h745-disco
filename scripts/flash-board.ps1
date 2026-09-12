# Flash both H745 ELFs over SWD and hardware-reset. No GDB.
$ErrorActionPreference = "Stop"

$Root = Split-Path -Parent $PSScriptRoot
if (-not $env:CM7_ELF) {
    $Cm7 = Join-Path $Root "CM7\build\stm32h745-disco_CM7.elf"
} else {
    $Cm7 = $env:CM7_ELF
}
if (-not $env:CM4_ELF) {
    $Cm4 = Join-Path $Root "CM4\build\stm32h745-disco_CM4.elf"
} else {
    $Cm4 = $env:CM4_ELF
}

function Find-Cli {
    if ($env:STM32_PROGRAMMER_CLI -and (Test-Path $env:STM32_PROGRAMMER_CLI)) {
        return $env:STM32_PROGRAMMER_CLI
    }
    $cmd = Get-Command STM32_Programmer_CLI -ErrorAction SilentlyContinue
    if ($cmd) {
        return $cmd.Source
    }
    $roots = @(
        "$env:ProgramFiles\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
        "$env:ProgramFiles(x86)\STMicroelectronics\STM32Cube\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
    )
    foreach ($p in $roots) {
        if (Test-Path $p) {
            return $p
        }
    }
    $globs = @(
        "C:\ST\STM32CubeCLT_*\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
        "$env:USERPROFILE\ST\STM32CubeCLT_*\STM32CubeProgrammer\bin\STM32_Programmer_CLI.exe"
        "$env:LOCALAPPDATA\stm32cube\bundles\programmer\*\bin\STM32_Programmer_CLI.exe"
        "$env:USERPROFILE\.local\share\stm32cube\bundles\programmer\*\bin\STM32_Programmer_CLI.exe"
    )
    foreach ($g in $globs) {
        $hit = Get-Item $g -ErrorAction SilentlyContinue | Select-Object -Last 1
        if ($hit) {
            return $hit.FullName
        }
    }
    return $null
}

$Cli = Find-Cli
if (-not $Cli) {
    Write-Error "flash: STM32_Programmer_CLI not found. Install STM32CubeCLT / CubeProgrammer, or set STM32_PROGRAMMER_CLI."
}
if (-not (Test-Path $Cm7)) {
    Write-Error "flash: missing $Cm7 — build the Debug preset first"
}
if (-not (Test-Path $Cm4)) {
    Write-Error "flash: missing $Cm4 — build the Debug preset first"
}

Write-Host "flash: $Cli"
Write-Host "flash: $Cm7"
Write-Host "flash: $Cm4"
Write-Host "flash: stop any debug session that owns the ST-LINK"

& $Cli -c port=SWD mode=UR -w $Cm7 -w $Cm4 -v -hardRst
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
Write-Host "flash: ok (NRST). Board should run without the debugger."
