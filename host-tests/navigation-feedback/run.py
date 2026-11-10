"""Regression tests using production styles, input routing and buzzer driver."""
import argparse
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
parser = argparse.ArgumentParser()
parser.add_argument("--compiler", type=Path, required=True)
parser.add_argument("--snapshots", type=Path)
args = parser.parse_args()
compiler = [str(args.compiler)]
if args.compiler.name.lower().startswith("zig"):
    compiler += ["c++"]
common = compiler + ["-std=c++20", "-Wall", "-Wextra", "-Werror", "-Wno-missing-field-initializers"]
with tempfile.TemporaryDirectory() as folder:
    binary = Path(folder) / "buzzer.exe"
    subprocess.run(common + [
        "-Ihost-tests/navigation-feedback/stubs", "-Ilib/hal",
        "-Ifreeink-sdk/libs/hardware/Buzzer/include",
        "host-tests/navigation-feedback/test_buzzer.cpp",
        "freeink-sdk/libs/hardware/Buzzer/src/Buzzer.cpp", "-o", str(binary),
    ], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)
    binary = Path(folder) / "adapter.exe"
    subprocess.run(common + [
        "-Ihost-tests/navigation-feedback/adapter-stubs", "-Isrc/components",
        "-Ifreeink-sdk/libs/ui/FreeInkUI/include",
        "host-tests/navigation-feedback/test_adapter.cpp", "-o", str(binary),
    ], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)
    binary = Path(folder) / "repeat.exe"
    subprocess.run(common + [
        "-Itest/button_press_navigation/stubs", "-Isrc",
        "host-tests/navigation-feedback/test_repeat.cpp", "src/util/ButtonNavigator.cpp", "-o", str(binary),
    ], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)
    binary = Path(folder) / "screens.exe"
    subprocess.run(common + [
        "-Wno-comment", "-Ifreeink-sdk/libs/ui/FreeInkUI/include",
        "-Ifreeink-sdk/libs/assets/Icons/include", "-Ilib/EpdFont", "-Ilib/I18n",
        "host-tests/navigation-feedback/test_screens.cpp", "src/apps_local/ui/PixelMenu.cpp",
        "src/apps_local/ShelfScreen.cpp", "lib/I18n/I18n.cpp", "lib/I18n/I18nStrings.cpp",
        "freeink-sdk/libs/ui/FreeInkUI/src/FreeInkUI.cpp", "-o", str(binary),
    ], cwd=root, check=True)
    command = [str(binary)]
    if args.snapshots:
        args.snapshots.mkdir(parents=True, exist_ok=True)
        command += [str(args.snapshots.resolve())]
    subprocess.run(command, check=True)
    binary = Path(folder) / "menus.exe"
    sources = [
        "host-tests/navigation-feedback/test_menus.cpp", "src/apps_local/ui/PixelMenu.cpp",
        "src/apps_local/ShelfScreen.cpp", "src/apps_local/ui/GameFrontDoor.cpp",
        "src/apps_local/chess/ChessScreens.cpp", "src/apps_local/chess/ChessCore.cpp",
        "src/apps_local/chess/ChessEngine.cpp", "src/apps_local/chess/puzzles/ChessPuzzleScreens.cpp",
        "src/apps_local/chess/puzzles/ChessPuzzleCore.cpp", "src/apps_local/chess/puzzles/ChessPuzzleData.cpp",
        "src/apps_local/checkers/DraughtsScreens.cpp", "src/apps_local/checkers/DraughtsCore.cpp",
        "src/apps_local/go/GoScreens.cpp", "src/apps_local/go/GoCore.cpp",
        "src/apps_local/minesweeper/MinesweeperScreens.cpp",
        "src/apps_local/solitaire/SolitaireScreens.cpp", "src/apps_local/solitaire/SolitaireCore.cpp",
        "src/apps_local/hearts/HeartsScreens.cpp", "src/apps_local/hearts/HeartsCore.cpp",
        "src/apps_local/picross/PicrossScreens.cpp", "src/apps_local/picross/PicrossCore.cpp",
        "src/apps_local/wordle/WordleScreens.cpp", "src/apps_local/wordle/WordleCore.cpp",
        "src/apps_local/pocketgames/PocketGamesScreens.cpp", "src/apps_local/pocketgames/PocketGamesCore.cpp",
        "src/apps_local/pocketgames/OthelloScreens.cpp", "src/apps_local/cards/CardArt.cpp",
        "src/apps_local/pomodoro/PomodoroScreens.cpp", "src/apps_local/pomodoro/PomodoroCore.cpp",
        "src/apps_local/habits/HabitsScreens.cpp", "src/apps_local/habits/HabitsCore.cpp",
        "src/apps_local/flashcards/FlashSettingsScreens.cpp", "lib/I18n/I18n.cpp", "lib/I18n/I18nStrings.cpp",
        "freeink-sdk/libs/ui/FreeInkUI/src/FreeInkUI.cpp"]
    # Existing CardPanels.h has an unused local in both card-game builders.
    subprocess.run(common + ["-Wno-comment", "-Wno-unused-variable", "-Ifreeink-sdk/libs/ui/FreeInkUI/include",
        "-Ifreeink-sdk/libs/assets/Icons/include", "-Ilib/EpdFont", "-Ilib/I18n", *sources,
        "-o", str(binary)], cwd=root, check=True)
    subprocess.run([str(binary)], check=True)
