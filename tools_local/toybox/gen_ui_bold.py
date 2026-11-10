"""Generate matching one-bit UI bold cuts from the bundled OFL fonts with full Vietnamese support."""
from pathlib import Path
import re
import subprocess
import sys
import tempfile
from fontTools import subset

root = Path(__file__).resolve().parents[2]
UNICODES = (
    "U+0020-007E,U+00A0-00FF,U+0102-0103,U+0110-0111,U+0128-0129,U+0168-0169,"
    "U+01A0-01A1,U+01AF-01B0,U+1EA0-1EF9,U+2013-2014,U+2018-2019,U+2026"
)

with tempfile.TemporaryDirectory() as temporary:
    for family, sizes, prefix in (("NotoSans", (14,), "ui_prose_bold"), ("NotoSerif", (11, 14), "reading_serif_bold")):
        font = Path(temporary) / f"{family}-Bold.ttf"
        subset.main([
            str(root / f"lib/EpdFont/builtinFonts/source/{family}/{family}-Bold.ttf"),
            f"--unicodes={UNICODES}",
            "--output-file=" + str(font),
        ])
        for size in sizes:
            name = f"{prefix}_{size}"
            result = subprocess.run(
                [sys.executable, str(root / "lib/EpdFont/scripts/fontconvert.py"), name, str(size), str(font), "--force-autohint"],
                capture_output=True,
                text=True,
                encoding="utf-8",
                check=True,
            )
            source = re.sub(r" \* Command used:.*", " * Source: tools_local/toybox/gen_ui_bold.py (bundled Noto, SIL OFL)", result.stdout)
            source = "\n".join(line for line in source.splitlines() if not line.endswith("extracted")) + "\n"
            (root / f"src/apps_local/ui/fonts/{name}.h").write_text(source, encoding="utf-8", newline="\n")
            print(f"Generated {name}.h")
