# Dual-core Cube contexts

STM32CubeIDE for VS Code expects one CMake project per core. These folders
are thin wrappers: all sources stay in `firmware/`. Configure the **repo
root** with preset **Debug (M7 + M4)** to build both ELFs.
