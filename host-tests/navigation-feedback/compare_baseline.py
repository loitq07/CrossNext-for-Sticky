"""Compare the wider existing UI suite with HEAD without changing the checkout."""
import argparse
from collections import Counter
import io
import shutil
from pathlib import Path
import subprocess
import tarfile
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--compiler", type=Path, required=True)
parser.add_argument("--output", type=Path, required=True)
args = parser.parse_args()
output = args.output.resolve()
output.mkdir(parents=True, exist_ok=True)
compiler = [str(args.compiler.resolve())]
if args.compiler.name.lower().startswith("zig"):
    compiler += ["c++"]
sdk = root / "freeink-sdk"

def run_suite(source, binary, name):
    command = compiler + ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-comment",
        "-Wno-missing-field-initializers", "-I" + str(sdk / "libs/ui/FreeInkUI/include"),
        "-I" + str(sdk / "libs/assets/Icons/include"), "-I" + str(source / "lib/EpdFont"),
        "-I" + str(source / "lib/I18n")]
    command += [str(source / path) for path in [
        "host-tests/settings-redesign/test_screens.cpp", "src/apps_local/ui/PixelMenu.cpp",
        "src/apps_local/ShelfScreen.cpp", "lib/I18n/I18n.cpp", "lib/I18n/I18nStrings.cpp"]]
    command += [str(sdk / "libs/ui/FreeInkUI/src/FreeInkUI.cpp"), "-o", str(binary)]
    subprocess.run(command, check=True)
    result = subprocess.run([str(binary)], capture_output=True, text=True)
    (output / (name + ".log")).write_text(result.stdout + result.stderr, encoding="utf-8")
    print(name, result.stdout.strip().splitlines()[-1])
    return Counter(line.split(": ", 1)[1] for line in result.stdout.splitlines() if line.startswith("FAIL "))

with tempfile.TemporaryDirectory() as folder:
    temporary = Path(folder)
    baseline = temporary / "baseline"
    baseline.mkdir()
    archive = subprocess.run(["git", "archive", "HEAD", "src", "lib", "host-tests/settings-redesign"],
                             cwd=root, check=True, capture_output=True).stdout
    with tarfile.open(fileobj=io.BytesIO(archive)) as tar:
        tar.extractall(baseline, filter="data")
    for name in ("I18nKeys.h", "I18nStrings.h", "I18nStrings.cpp"):
        shutil.copy2(root / "lib/I18n" / name, baseline / "lib/I18n" / name)
    before = run_suite(baseline, temporary / "baseline.exe", "baseline-settings")
    after = run_suite(root, temporary / "current.exe", "current-settings")
    added = after - before
    if added:
        raise SystemExit("Additional failures: " + repr(added))
    print("PASS wider UI suite: no additional failures versus HEAD")
    probe = temporary / "probe.cpp"
    probe.write_text('#include "HalBuzzer.h"\nint main() { HalBuzzer::click(); HalBuzzer::beep(); '
                     'return pwmtest::tones == 2 ? 0 : 1; }\n', encoding="utf-8")
    command = compiler + ["-std=c++20", "-I" + str(root / "host-tests/navigation-feedback/stubs"),
        "-I" + str(baseline / "lib/hal"), "-I" + str(sdk / "libs/hardware/Buzzer/include"), str(probe),
        str(sdk / "libs/hardware/Buzzer/src/Buzzer.cpp"), "-o", str(temporary / "probe.exe")]
    subprocess.run(command, check=True)
    assert subprocess.run([str(temporary / "probe.exe")]).returncode == 1
    print("PASS root cause reproduced: original HalBuzzer loses beep after click")
