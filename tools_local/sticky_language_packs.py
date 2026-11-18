#!/usr/bin/env python3
"""Build the Sticky's microSD interface language packs (upstream 2.0 .cplang).

  python tools_local/sticky_language_packs.py [OUTPUT_DIR] [--langs vi,...]

The Sticky firmware keeps only English in flash (custom_i18n_builtin_langs in
platformio.ini). Every other language is compiled here from its repository
YAML into OUTPUT_DIR/<bcp47>.cplang, by the same compiler as upstream's
`language_packs.py` (printf signatures checked against English, strings the
firmware never uses left out, strings equal to English left to the fallback).

The default OUTPUT_DIR is ../output/modules/languages: release.py publishes
that folder with the native modules under appstore:modules/<abi>/languages/,
where the App Store's language-pack item fetches it from.
"""
import argparse
import importlib.util
from pathlib import Path
import struct
import sys

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_OUT = ROOT.parent / "output" / "modules" / "languages"

spec = importlib.util.spec_from_file_location("crossnext_language_packs", ROOT / "tools_local/language_packs.py")
packs = importlib.util.module_from_spec(spec)
spec.loader.exec_module(packs)


def build(out: Path, langs):
    out.mkdir(parents=True, exist_ok=True)
    active = packs.i18n.find_used_string_keys([str(ROOT / "src"), str(ROOT / "lib")])
    built = []
    for path in sorted((ROOT / "lib/I18n/translations").glob("*.yaml")):
        values = packs.i18n.parse_yaml_file(str(path))
        code = values.get("_bcp47", "")
        if code not in langs:
            continue
        strings = {k: v for k, v in values.items() if k.startswith("STR_")}
        # Keys the English source no longer has are dropped with a warning
        # rather than failing the release; a missing translation uses English.
        blob, warnings = packs.compile_pack(strings, code, values["_language_name"], skip_invalid=True,
                                            included_keys=active)
        for warning in warnings:
            print(f"  {code}: {warning}", file=sys.stderr)
        target = out / f"{code}.cplang"
        target.write_bytes(blob)
        entries = struct.unpack_from("<I", blob, 12)[0]
        print(f"{target}: {len(blob)} bytes, {entries} translations")
        built.append(target)
    missing = set(langs) - {p.stem for p in built}
    if missing:
        sys.exit(f"no translation source for: {', '.join(sorted(missing))}")
    return built


def main():
    parser = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("output", nargs="?", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--langs", default="vi", help="comma-separated BCP 47 codes (default: vi)")
    args = parser.parse_args()
    build(args.output, [code.strip() for code in args.langs.split(",") if code.strip()])


if __name__ == "__main__":
    main()
