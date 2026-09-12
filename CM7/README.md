# Dual-core Cube contexts

STM32CubeIDE for VS Code expects one CMake project per core, matching the
CubeMX layout (`CM7/`, `CM4/`, root `mx-generated.cmake` ExternalProject).
These folders are thin wrappers: all sources stay in `firmware/`.

Configure the **repo root** with preset **Debug** to build both
ELFs at `CM7/build/stm32h745-disco_CM7.elf` and `CM4/build/stm32h745-disco_CM4.elf`.
