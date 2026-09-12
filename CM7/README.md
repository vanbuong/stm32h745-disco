# Dual-core Cube contexts

STM32CubeIDE for VS Code expects one CMake project per core, matching the
CubeMX layout (`CM7/`, `CM4/`, root `mx-generated.cmake` ExternalProject).
These folders are thin wrappers: all sources stay in `firmware/`.

Configure the **repo root** with preset **Debug (M7 + M4)** to build both
ELFs at `CM7/build/firmware-m7.elf` and `CM4/build/firmware-m4.elf`.
