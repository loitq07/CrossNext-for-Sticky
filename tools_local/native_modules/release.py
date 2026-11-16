"""One-shot release of the firmware plus every native module built for it.

  python release.py [--publish <appstore checkout>]

1. compile all modules, refresh keep_symbols.txt (the firmware export list)
2. build the firmware (stamps a new module ABI, exports what modules import)
3. link every module against that build
4. with --publish: copy the modules to <checkout>/modules/<abi>/<folder>/ (the
   App Store fetches from there), commit, and push the `appstore` branch

A module only runs on the firmware it was linked with, so steps 2-4 always go
together: flash the firmware from this run and publish these modules with it.
"""

import os
import re
import shutil
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
WORKSPACE = os.path.abspath(os.path.join(ROOT, ".."))
PY = sys.executable
BUILD = os.path.join(WORKSPACE, "build-sticky.ps1")
OUT = os.path.join(WORKSPACE, "output", "modules")
ABI_HEADER = os.path.join(ROOT, "src", "apps_local", "modules", "NativeModuleAbi.generated.h")


def step(cmd, **kw):
    print("$", " ".join(cmd), flush=True)
    r = subprocess.run(cmd, **kw)
    if r.returncode != 0:
        sys.exit("failed: %s" % " ".join(cmd))


def update_catalog_sizes():
    """Store sizes from the previous link (a relink only changes the ABI word)."""
    path = os.path.join(ROOT, "src", "apps_local", "appstore", "AppStoreCatalog.h")
    text = open(path, encoding="utf-8").read()
    for folder in (os.listdir(OUT) if os.path.isdir(OUT) else []):
        for f in os.listdir(os.path.join(OUT, folder)):
            n = os.path.getsize(os.path.join(OUT, folder, f))
            text = re.sub(r'("%s", )\d+, "[^"]*"(, true)' % re.escape(f),
                          lambda m: '%s%d, "%.1f KB"%s' % (m.group(1), n, n / 1024, m.group(2)), text)
    open(path, "w", encoding="utf-8").write(text)


def main():
    publish = None
    if "--publish" in sys.argv:
        publish = sys.argv[sys.argv.index("--publish") + 1]

    step([PY, os.path.join(HERE, "build_module.py"), "objects"])
    update_catalog_sizes()
    step(["powershell", "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", BUILD])
    step([PY, os.path.join(HERE, "build_module.py"), "link"])
    abi = re.search(r"0x([0-9a-fA-F]+)", open(ABI_HEADER, encoding="utf-8").read()).group(1).lower()
    print("module ABI:", abi)

    if publish:
        dest = os.path.join(publish, "modules", abi)
        for folder in os.listdir(OUT):
            os.makedirs(os.path.join(dest, folder), exist_ok=True)
            for f in os.listdir(os.path.join(OUT, folder)):
                shutil.copy2(os.path.join(OUT, folder, f), os.path.join(dest, folder, f))
        step(["git", "-C", publish, "add", "modules/" + abi])
        step(["git", "-C", publish, "commit", "-m",
              "Native modules for firmware ABI " + abi + chr(10) + chr(10) +
              "Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>"])
        step(["git", "-C", publish, "push", "github", "appstore"])
        print("published modules/%s" % abi)


if __name__ == "__main__":
    main()
