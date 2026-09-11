Vendored decoders used by `media_decode_image` (not LVGL, not LibJPEG).

- `tjpgd/` — ChaN TJpgDec R0.03, copied from LVGL's tree with `LV_USE_TJPGD` guards removed. License in `tjpgd.c`.
- `puff/` — Mark Adler puff 2.3 (zlib inflate). License in `puff.h`.

Do not clang-format these files. cppcheck/coverage skip this directory.
