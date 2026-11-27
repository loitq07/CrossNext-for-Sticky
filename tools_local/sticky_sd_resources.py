#!/usr/bin/env python3
"""Build CrossNext-Sticky-Resources.zip: the microSD content a Sticky needs.

    python tools_local/sticky_sd_resources.py <output.zip> [--cache DIR]

PET, Reading Quest and The Intercept read their artwork from /apps/ on the
card, and every reader font (Noto Serif included, since 2.0) lives in /fonts/.
That artwork is built from masters this repository does not carry, so the pack
is assembled from upstream CrossNext's published resources, pinned by version
and SHA-256 below. Kept from it:

- apps/                PET (pet-v2), Reading Quest (reading-adventure-v2), The Intercept
- fonts/               reader fonts, minus the large CJK families
- languages/fonts/     UI fallback fonts for non-Latin scripts (Arabic, Hebrew, Thai, CJK)
- languages/keyboards/ extra microSD keyboard layouts
- licenses, notices and SOURCES.json, which their licences require

Left out on purpose:

- languages/*.cplang   packs are tied to one firmware's string table; Sticky ships its
                       own per build, and the App Store installs them per ABI.
- languages/wordle/    large Wordle word banks
- dictionaries/        237 MB of English/European dictionaries, none Vietnamese. The
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

KEEP_PREFIXES = ("apps/", "fonts/", "languages/fonts/", "languages/keyboards/", "licenses/")
KEEP_FILES = ("THIRD-PARTY-NOTICES.txt", "SOURCES.json")
DROP_FONTS = ("fonts/Noto Serif JP/", "fonts/Noto Serif SC/", "fonts/Noto Serif TC/")
REQUIRED = (
    "apps/pet-v2/graphics.bin", "apps/pet-v2/home.bin", "apps/pet-v2/habitat.bin",
    "apps/pet-v2/breathing.bin", "apps/pet-v2/farewell.bin", "apps/pet-v2/collection.bin",
    "apps/reading-adventure-v2/graphics.bin", "apps/reading-adventure-v2/story.bin",
    "apps/intercept/graphics.bin",
    "fonts/Noto Serif/Noto Serif_14.cpfont",
    "languages/fonts/Interface_10.cpfont",
)

README = f"""CROSSNEXT FOR SEEED RETERMINAL STICKY - MICROSD RESOURCES
(from CrossNext {UPSTREAM_VERSION} resources)

TIENG VIET
----------
Giai nen file ZIP nay va chep thu muc "apps", "fonts" va "languages" vao THU MUC
GOC cua the nho microSD (ket qua la /apps/..., /fonts/..., /languages/fonts/...,
/languages/keyboards/... tren the), roi khoi dong lai Sticky.

- apps/pet-v2, apps/reading-adventure-v2, apps/intercept: hinh anh cho Thu cung
  (PET), Reading Quest va The Intercept. Thieu thu muc nay, cac app bao thieu
  tai nguyen.
- fonts/: phong chu doc sach, gom Noto Serif (tu ban 2.0 khong con nam trong
  firmware). Thieu thu muc nay, sach chi hien bang phong chu giao dien.
- languages/fonts/ & languages/keyboards/: phong chu giao dien bo sung cho
  tieng A Rap, Do Thai, Thai, Nhat, Trung va ban phim mo rong.
- Neu the da co thu muc apps/fonts cu, hay THAY THE (khong tron) thu muc cu.
  Voi "languages", giu lai cac file *.cplang da cai qua App Store (khong can
  chep file *.cplang tu goi CrossNext goc).
  Giu nguyen sach va thu muc /.crosspoint (tien do, cai dat, du lieu luu).
- Tu dien (dictionaries) khong kem theo. Neu can, lay tu goi goc:
  {UPSTREAM_URL}

ENGLISH
-------
Extract this ZIP and copy the "apps", "fonts" and "languages" folders to the
ROOT of the microSD card (giving /apps/..., /fonts/..., /languages/fonts/...,
/languages/keyboards/... on the card), then restart.

- apps/: artwork for PET, Reading Quest and The Intercept. Without it those apps
  report missing resources.
- fonts/: reader fonts, including Noto Serif (no longer built into the firmware
  since 2.0). Without it books render in the interface font.
- languages/fonts/ & languages/keyboards/: UI fallback fonts for non-Latin
  scripts (Arabic, Hebrew, Thai, Japanese, Chinese) and extra keyboard layouts.
- Replace, do not merge, existing bundled apps/fonts folders. Keep your books,
  installed *.cplang packs, and /.crosspoint (progress, settings, saves).
- Do NOT copy *.cplang files from the upstream CrossNext package: Sticky's
  language packs install from the App Store per firmware build.
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


# --- Over-the-air packs -----------------------------------------------------
#
# The App Store installs these straight onto the card (AppStoreActivity's
# Resources tab). Reader fonts are not here: Settings > Manage fonts > Download
# already fetches them (Noto Serif included) from the CrossPoint font catalog. Each
# file is a flat asset of the RELEASE_TAG release; the firmware carries every
# path, size and SHA-256 in ResourcePacks.generated.h, so a device verifies
# what it fetched without trusting a manifest from the network.
RELEASE_TAG = f"resources-{UPSTREAM_VERSION}"
RELEASE_URL = f"https://github.com/loitq07/CrossNext-for-Sticky/releases/download/{RELEASE_TAG}/"
PACKS = (
    # id, card folder (inside the upstream zip), English name, Vietnamese name, English blurb, Vietnamese blurb
    ("pet", "apps/pet-v2/", "PET RESOURCES", "Tài nguyên Thú cưng",
     "Artwork the PET app needs", "Hình ảnh cần cho app Thú cưng (PET)"),
    ("quest", "apps/reading-adventure-v2/", "READING QUEST RESOURCES", "Tài nguyên Reading Quest",
     "Artwork Reading Quest needs", "Hình ảnh cần cho Reading Quest"),
    ("intercept", "apps/intercept/", "THE INTERCEPT RESOURCES", "Tài nguyên The Intercept",
     "Artwork The Intercept needs", "Hình ảnh cần cho The Intercept"),
)


def asset_name(pack_id, path):
    base = path.rsplit("/", 1)[1]
    clean = "".join(c if c.isalnum() or c in "._-" else "-" for c in base)
    return f"{pack_id}--{clean}"


def c_str(text):
    return '"' + text.replace("\\", "\\\\").replace('"', '\\"') + '"'


def ota(assets_dir, header, cache):
    source = fetch(cache)
    assets_dir.mkdir(parents=True, exist_ok=True)
    out = ["// Generated by tools_local/sticky_sd_resources.py --ota. Do not edit.",
           f"// Upstream CrossNext {UPSTREAM_VERSION} resources, served from the {RELEASE_TAG} release.",
           "#pragma once", "", "#include <cstddef>", "#include <cstdint>", "",
           "struct ResourceFile {", "  const char* path;   // where it lives on the card",
           "  const char* asset;  // release asset name", "  uint32_t size;",
           "  uint8_t sha256[32];", "};", "",
           "struct ResourcePack {", "  const char* id;", "  const char* folder;  // card folder, for display",
           "  const ResourceFile* files;", "  size_t count;", "  uint32_t totalBytes;", "};", "",
           f"inline constexpr const char kResourceReleaseUrl[] = {c_str(RELEASE_URL)};", ""]
    packs = []
    with zipfile.ZipFile(source) as src:
        for pack_id, folder, *_ in PACKS:
            names = sorted(n for n in src.namelist() if n.startswith(folder) and not n.endswith("/"))
            if not names:
                sys.exit(f"upstream resources have no {folder}")
            sym = "kResourceFiles_" + pack_id.replace("-", "_")
            out.append(f"inline constexpr ResourceFile {sym}[] = {{")
            total = 0
            for name in names:
                data = src.read(name)
                asset = asset_name(pack_id, name)
                (assets_dir / asset).write_bytes(data)
                digest = ", ".join(f"0x{b:02x}" for b in hashlib.sha256(data).digest())
                out.append(f"    {{{c_str('/' + name)}, {c_str(asset)}, {len(data)}u, {{{digest}}}}},")
                total += len(data)
            out.append("};")
            packs.append((pack_id, "/" + folder, sym, len(names), total))
    out.append("")
    out.append("inline constexpr ResourcePack kResourcePacks[] = {")
    for pack_id, folder, sym, count, total in packs:
        out.append(f"    {{{c_str(pack_id)}, {c_str(folder)}, {sym}, {count}, {total}u}},")
    out += ["};", "inline constexpr size_t kResourcePackCount = sizeof(kResourcePacks) / sizeof(kResourcePacks[0]);", ""]
    header.write_text("\n".join(out), encoding="utf-8", newline="\n")
    for pack_id, folder, _, count, total in packs:
        print(f"  {pack_id}: {count} files, {total:,} bytes -> {folder}")
    print(f"{header}; release assets in {assets_dir} (upload them to {RELEASE_TAG})")


def main():
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("output", type=Path, nargs="?")
    parser.add_argument("--cache", type=Path, default=Path(__file__).resolve().parents[1] / ".pio" / "upstream-resources")
    parser.add_argument("--ota", type=Path, metavar="ASSETS_DIR",
                        help="write the over-the-air release assets here and regenerate ResourcePacks.generated.h")
    args = parser.parse_args()
    if args.ota:
        header = Path(__file__).resolve().parents[1] / "src" / "apps_local" / "appstore" / "ResourcePacks.generated.h"
        ota(args.ota, header, args.cache)
    if args.output:
        build(args.output, args.cache)
    if not args.ota and not args.output:
        parser.error("give an output zip, --ota ASSETS_DIR, or both")


if __name__ == "__main__":
    main()
