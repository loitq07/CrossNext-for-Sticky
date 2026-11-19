#!/usr/bin/env python3
"""Build CrossNext-Sticky-Resources.zip: the microSD content a Sticky needs.

    python tools_local/sticky_sd_resources.py <output.zip> [--cache DIR]

PET, Reading Quest and The Intercept read their artwork from /apps/ on the
card, and every reader font (Noto Serif included, since 2.0) lives in /fonts/.
That artwork is built from masters this repository does not carry, so the pack
is assembled from upstream CrossNext's published resources, pinned by version
and SHA-256 below. Kept from it:

- apps/          PET (pet-v2), Reading Quest (reading-adventure-v2), The Intercept
- fonts/         reader fonts, minus the large CJK families
- licenses, notices and SOURCES.json, which their licences require

Left out on purpose:

- languages/     packs are tied to one firmware's string table; Sticky ships its
                 own per build (vi.cplang), and upstream's would replace it with
                 one that no longer matches.
- dictionaries/  237 MB of English/European dictionaries, none Vietnamese. The
                 README points to the upstream archive for anyone who wants them.

Before changing UPSTREAM, check the art packs still match the firmware: the
PET loader refuses a pack whose image count differs (petui::*_COUNT).
"""
import argparse
import hashlib
from pathlib import Path
import sys
import urllib.request
import zipfile

UPSTREAM_VERSION = "2.0.4"
UPSTREAM_URL = ("https://github.com/manuelx900/CrossNext/releases/download/"
                f"{UPSTREAM_VERSION}/CrossNext-Resources.zip")
UPSTREAM_SHA256 = "a830be351ed9d4c24126e76a7e85c452f09d34aa4cd551c64b6c1ef815a1ad2a"

KEEP_PREFIXES = ("apps/", "fonts/", "licenses/")
KEEP_FILES = ("THIRD-PARTY-NOTICES.txt", "SOURCES.json")
DROP_FONTS = ("fonts/Noto Serif JP/", "fonts/Noto Serif SC/", "fonts/Noto Serif TC/")
REQUIRED = (
    "apps/pet-v2/graphics.bin", "apps/pet-v2/home.bin", "apps/pet-v2/habitat.bin",
    "apps/pet-v2/breathing.bin", "apps/pet-v2/farewell.bin", "apps/pet-v2/collection.bin",
    "apps/reading-adventure-v2/graphics.bin", "apps/reading-adventure-v2/story.bin",
    "apps/intercept/graphics.bin",
    "fonts/Noto Serif/Noto Serif_14.cpfont",
)

README = f"""CROSSNEXT FOR SEEED RETERMINAL STICKY - MICROSD RESOURCES
(from CrossNext {UPSTREAM_VERSION} resources)

TIENG VIET
----------
Giai nen file ZIP nay va chep thu muc "apps" va "fonts" vao THU MUC GOC cua the
nho microSD (ket qua la /apps/... va /fonts/... tren the), roi khoi dong lai Sticky.

- apps/pet-v2, apps/reading-adventure-v2, apps/intercept: hinh anh cho Thu cung
  (PET), Reading Quest va The Intercept. Thieu thu muc nay, cac app bao thieu
  tai nguyen.
- fonts/: phong chu doc sach, gom Noto Serif (tu ban 2.0 khong con nam trong
  firmware). Thieu thu muc nay, sach chi hien bang phong chu giao dien.
- Neu the da co thu muc apps/fonts cu, hay THAY THE (khong tron) thu muc cu.
  Giu nguyen sach va thu muc /.crosspoint (tien do, cai dat, du lieu luu).
- KHONG chep thu muc "languages" tu goi CrossNext goc: goi ngon ngu cua Sticky
  (vi.cplang) di kem tung ban firmware hoac cai qua App Store.
- Tu dien (dictionaries) khong kem theo. Neu can, lay tu goi goc:
  {UPSTREAM_URL}

ENGLISH
-------
Extract this ZIP and copy the "apps" and "fonts" folders to the ROOT of the
microSD card (giving /apps/... and /fonts/... on the card), then restart.

- apps/: artwork for PET, Reading Quest and The Intercept. Without it those apps
  report missing resources.
- fonts/: reader fonts, including Noto Serif (no longer built into the firmware
  since 2.0). Without it books render in the interface font.
- Replace, do not merge, existing bundled apps/fonts folders. Keep your books and
  /.crosspoint (progress, settings, saves).
- Do NOT copy "languages" from the upstream CrossNext package: Sticky's language
  pack (vi.cplang) ships with each firmware build or installs from the App Store.
- Dictionaries are not included; get them from the upstream archive above.

Keep THIRD-PARTY-NOTICES.txt, SOURCES.json and licenses/ with the files.
"""


def sha256(path):
    digest = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def fetch(cache):
    cache.mkdir(parents=True, exist_ok=True)
    source = cache / f"CrossNext-Resources-{UPSTREAM_VERSION}.zip"
    if not source.exists() or sha256(source) != UPSTREAM_SHA256:
        print(f"downloading {UPSTREAM_URL}", flush=True)
        partial = source.with_suffix(".part")
        urllib.request.urlretrieve(UPSTREAM_URL, partial)
        partial.replace(source)
    actual = sha256(source)
    if actual != UPSTREAM_SHA256:
        sys.exit(f"{source}: sha256 {actual} is not the pinned {UPSTREAM_SHA256}")
    return source


def keep(name):
    if name.endswith("/") or name.startswith(DROP_FONTS):
        return False
    return name.startswith(KEEP_PREFIXES) or name in KEEP_FILES


def build(output, cache):
    source = fetch(cache)
    output.parent.mkdir(parents=True, exist_ok=True)
    partial = output.with_suffix(".part")
    with zipfile.ZipFile(source) as src, zipfile.ZipFile(partial, "w", zipfile.ZIP_DEFLATED, compresslevel=9) as dst:
        names = [n for n in src.namelist() if keep(n)]
        missing = [n for n in REQUIRED if n not in names]
        if missing:
            sys.exit("upstream resources lack: " + ", ".join(missing))
        for name in names:
            info = zipfile.ZipInfo(name, date_time=(2026, 1, 1, 0, 0, 0))
            info.compress_type = zipfile.ZIP_DEFLATED
            dst.writestr(info, src.read(name))
        info = zipfile.ZipInfo("README-STICKY.txt", date_time=(2026, 1, 1, 0, 0, 0))
        info.compress_type = zipfile.ZIP_DEFLATED
        dst.writestr(info, README)
    with zipfile.ZipFile(partial) as check:
        if check.testzip() is not None:
            sys.exit(f"{partial}: archive check failed")
    partial.replace(output)
    print(f"{output}: {len(names) + 1} files, {output.stat().st_size:,} bytes")
    return output


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", type=Path)
    parser.add_argument("--cache", type=Path, default=Path(__file__).resolve().parents[1] / ".pio" / "upstream-resources")
    args = parser.parse_args()
    build(args.output, args.cache)


if __name__ == "__main__":
    main()
