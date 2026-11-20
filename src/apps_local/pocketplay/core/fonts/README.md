# Fonts

Jersey 25 at six sizes, 1 bit per pixel. These files are copied unchanged from
[CrossPlay](https://github.com/ma-r-s/crossplay) at commit `fbeac762`.

| File here | From CrossPlay |
|---|---|
| `toybox_{10,14,20,30,44,64}.h` | `src/apps_local/ui/fonts/` |
| `EpdFontData.h` | `lib/EpdFont/` |
| `OFL-Jersey25.txt` | `site/assets/fonts/` |
| `LICENSE-CrossPlay.txt` | `LICENSE` |

Jersey 25 is under the SIL Open Font License, in `OFL-Jersey25.txt`. CrossPlay's
code and generated headers are under the MIT licence, in `LICENSE-CrossPlay.txt`.
`EpdFontData.h` is CrossPlay's, apart from the `EpdGlyph` and `EpdUnicodeInterval`
structs and their field comments, which come from
[epdiy](https://github.com/vroland/epdiy) by Valentin Roland (`src/epd_internals.h`,
LGPL-3.0). Those two layouts are what `text.cpp` reads the fonts through. Only
their layout is used, which the LGPL's section 3 allows in a program under any
licence.

**Don't regenerate these casually.** CrossPlay found that regenerating with newer
font tools makes glyphs a pixel taller, which moves text on every screen. If you
do replace one, check every screen afterwards.

All six hold ASCII as their first interval. `toybox_10` also holds accented
letters, which `text.cpp` deliberately ignores.
