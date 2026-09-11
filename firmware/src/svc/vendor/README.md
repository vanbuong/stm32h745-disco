Vendored decoders used by `media_decode_image` and the audio engine (not LVGL, not LibJPEG).

- `tjpgd/` — ChaN TJpgDec R0.03, copied from LVGL's tree with `LV_USE_TJPGD` guards removed. License in `tjpgd.c`.
- `puff/` — Mark Adler puff 2.3 (zlib inflate). License in `puff.h`.
- `helix_generic_asm.h` — C fallback for Helix `MULSHIFT32` / `MADD64` (host GCC and Cortex-M4). Force-included so `third_party/helix/real/assembly.h` is skipped.

Do not clang-format these files. cppcheck/coverage skip this directory.
