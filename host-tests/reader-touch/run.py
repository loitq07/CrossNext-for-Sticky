"""Run with python host-tests/reader-touch/run.py; CXX may name a compiler plus arguments."""
import os
from pathlib import Path
import shlex
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory() as directory:
    executable = Path(directory) / ("reader-touch.exe" if os.name == "nt" else "reader-touch")
    subprocess.run([
        *shlex.split(os.environ.get("CXX", "c++")), "-std=c++20", "-Wall", "-Wextra", "-Werror",
        "-include", str(root / "test/chapter_html_slim_parser/stubs/Print.h"),
        "-I", str(root / "test/clipping_store/stubs"),
        "-I", str(root / "test/library_builder/stubs"),
        "-I", str(root / "src"), "-I", str(root / "lib/Serialization"), "-I", str(root / "lib/Utf8"),
        str(root / "host-tests/reader-touch/test_reader_touch.cpp"),
        str(root / "src/ClippingStore.cpp"), str(root / "lib/Utf8/Utf8.cpp"), "-o", str(executable),
    ], check=True)
    subprocess.run([str(executable)], check=True)
print("Reader touch and atomic highlight edit checks passed")
