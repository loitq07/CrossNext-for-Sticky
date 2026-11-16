import os
import sys
import subprocess

def build_app(app_dir, output_elf):
    sdk_dir = os.path.dirname(os.path.abspath(__file__))
    toolchain_bin = r"D:\Projects\Sticky\.platformio\packages\toolchain-xtensa-esp-elf\bin"
    gcc_path = os.path.join(toolchain_bin, "xtensa-esp32s3-elf-gcc.exe")
    gxx_path = os.path.join(toolchain_bin, "xtensa-esp32s3-elf-g++.exe")
    strip_path = os.path.join(toolchain_bin, "xtensa-esp32s3-elf-strip.exe")

    if not os.path.exists(gcc_path) or not os.path.exists(gxx_path):
        print(f"Error: Compiler toolchain not found at {toolchain_bin}")
        return False

    include_dir = os.path.join(sdk_dir, "include")

    src_files = []
    is_cpp = False
    for root, _, files in os.walk(app_dir):
        for f in files:
            if f.endswith(".c"):
                src_files.append(os.path.join(root, f))
            elif f.endswith(".cpp"):
                src_files.append(os.path.join(root, f))
                is_cpp = True

    if not src_files:
        print(f"Error: No source files (.c or .cpp) found in {app_dir}")
        return False

    compiler = gxx_path if is_cpp else gcc_path
    extra_flags = ["-std=gnu++2a", "-fno-exceptions", "-fno-rtti"] if is_cpp else []

    os.makedirs(os.path.dirname(output_elf), exist_ok=True)

    cmd = [
        compiler,
        "-mlongcalls",
        "-fPIC",
        "-shared",
        "-O2",
        "-Wall",
        "-Wextra",
        f"-I{include_dir}",
        "-o", output_elf,
    ] + extra_flags + src_files

    print(f"\n[BUILD] Compiling {os.path.basename(app_dir)}...")
    res = subprocess.run(cmd, capture_output=True, text=True)
    if res.returncode != 0:
        print("Compilation failed:")
        print(res.stderr)
        return False

    print(f"  -> Built binary: {output_elf} ({os.path.getsize(output_elf):,} bytes)")

    # Strip binary in-place for maximum efficiency on SD card
    subprocess.run([strip_path, "-s", output_elf])
    print(f"  -> Stripped binary: {output_elf} ({os.path.getsize(output_elf):,} bytes)")

    return True

def build_all(output_dir):
    sdk_dir = os.path.dirname(os.path.abspath(__file__))
    games_out = os.path.join(output_dir, "games")
    apps_out = os.path.join(output_dir, "apps")
    os.makedirs(games_out, exist_ok=True)
    os.makedirs(apps_out, exist_ok=True)

    targets = [
        ("sample_dice", os.path.join(games_out, "dice.elf")),
        ("game_2048", os.path.join(games_out, "2048.elf")),
        ("game_minesweeper", os.path.join(games_out, "minesweeper.elf")),
        ("game_connectfour", os.path.join(games_out, "connectfour.elf")),
        ("game_sudoku", os.path.join(games_out, "sudoku.elf")),
        ("game_checkers", os.path.join(games_out, "checkers.elf")),
        ("game_othello", os.path.join(games_out, "othello.elf")),
        ("game_battleship", os.path.join(games_out, "battleship.elf")),
        ("game_wordle", os.path.join(games_out, "wordle.elf")),
        ("game_picross", os.path.join(games_out, "picross.elf")),
        ("app_calculator", os.path.join(apps_out, "calculator.elf")),
        ("app_counter", os.path.join(apps_out, "counter.elf")),
        ("app_pomodoro", os.path.join(apps_out, "pomodoro.elf")),
        ("app_clock", os.path.join(apps_out, "clock.elf")),
        ("app_calendar", os.path.join(apps_out, "calendar.elf")),
        ("app_habits", os.path.join(apps_out, "habits.elf")),
        ("app_notes", os.path.join(apps_out, "notes.elf")),
        ("app_quotes", os.path.join(apps_out, "quotes.elf")),
        ("app_words", os.path.join(apps_out, "words.elf")),
        ("app_flashcards", os.path.join(apps_out, "flashcards.elf")),
        ("app_pet", os.path.join(apps_out, "pet.elf")),
        ("app_readingquest", os.path.join(apps_out, "readingquest.elf")),
    ]

    success_count = 0
    for subdir, out_elf in targets:
        app_path = os.path.join(sdk_dir, subdir)
        if os.path.isdir(app_path):
            if build_app(app_path, out_elf):
                success_count += 1
            else:
                print(f"Failed to build {subdir}")

    print(f"\n==========================================")
    print(f"Build Complete: {success_count}/{len(targets)} apps succeeded.")
    print(f"Output folder: {output_dir}")
    print(f"==========================================\n")
    return success_count == len(targets)

if __name__ == "__main__":
    out_dir = r"D:\Projects\Sticky\output\sdcard"
    if len(sys.argv) > 1:
        out_dir = sys.argv[1]
    success = build_all(out_dir)
    sys.exit(0 if success else 1)
