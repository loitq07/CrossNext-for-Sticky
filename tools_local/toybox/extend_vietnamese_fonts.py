#!/usr/bin/env python3
"""Extend Jersey 25 pixel fonts with complete Vietnamese and Spanish support.

Preserves reviewed ASCII cuts verbatim. Renders diacritics on the matching
integer pixel grid so all accents are pixel-crisp and fit font metrics.
"""
from pathlib import Path
import re
import sys
import unicodedata

ROOT = Path(__file__).resolve().parents[2]

# Full list of Vietnamese precomposed characters (plus Spanish ¡, ¿)
SPANISH_EXTRA = "¡¿ÜüÑñ"

def get_vietnamese_chars():
    v_raw = """
    aàảãáạ
    ăằẳẵắặ
    âầẩẫấậ
    eèẻẽéẹ
    êềểễếệ
    iìỉĩíị
    oòỏõóọ
    ôồổỗốộ
    ơờởỡớợ
    uùủũúụ
    ưừửữứự
    yỳỷỹýỵ
    dđ
    """
    chars = set()
    for line in v_raw.splitlines():
        for ch in line.strip():
            chars.add(ch)
            chars.add(ch.upper())
    for ch in SPANISH_EXTRA:
        chars.add(ch)
    # Exclude basic ASCII
    return sorted([ch for ch in chars if ord(ch) >= 128])


def extend_font(path: Path):
    source = path.read_text(encoding="utf-8")
    name = path.stem

    bm = re.search(r"static const uint8_t " + name + r"Bitmaps\[\d+\] = \{(.*?)\};", source, re.S)
    gs = re.search(r"static const EpdGlyph " + name + r"Glyphs\[\] = \{(.*?)\};", source, re.S)
    if not bm or not gs:
        print(f"Skipping {path.name}: not a standard EpdFont header")
        return

    # Take the 95 ASCII glyphs (space 32 to ~ 126)
    all_glyph_lines = re.findall(r"^\s*\{([-\d, \t]+)\},", gs[1], re.M)
    glyphs = [list(map(int, x.split(","))) for x in all_glyph_lines][:95]
    end = glyphs[-1][-1] + glyphs[-1][-2]
    data = bytearray(int(x, 16) for x in re.findall(r"0x([0-9a-fA-F]+)", bm[1]))[:end]

    def pixels(g):
        w, h, advance, left, top, count, offset = g
        return [[bool(data[offset + (y * w + x) // 8] & (128 >> ((y * w + x) % 8))) for x in range(w)] for y in range(h)]

    def append(rows, g):
        w = len(rows[0])
        h = len(rows)
        bits = [v for row in rows for v in row]
        packed = bytearray((len(bits) + 7) // 8)
        for n, v in enumerate(bits):
            if v:
                packed[n // 8] |= 128 >> (n % 8)
        result = [w, h, g[2], g[3], g[4], len(packed), len(data)]
        data.extend(packed)
        return result

    # Measurements based on cap height 'H'
    cap = glyphs[ord("H") - 32][1]
    unit = max(1, cap // 12)
    mh = 3 * unit
    gap = max(1, unit)

    # Pre-generate marks for width w
    def make_acute(w):
        mark = [[False] * w for _ in range(mh)]
        cx = w // 2
        for y in range(mh):
            for dx in range(unit):
                x = min(w - 1, cx + unit - y // 2 + dx)
                mark[y][x] = True
        return mark

    def make_grave(w):
        return [list(reversed(row)) for row in make_acute(w)]

    def make_circumflex(w):
        mark = [[False] * w for _ in range(mh)]
        cx = w // 2
        for y in range(mh):
            spread = y
            for dx in range(unit):
                lx = cx - spread - 1 + dx
                rx = cx + spread + dx
                if 0 <= lx < w:
                    mark[y][lx] = True
                if 0 <= rx < w:
                    mark[y][rx] = True
        return mark

    def make_breve(w):
        mark = [[False] * w for _ in range(mh)]
        cx = w // 2
        for y in range(mh):
            spread = mh - 1 - y
            for dx in range(unit):
                lx = cx - spread - 1 + dx
                rx = cx + spread + dx
                if 0 <= lx < w:
                    mark[y][lx] = True
                if 0 <= rx < w:
                    mark[y][rx] = True
        return mark

    def make_hook(w):
        mark = [[False] * w for _ in range(mh)]
        cx = w // 2
        for y in range(mh):
            if y == 0:
                for dx in range(2 * unit):
                    if 0 <= cx - unit + dx < w:
                        mark[y][cx - unit + dx] = True
            elif y == 1:
                for dx in range(unit):
                    if 0 <= cx + unit + dx < w:
                        mark[y][cx + unit + dx] = True
            elif y == 2:
                for dx in range(unit):
                    if 0 <= cx + dx < w:
                        mark[y][cx + dx] = True
        return mark

    def make_tilde(w):
        start = max(0, (w - 6 * unit) // 2)
        mark = [[False] * w for _ in range(mh)]
        for x in range(min(w, 6 * unit)):
            y = unit if x < 2 * unit or x >= 4 * unit else 0
            for dy in range(unit):
                if y + dy < mh and start + x < w:
                    mark[y + dy][start + x] = True
        return mark

    def add_dot_below(rows):
        w = len(rows[0])
        dh = unit
        dw = max(unit, 2 * unit if unit > 1 else 1)
        dot = [[False] * w for _ in range(dh)]
        cx = (w - dw) // 2
        for y in range(dh):
            for x in range(dw):
                dot[y][cx + x] = True
        return rows + [[False] * w for _ in range(gap)] + dot

    def add_horn(rows):
        w = len(rows[0])
        h = len(rows)
        hw = max(1, unit)
        new_w = w + hw
        new_rows = [row + [False] * hw for row in rows]
        for y in range(min(h, 3 * unit)):
            for dx in range(hw):
                new_rows[y][w + dx] = True
        return new_rows

    def make_D_stroke(rows):
        w = len(rows[0])
        h = len(rows)
        cy = h // 2
        for dy in range(unit):
            for x in range(min(w, 4 * unit)):
                rows[cy + dy][x] = True
        return rows

    def make_d_stroke(rows):
        w = len(rows[0])
        cy = 2 * unit
        for dy in range(unit):
            for x in range(w):
                rows[cy + dy][x] = True
        return rows

    def make_stacked_circ(tone, w):
        circ = make_circumflex(w)
        th = 2 * unit
        cx = w // 2
        tmark = [[False] * w for _ in range(th)]
        if tone == "acute":
            for y in range(th):
                for dx in range(unit):
                    x = min(w - 1, cx + dx + (th - 1 - y))
                    tmark[y][x] = True
        elif tone == "grave":
            for y in range(th):
                for dx in range(unit):
                    x = max(0, cx - dx - (th - 1 - y) - 1)
                    tmark[y][x] = True
        elif tone == "hook":
            for dx in range(2 * unit):
                if cx + dx < w:
                    tmark[0][cx + dx] = True
            for dy in range(unit):
                if 1 + dy < th and cx + unit < w:
                    tmark[1 + dy][cx + unit] = True
        elif tone == "tilde":
            start = max(0, (w - 4 * unit) // 2)
            for x in range(min(w, 4 * unit)):
                y = unit if x < 2 * unit else 0
                for dy in range(unit):
                    if y + dy < th and start + x < w:
                        tmark[y + dy][start + x] = True
        return tmark + [[False] * w for _ in range(gap)] + circ

    def make_stacked_breve(tone, w):
        breve = make_breve(w)
        th = 2 * unit
        cx = w // 2
        tmark = [[False] * w for _ in range(th)]
        if tone == "acute":
            for y in range(th):
                for dx in range(unit):
                    x = min(w - 1, cx + dx + (th - 1 - y))
                    tmark[y][x] = True
        elif tone == "grave":
            for y in range(th):
                for dx in range(unit):
                    x = max(0, cx - dx - (th - 1 - y) - 1)
                    tmark[y][x] = True
        elif tone == "hook":
            for dx in range(2 * unit):
                if cx + dx < w:
                    tmark[0][cx + dx] = True
            for dy in range(unit):
                if 1 + dy < th and cx + unit < w:
                    tmark[1 + dy][cx + unit] = True
        elif tone == "tilde":
            start = max(0, (w - 4 * unit) // 2)
            for x in range(min(w, 4 * unit)):
                y = unit if x < 2 * unit else 0
                for dy in range(unit):
                    if y + dy < th and start + x < w:
                        tmark[y + dy][start + x] = True
        return tmark + [[False] * w for _ in range(gap)] + breve

    def make_diaeresis(w):
        mark = [[False] * w for _ in range(mh)]
        for y in range(unit):
            for x in (w // 3, 2 * w // 3):
                for dx in range(unit):
                    mark[y + unit][min(w - 1, x + dx)] = True
        return mark

    # Build glyph for any character
    chars_to_add = get_vietnamese_chars()
    # Codepoints mapping
    created_glyphs = {}  # ord(ch): (ch, glyph_struct)

    for ch in chars_to_add:
        cp = ord(ch)
        if ch == '¡':
            g = glyphs[ord('!') - 32].copy()
            rows = [list(reversed(r)) for r in reversed(pixels(g))]
            g[4] = g[1] - max(0, g[1] // 6)
            created_glyphs[cp] = (ch, rows, g)
            continue
        elif ch == '¿':
            g = glyphs[ord('?') - 32].copy()
            rows = [list(reversed(r)) for r in reversed(pixels(g))]
            g[4] = g[1] - max(0, g[1] // 6)
            created_glyphs[cp] = (ch, rows, g)
            continue
        elif ch == 'Đ':
            g = glyphs[ord('D') - 32].copy()
            rows = [row[:] for row in pixels(g)]
            rows = make_D_stroke(rows)
            created_glyphs[cp] = (ch, rows, g)
            continue
        elif ch == 'đ':
            g = glyphs[ord('d') - 32].copy()
            rows = [row[:] for row in pixels(g)]
            rows = make_d_stroke(rows)
            created_glyphs[cp] = (ch, rows, g)
            continue

        nfd = unicodedata.normalize('NFD', ch)
        base_char = nfd[0]
        mark_names = [unicodedata.name(m) for m in nfd[1:]]

        base_ord = ord(base_char)
        if base_ord < 32 or base_ord > 126:
            continue
        g = glyphs[base_ord - 32].copy()
        rows = [row[:] for row in pixels(g)]

        # Check special base modifications
        has_horn = 'COMBINING HORN' in mark_names
        has_dot_below = 'COMBINING DOT BELOW' in mark_names
        has_circumflex = 'COMBINING CIRCUMFLEX ACCENT' in mark_names
        has_breve = 'COMBINING BREVE' in mark_names
        has_acute = 'COMBINING ACUTE ACCENT' in mark_names
        has_grave = 'COMBINING GRAVE ACCENT' in mark_names
        has_hook = 'COMBINING HOOK ABOVE' in mark_names
        has_tilde = 'COMBINING TILDE' in mark_names
        has_diaeresis = 'COMBINING DIAERESIS' in mark_names

        if has_horn:
            rows = add_horn(rows)
            g[0] = len(rows[0])
            g[2] += max(1, unit)

        w = len(rows[0])

        # If it has a dot below
        if has_dot_below:
            rows = add_dot_below(rows)

        # Build top marks
        top_mark = None
        if has_circumflex:
            if has_acute:
                top_mark = make_stacked_circ("acute", w)
            elif has_grave:
                top_mark = make_stacked_circ("grave", w)
            elif has_hook:
                top_mark = make_stacked_circ("hook", w)
            elif has_tilde:
                top_mark = make_stacked_circ("tilde", w)
            else:
                top_mark = make_circumflex(w)
        elif has_breve:
            if has_acute:
                top_mark = make_stacked_breve("acute", w)
            elif has_grave:
                top_mark = make_stacked_breve("grave", w)
            elif has_hook:
                top_mark = make_stacked_breve("hook", w)
            elif has_tilde:
                top_mark = make_stacked_breve("tilde", w)
            else:
                top_mark = make_breve(w)
        elif has_diaeresis:
            top_mark = make_diaeresis(w)
        elif has_acute:
            top_mark = make_acute(w)
        elif has_grave:
            top_mark = make_grave(w)
        elif has_hook:
            top_mark = make_hook(w)
        elif has_tilde:
            top_mark = make_tilde(w)

        if top_mark:
            # For lowercase 'i', remove dot before adding mark
            if base_char == 'i':
                dot_end = 0
                while dot_end < len(rows) and any(rows[dot_end]):
                    dot_end += 1
                for y in range(dot_end):
                    rows[y] = [False] * w
            rows = top_mark + [[False] * w for _ in range(gap)] + rows
            g[4] += len(top_mark) + gap

        created_glyphs[cp] = (ch, rows, g)

    # Sort all created glyphs by codepoint
    sorted_cps = sorted(created_glyphs.keys())
    extra_appended = []  # list of (cp, ch, glyph_entry)

    for cp in sorted_cps:
        ch, r, g = created_glyphs[cp]
        extra_appended.append((cp, ch, append(r, g)))

    # Now group codepoints into contiguous EpdUnicodeInterval
    intervals = [(0x20, 0x7E, 0)]  # ASCII interval
    all_extra_cps = [x[0] for x in extra_appended]

    # Find contiguous ranges
    range_start = all_extra_cps[0]
    range_end = all_extra_cps[0]
    start_idx = 95

    for i in range(1, len(all_extra_cps)):
        curr_cp = all_extra_cps[i]
        if curr_cp == range_end + 1:
            range_end = curr_cp
        else:
            intervals.append((range_start, range_end, start_idx))
            start_idx = 95 + i
            range_start = curr_cp
            range_end = curr_cp
    intervals.append((range_start, range_end, start_idx))

    # Append right arrow if present in original
    # U+2192 (right arrow)
    arrow_g = [18, 14, 352, 0, 20, 32, len(data)]
    # We can keep U+2192 if needed
    arrow_rows = [[False] * 18 for _ in range(14)]
    # Draw simple right arrow
    for x in range(16):
        arrow_rows[7][x] = True
        arrow_rows[6][x] = True
    for y in range(6):
        arrow_rows[7 - y][16 - y] = True
        arrow_rows[6 + y][16 - y] = True
    arrow_g = append(arrow_rows, arrow_g)
    arrow_idx = 95 + len(extra_appended)
    intervals.append((0x2192, 0x2192, arrow_idx))
    extra_appended.append((0x2192, "->", arrow_g))

    # Reconstruct C++ file
    bitmap_str = "static const uint8_t " + name + "Bitmaps[" + str(len(data)) + "] = {\n"
    bitmap_str += "".join("    " + ", ".join(f"0x{x:02X}" for x in data[i : i + 18]) + ",\n" for i in range(0, len(data), 18))
    bitmap_str += "};"
    source = source[: bm.start()] + bitmap_str + source[bm.end() :]

    gs = re.search(r"static const EpdGlyph " + name + r"Glyphs\[\] = \{(.*?)\};", source, re.S)
    glyph_block = "static const EpdGlyph " + name + "Glyphs[] = {\n"
    for i, g in enumerate(glyphs):
        ch_label = chr(32 + i) if i != 60 else "backslash"
        glyph_block += f"    {{{', '.join(map(str, g))}}},  // {ch_label}\n"
    for cp, ch, g in extra_appended:
        glyph_block += f"    {{{', '.join(map(str, g))}}},  // {ch} (U+{cp:04X})\n"
    glyph_block += "};"
    source = source[: gs.start()] + glyph_block + source[gs.end() :]

    interval_block = "static const EpdUnicodeInterval " + name + "Intervals[] = {\n"
    for a, b, idx in intervals:
        interval_block += f"    {{0x{a:X}, 0x{b:X}, {idx}}},\n"
    interval_block += "};"
    source = re.sub(
        r"static const EpdUnicodeInterval " + name + r"Intervals\[\] = \{.*?\};",
        interval_block,
        source,
        flags=re.S,
    )
    source = re.sub(
        r"(" + name + r"Intervals,\s*)\d+,",
        lambda m: m.group(1) + str(len(intervals)) + ",",
        source,
    )

    if "Vietnamese glyphs:" not in source:
        source = source.replace(" */", " * Vietnamese glyphs: tools_local/toybox/extend_vietnamese_fonts.py\n */", 1)

    path.write_text(source, encoding="utf-8", newline="\n")
    print(f"Updated {name}: added {len(extra_appended)} glyphs, {len(intervals)} intervals, bitmap data size {len(data)} bytes")


if __name__ == "__main__":
    for size in (10, 14, 20, 30):
        target_path = ROOT / f"src/apps_local/ui/fonts/toybox_{size}.h"
        if target_path.exists():
            extend_font(target_path)
