#!/usr/bin/env python3
"""Generate one-bit Baemin Daniel display, UI, button and tile font headers with balanced sizing."""
from pathlib import Path
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
SOURCE = Path("D:/Downloads/BAEMIN FONT/BMDANIEL-Bold.ttf")
FONTCONVERT = ROOT / "lib/EpdFont/scripts/fontconvert.py"
OUT_DIR = ROOT / "src/apps_local/ui/fonts"

SIZES = [
    (10, "baemin_10"),  # tile / dense caption / subtitle (25px advanceY, 21px asc, 15px ink)
    (12, "baemin_12"),  # button / pager / clock / badge (30px advanceY, 25px asc, 18px ink)
    (14, "baemin_14"),  # UI / menu row / item label (35px advanceY, 30px asc, 21px ink)
    (18, "baemin_18"),  # header / display title (45px advanceY, 38px asc, 27px ink)
]

INTERVALS = [
    ("0x0020", "0x007E"),  # ASCII printable
    ("0x00A0", "0x00FF"),  # Latin-1 supplement (symbols + accented Latin)
    ("0x0100", "0x017F"),  # Latin Extended-A (Đ, đ, ă, etc.)
    ("0x1EA0", "0x1EF9"),  # Vietnamese diacritics
    ("0x2010", "0x2026"),  # General punctuation (dashes, quotes, ellipsis)
]

def main():
    if not SOURCE.exists():
        print(f"Font source not found: {SOURCE}", file=sys.stderr)
        sys.exit(1)

    py = sys.executable

    for sz, name in SIZES:
        cmd = [py, str(FONTCONVERT), name, str(sz), str(SOURCE), "--force-autohint"]
        for start, end in INTERVALS:
            cmd.extend(["--additional-intervals", f"{start},{end}"])

        out_path = OUT_DIR / f"{name}.h"
        print(f"Generating {out_path.name} (size {sz})...")
        res = subprocess.run(cmd, capture_output=True, text=True, encoding="utf-8")
        if res.returncode != 0:
            print(f"Error generating {name}:\n{res.stderr}", file=sys.stderr)
            sys.exit(res.returncode)

        content = "\n".join(line for line in res.stdout.splitlines() if not line.endswith("extracted")) + "\n"
        out_path.write_text(content, encoding="utf-8")
        print(f"Wrote {out_path} ({len(content)} bytes)")

if __name__ == "__main__":
    main()
