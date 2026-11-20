"""Build a built-in app's ORIGINAL source as a loadable native module (.elf).

The module is the app's own C++ (same Activity, screens, assets), compiled with
the firmware's exact flags and linked against the firmware image it will run
on (--just-symbols=firmware.elf). Everything shared -- GfxRenderer, FreeInkUI,
settings, storage, fonts -- is called in place in the firmware, so the module
carries only the app itself and behaves exactly like the built-in version.

Because it binds to absolute firmware addresses, a module only runs on the
firmware build it was linked against. The firmware stamps a random ABI id per
build (NativeModuleAbi.generated.h); the module embeds the same id and the
loader refuses a mismatch instead of jumping into the wrong code.

Two passes per release:

  1. objects   compile every module, write keep_symbols.txt (the firmware
               functions/data modules call, so --gc-sections keeps them)
  2. (build the firmware -- scripts_local/native_modules_fw.py reads the list)
  3. link      link each module against .pio/build/<env>/firmware.elf

Usage:
  python build_module.py objects [module ...]
  python build_module.py link    [module ...]
"""

import concurrent.futures
import json
import os
import re
import subprocess
import sys

NL = chr(10)
# Compiles are independent processes: run as many as there are cores.
JOBS = max(1, int(os.environ.get("MODULE_JOBS", os.cpu_count() or 4)))
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, "..", ".."))
ENV = os.environ.get("PIO_ENV", "gh_release_sticky")
TOOLBIN = os.path.abspath(os.path.join(ROOT, "..", ".platformio", "packages", "toolchain-xtensa-esp-elf", "bin"))
GXX = os.path.join(TOOLBIN, "xtensa-esp32s3-elf-g++.exe")
GCC = os.path.join(TOOLBIN, "xtensa-esp32s3-elf-gcc.exe")
NM = os.path.join(TOOLBIN, "xtensa-esp32s3-elf-nm.exe")
STRIP = os.path.join(TOOLBIN, "xtensa-esp32s3-elf-strip.exe")
OBJ_DIR = os.path.join(ROOT, ".pio", "native_modules", ENV)
TEMPLATE = os.path.join(HERE, "compile_template.json")
KEEP = os.path.join(HERE, "keep_symbols.txt")
ABI_HEADER = os.path.join(ROOT, "src", "apps_local", "modules", "NativeModuleAbi.generated.h")
FIRMWARE_ELF = os.path.join(ROOT, ".pio", "build", ENV, "firmware.elf")
OUT_DIR = os.environ.get("MODULE_OUT", os.path.abspath(os.path.join(ROOT, "..", "output", "modules")))

# name -> (shelf folder, source dirs under src/, descriptor title, create expression, header)
MODULES = {
    "2048": ("games", ["apps_local/game2048"], "2048", "Game2048Activity::create",
             "apps_local/game2048/Game2048Activity.h"),
    "minesweeper": ("games", ["apps_local/minesweeper"], "MINESWEEPER", "MinesweeperActivity::create",
                    "apps_local/minesweeper/MinesweeperActivity.h"),
    "picross": ("games", ["apps_local/picross"], "PICROSS", "PicrossActivity::create",
                "apps_local/picross/PicrossActivity.h"),
    "sudoku": ("games", ["apps_local/sudoku"], "SUDOKU", "SudokuActivity::create",
               "apps_local/sudoku/SudokuActivity.h"),
    "wordle": ("games", ["apps_local/wordle"], "WORDLE", "WordleActivity::create",
               "apps_local/wordle/WordleActivity.h"),
    "connectfour": ("games", ["apps_local/connectfour"], "CONNECT FOUR", "ConnectFourActivity::create",
                    "apps_local/connectfour/ConnectFourActivity.h"),
    "checkers": ("games", ["apps_local/checkers"], "CHECKERS", "DraughtsActivity::create",
                 "apps_local/checkers/DraughtsActivity.h"),
    "battleship": ("games", ["apps_local/battleship"], "BATTLESHIP", "BattleshipActivity::create",
                   "apps_local/battleship/BattleshipActivity.h"),
    "othello": ("games", ["apps_local/pocketgames"], "OTHELLO", "PocketGamesActivity::createOthello",
                "apps_local/pocketgames/PocketGamesActivity.h"),
    "chess": ("games", ["apps_local/chess", "apps_local/chess/puzzles"], "CHESS", "ChessActivity::create",
              "apps_local/chess/ChessActivity.h"),
    "go": ("games", ["apps_local/go", "apps_local/go/michi"], "GO", "GoActivity::create",
           "apps_local/go/GoActivity.h"),
    "theintercept": ("games", ["apps_local/intercept"], "THE INTERCEPT", "InterceptActivity::create",
                     "apps_local/intercept/InterceptActivity.h"),
    # cards/ is used by these two games only, so each carries its own copy.
    "solitaire": ("games", ["apps_local/solitaire", "apps_local/cards"], "SOLITAIRE", "SolitaireActivity::create",
                  "apps_local/solitaire/SolitaireActivity.h"),
    "hearts": ("games", ["apps_local/hearts", "apps_local/cards"], "HEARTS", "HeartsActivity::create",
               "apps_local/hearts/HeartsActivity.h"),
    "murdle": ("games", ["apps_local/murdle"], "MURDLE", "MurdleActivity::create",
               "apps_local/murdle/MurdleActivity.h"),
    "carnivoro": ("games", ["apps_local/carnivoro"], "CARNIVORO", "CarnivoroActivity::create",
                  "apps_local/carnivoro/CarnivoroActivity.h"),
    "pet": ("apps", ["apps_local/pet"], "PET", "PetActivity::create", "apps_local/pet/PetActivity.h"),
    "readingquest": ("apps", ["apps_local/quest"], "READING QUEST", "ReadingQuestActivity::create",
                     "apps_local/quest/ReadingQuestActivity.h"),
    "habits": ("apps", ["apps_local/habits"], "HABITS", "HabitsActivity::create",
               "apps_local/habits/HabitsActivity.h"),
    "pomodoro": ("apps", ["apps_local/pomodoro"], "POMODORO", "PomodoroActivity::create",
                 "apps_local/pomodoro/PomodoroActivity.h"),
    "focus": ("apps", ["apps_local/focus", "apps_local/pomodoro"], "FOCUS", "FocusActivity::create",
              "apps_local/focus/FocusActivity.h"),
    "calculator": ("apps", ["apps_local/calculator", "lib/decNumber/src"], "CALCULATOR", "CalculatorActivity::create",
                   "apps_local/calculator/CalculatorActivity.h"),
    "instapaper": ("apps", ["apps_local/instapaper"], "INSTAPAPER", "InstapaperActivity::create",
                   "apps_local/instapaper/InstapaperActivity.h"),
    "voicenotes": ("apps", ["apps_local/voicenotes"], "VOICE NOTES", "voicenotes::VoiceNotesActivity::create",
                   "apps_local/voicenotes/VoiceNotesActivity.h"),
    "connections": ("games", ["apps_local/connections"], "CONNECTIONS", "ConnectionsActivity::create",
                    "apps_local/connections/ConnectionsActivity.h"),
    "xkcd": ("apps", ["apps_local/xkcd"], "XKCD", "XkcdActivity::create",
             "apps_local/xkcd/XkcdActivity.h"),
    "hackernews": ("apps", ["apps_local/hackernews"], "HACKER NEWS", "HackerNewsActivity::create",
                   "apps_local/hackernews/HackerNewsActivity.h"),
    "wikipedia": ("apps", ["apps_local/wikipedia"], "WIKIPEDIA", "WikipediaActivity::create",
                  "apps_local/wikipedia/WikipediaActivity.h"),
    "trivia": ("games", ["apps_local/trivia"], "TRIVIA", "TriviaActivity::create",
               "apps_local/trivia/TriviaActivity.h"),
    "underhand": ("games", ["apps_local/underhand"], "UNDERHAND", "UnderhandActivity::create",
                  "apps_local/underhand/UnderhandActivity.h"),
    "wavelength": ("games", ["apps_local/wavelength"], "WAVELENGTH", "WavelengthActivity::create",
                   "apps_local/wavelength/WavelengthActivity.h"),
    "forehead": ("games", ["apps_local/forehead"], "FOREHEAD", "ForeheadActivity::create",
                 "apps_local/forehead/ForeheadActivity.h"),
    "insider": ("games", ["apps_local/insider"], "INSIDER", "InsiderActivity::create",
                "apps_local/insider/InsiderActivity.h"),
    "knucklebones": ("games", ["apps_local/knucklebones", "apps_local/link"], "KNUCKLEBONES", "KnucklebonesActivity::create",
                     "apps_local/knucklebones/KnucklebonesActivity.h"),
    "yahtzee": ("games", ["apps_local/yahtzee", "apps_local/link"], "YAHTZEE", "YahtzeeActivity::create",
                "apps_local/yahtzee/YahtzeeActivity.h"),
    "seasalt": ("games", ["apps_local/seasalt", "apps_local/link"], "SEA SALT", "SeaSaltActivity::create",
                "apps_local/seasalt/SeaSaltActivity.h"),
    "toybattle": ("games", ["apps_local/toybattle", "apps_local/link"], "TOY BATTLE", "ToyBattleActivity::create",
                  "apps_local/toybattle/ToyBattleActivity.h"),
    "tictactoe": ("games", ["apps_local/pocketplay/games/tictactoe", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "TIC-TAC-TOE",
                  "pocketplay::createTicTacToe", "apps_local/pocketplay/games/tictactoe/tictactoe_activity.h"),
    "dungeon": ("games", ["apps_local/pocketplay/games/dungeon", "apps_local/pocketplay/bridge",
                         "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "D&DIAGRAMS",
               "pocketplay::createDungeon", "apps_local/pocketplay/games/dungeon/dungeon_activity.h"),
    "oneclue": ("games", ["apps_local/pocketplay/games/oneclue", "apps_local/pocketplay/bridge",
                         "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "ONE CLUE",
               "pocketplay::createOneClue", "apps_local/pocketplay/games/oneclue/oneclue_activity.h"),
    "lightsout": ("games", ["apps_local/pocketplay/games/lightsout", "apps_local/pocketplay/bridge",
                           "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "LIGHTS OUT",
                 "pocketplay::createLightsOut", "apps_local/pocketplay/games/lightsout/lightsout_activity.h"),
    "suguru": ("games", ["apps_local/pocketplay/games/suguru", "apps_local/pocketplay/bridge",
                        "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "SUGURU",
              "pocketplay::createSuguru", "apps_local/pocketplay/games/suguru/suguru_activity.h"),
    "sokoban": ("games", ["apps_local/pocketplay/games/sokoban", "apps_local/pocketplay/bridge",
                         "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "SOKOBAN",
               "pocketplay::createSokoban", "apps_local/pocketplay/games/sokoban/sokoban_activity.h"),
    "queens": ("games", ["apps_local/pocketplay/games/queens", "apps_local/pocketplay/bridge",
                        "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "QUEENS",
              "pocketplay::createQueens", "apps_local/pocketplay/games/queens/queens_activity.h"),
    "symboldice": ("games", ["apps_local/pocketplay/games/symboldice", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "SYMBOL DICE",
                  "pocketplay::createSymbolDice", "apps_local/pocketplay/games/symboldice/symboldice_activity.h"),
    "pegsolitaire": ("games", ["apps_local/pocketplay/games/pegsolitaire", "apps_local/pocketplay/bridge",
                              "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "PEG SOLITAIRE",
                    "pocketplay::createPegSolitaire", "apps_local/pocketplay/games/pegsolitaire/pegsolitaire_activity.h"),
    "textadv": ("games", ["apps_local/pocketplay/games/textadv", "apps_local/pocketplay/bridge",
                         "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "TEXT ADVENTURES",
               "pocketplay::createTextAdv", "apps_local/pocketplay/games/textadv/textadv_activity.h"),
    "wordladder": ("games", ["apps_local/pocketplay/games/wordladder", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "WORD LADDER",
                  "pocketplay::createWordLadder", "apps_local/pocketplay/games/wordladder/wordladder_activity.h"),
    "diceroller": ("games", ["apps_local/pocketplay/games/diceroller", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "DICE ROLLER",
                  "pocketplay::createDiceRoller", "apps_local/pocketplay/games/diceroller/diceroller_activity.h"),
    "nonogram": ("games", ["apps_local/pocketplay/games/nonogram", "apps_local/pocketplay/bridge",
                          "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "NONOGRAM",
                "pocketplay::createNonogram", "apps_local/pocketplay/games/nonogram/nonogram_activity.h"),
    "kakuro": ("games", ["apps_local/pocketplay/games/kakuro", "apps_local/pocketplay/bridge",
                        "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "KAKURO",
              "pocketplay::createKakuro", "apps_local/pocketplay/games/kakuro/kakuro_activity.h"),
    "bingocaller": ("games", ["apps_local/pocketplay/games/bingocaller", "apps_local/pocketplay/bridge",
                             "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "BINGO CALLER",
                   "pocketplay::createBingoCaller", "apps_local/pocketplay/games/bingocaller/bingocaller_activity.h"),
    "islandmaps": ("games", ["apps_local/pocketplay/games/islandmaps", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "ISLAND MAPS",
                  "pocketplay::createIslandMaps", "apps_local/pocketplay/games/islandmaps/islandmaps_activity.h"),
    "thinkalike": ("games", ["apps_local/pocketplay/games/thinkalike", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "THINK ALIKE",
                  "pocketplay::createThinkAlike", "apps_local/pocketplay/games/thinkalike/thinkalike_activity.h"),
    "authoritytracker": ("games", ["apps_local/pocketplay/games/authoritytracker", "apps_local/pocketplay/bridge",
                                  "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "AUTHORITY TRACKER",
                        "pocketplay::createAuthorityTracker", "apps_local/pocketplay/games/authoritytracker/authoritytracker_activity.h"),
    "gridgambit": ("games", ["apps_local/pocketplay/games/gridgambit", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "GRID GAMBIT",
                  "pocketplay::createGridGambit", "apps_local/pocketplay/games/gridgambit/gridgambit_activity.h"),
    "chesspuzzles": ("games", ["apps_local/pocketplay/games/chesspuzzles", "apps_local/pocketplay/bridge",
                              "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "CHESS PUZZLES",
                    "pocketplay::createChessPuzzles", "apps_local/pocketplay/games/chesspuzzles/chesspuzzles_activity.h"),
    "polyominopanic": ("games", ["apps_local/pocketplay/games/polyominopanic", "apps_local/pocketplay/bridge",
                                "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "POLYOMINO PANIC",
                      "pocketplay::createPolyominoPanic", "apps_local/pocketplay/games/polyominopanic/polyominopanic_activity.h"),
    "crowncrest": ("games", ["apps_local/pocketplay/games/crowncrest", "apps_local/pocketplay/bridge",
                            "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "CROWN & CREST",
                  "pocketplay::createCrownCrest", "apps_local/pocketplay/games/crowncrest/crowncrest_activity.h"),
    "lastpiece": ("games", ["apps_local/pocketplay/games/lastpiece", "apps_local/pocketplay/bridge",
                           "apps_local/pocketplay/core", "apps_local/pocketplay/ui"], "LAST PIECE",
                 "pocketplay::createLastPiece", "apps_local/pocketplay/games/lastpiece/lastpiece_activity.h"),
}


def compile_template():
    """Flags of one firmware app TU (a gcc response-file string). Shelf.cpp is
    always in the firmware, so its entry exists in compile_commands.json
    (`pio run -t compiledb`). Cached locally; the cache holds machine paths
    and is not committed."""
    if not os.path.exists(TEMPLATE):
        cc = json.load(open(os.path.join(ROOT, "compile_commands.json"), encoding="utf-8"))
        for e in cc:
            if e["file"].replace(chr(92), "/").endswith("src/apps_local/Shelf.cpp"):
                json.dump(e, open(TEMPLATE, "w", encoding="utf-8"))
                break
        else:
            sys.exit("no template entry in compile_commands.json; run `pio run -t compiledb` first")
    e = json.load(open(TEMPLATE, encoding="utf-8"))
    cmd = e["command"]
    cmd = cmd.split(" ", 1)[1]  # drop the compiler
    cmd = re.sub(r"(^| )-o \S+", " ", cmd)
    cmd = re.sub(r"(^| )-c( |$)", " ", cmd)
    cmd = cmd.replace(e["file"], " ")
    # gcc response files treat backslash as an escape: keep \" (quoted defines),
    # turn every path separator into '/'.
    cmd = re.sub(r'\\(?!")', "/", cmd)
    # Relative include dirs are relative to the PlatformIO project directory.
    base = e["directory"].replace(chr(92), "/")
    cmd = re.sub(r"(^| )-I(?![A-Za-z]:/|/)(\S+)", lambda m: m.group(1) + "-I" + base + "/" + m.group(2), cmd)
    return cmd


def q(a):
    a = a.replace(chr(92), "/")
    return '"%s"' % a if " " in a else a


def run(compiler, flags, args, rsp_name):
    rsp = os.path.join(OBJ_DIR, rsp_name)
    with open(rsp, "w", encoding="utf-8") as f:
        f.write(flags + " " + " ".join(q(a) for a in args))
    r = subprocess.run([compiler, "@" + rsp], capture_output=True, text=True)
    if r.returncode != 0:
        print(r.stdout[-6000:])
        print(r.stderr[-6000:])
        sys.exit("command failed: %s" % rsp)
    if r.stderr.strip():
        print(r.stderr[-3000:])


def abi_value():
    text = open(ABI_HEADER, encoding="utf-8").read()
    return re.search(r"STICKY_MODULE_ABI\s+(0x[0-9a-fA-F]+)", text).group(1)


def stub_source(name):
    folder, _, title, create, header = MODULES[name]
    return f'''// Generated by build_module.py -- the module's descriptor.
#include "{header}"
#include "apps_local/modules/NativeModule.h"

extern "C" {{
int sticky_native_module_entry(int, char**) {{ return 0; }}
const StickyNativeModule* sticky_native_module_get() {{
  static const StickyNativeModule desc = {{kStickyNativeModuleMagic, STICKY_MODULE_ABI, "{title}", &{create}}};
  return &desc;
}}
}}
'''


def sources(name):
    out = []
    for d in MODULES[name][1]:
        # "lib/..." is a library only this module uses (carried inside it, so
        # the firmware need not link it); anything else is under src/.
        base = os.path.join(ROOT, d) if d.startswith("lib/") else os.path.join(ROOT, "src", d)
        for f in sorted(os.listdir(base)):
            if f.endswith(".cpp") or f.endswith(".c"):
                out.append(os.path.join(base, f))
    return out


def dep_paths(dep):
    """Prerequisites listed in a gcc -MMD file: "target: a b <backslash-newline> c",
    with spaces inside a path written as backslash-space."""
    with open(dep, encoding="utf-8") as f:
        text = f.read().replace("\\" + NL, " ").replace("\\ ", "\t")
    tokens = text.split(" ")
    tokens = [t.strip(NL + "\r") for t in tokens if t.strip(NL + "\r")]
    while tokens and not tokens[0].endswith(":"):
        tokens.pop(0)  # the target itself (a Windows path may hold a drive colon)
    # Relative entries (forced includes) are relative to the project directory.
    return [os.path.join(ROOT, t.replace("\t", " ")) for t in tokens[1:]]


CXX_ONLY = re.compile(r"(^| )(-std=gnu\+\+\w+|-fno-rtti|-fuse-cxa-atexit|-Wno-bidi-chars)(?= |$)")


def extra_inc_for(name):
    extra = []
    has_pp = any("pocketplay" in d for d in MODULES[name][1])
    if has_pp:
        pp_root = os.path.join(ROOT, "src", "apps_local", "pocketplay").replace("\\", "/")
        extra.append("-I" + pp_root)
        extra.append("-I" + pp_root + "/core")
        extra.append("-I" + pp_root + "/ui")
        extra.append("-I" + pp_root + "/bridge")
    for d in MODULES[name][1]:
        if d.startswith("lib/"):
            extra.append("-I" + os.path.join(ROOT, d).replace("\\", "/"))
        else:
            extra.append("-I" + os.path.join(ROOT, "src", d).replace("\\", "/"))
    seen = set()
    return [x for x in extra if not (x in seen or seen.add(x))]


def objects(names):
    """Compile modules; record what each imports from the firmware.

    keep_symbols.txt is the firmware's export list: functions/data a module
    calls (resolved by name at load time), plus the module's weak data objects
    -- function-local statics and guards of inline functions (UITheme's
    instance, cached fonts...). Those must be the firmware's single copy, not a
    second one inside the module, so the module exports them and the loader
    lets the firmware's definition win.
    """
    flags = compile_template()
    all_keep = set()
    jobs, module_objs = [], {}
    for name in names:
        mdir = os.path.join(OBJ_DIR, name)
        os.makedirs(mdir, exist_ok=True)
        stub = os.path.join(mdir, "module_descriptor.cpp")
        stub_text = stub_source(name)
        if not os.path.exists(stub) or open(stub, encoding="utf-8").read() != stub_text:
            open(stub, "w", encoding="utf-8").write(stub_text)
        wanted = {os.path.basename(src) + ".o" for src in sources(name) + [stub]}
        for f in os.listdir(mdir):
            # Objects of sources that no longer exist would be linked in.
            if f.endswith(".cpp.o") or f.endswith(".c.o"):
                if f not in wanted and f != "module_descriptor.cpp.o":
                    os.remove(os.path.join(mdir, f))
        extra_inc = extra_inc_for(name)
        module_objs[name] = []
        for src in sources(name) + [stub]:
            obj = os.path.join(mdir, os.path.basename(src) + ".o")
            is_c = src.endswith(".c")
            jobs.append((GCC if is_c else GXX, CXX_ONLY.sub(" ", flags) if is_c else flags,
                         ["-DSTICKY_MODULE_ABI=0", "-mtext-section-literals", "-fPIC", "-MMD", "-MF", obj + ".d"]
                         + extra_inc + ["-c", src, "-o", obj],
                         os.path.join(name, os.path.basename(src) + ".cc.rsp")))
            module_objs[name].append(obj)

    def up_to_date(job):
        """Reuse an object when its flags, source and every header it read are unchanged."""
        obj, rsp = job[2][-1], os.path.join(OBJ_DIR, job[3])
        dep = obj + ".d"
        if os.environ.get("MODULE_REBUILD") or not all(os.path.exists(x) for x in (obj, dep, rsp)):
            return False
        if open(rsp, encoding="utf-8").read() != job[1] + " " + " ".join(q(a) for a in job[2]):
            return False
        built = os.path.getmtime(obj)
        try:
            return all(os.path.getmtime(x) <= built for x in dep_paths(dep))
        except OSError:  # a header moved or was deleted
            return False

    def compile_one(job):
        print("  CC", os.path.relpath(job[2][-3], ROOT), flush=True)
        run(*job)

    stale = [j for j in jobs if not up_to_date(j)]
    print(f"compiling {len(stale)} of {len(jobs)} sources with {JOBS} jobs", flush=True)
    with concurrent.futures.ThreadPoolExecutor(JOBS) as pool:
        for f in [pool.submit(compile_one, j) for j in stale]:
            f.result()  # re-raises the first failed compile

    def scan(name):
        undefined, defined, shared = set(), set(), set()
        for obj in module_objs[name]:
            out = subprocess.run([NM, "-g", obj], capture_output=True, text=True).stdout
            for line in out.splitlines():
                parts = line.split()
                if len(parts) == 2 and parts[0] in ("U", "w", "v"):
                    undefined.add(parts[1])
                elif len(parts) == 3:
                    defined.add(parts[2])
                    if parts[1] in ("V", "u"):
                        shared.add(parts[2])
        return name, undefined - defined - {"__dso_handle"}, shared

    with concurrent.futures.ThreadPoolExecutor(JOBS) as pool:
        for name, imports, shared in pool.map(scan, names):
            mdir = os.path.join(OBJ_DIR, name)
            open(os.path.join(mdir, "shared_data.txt"), "w", encoding="utf-8").write(NL.join(sorted(shared)) + NL)
            all_keep |= {"U " + x for x in imports} | {"S " + x for x in shared}
            print(f"  {name}: {len(imports)} imports, {len(shared)} shared data objects")
    existing = set()
    # A partial build adds to the list; a full one replaces it, so symbols no
    # module imports any more stop being kept in the firmware.
    if os.path.exists(KEEP) and set(names) != set(MODULES):
        existing = {l.strip() for l in open(KEEP, encoding="utf-8") if l.strip()}
    merged = sorted(existing | all_keep, key=lambda l: l[2:])
    # A name imported by one module wins over the same name shared by another.
    names_u = {l[2:] for l in merged if l.startswith("U ")}
    merged = [l for l in merged if l.startswith("U ") or l[2:] not in names_u]
    open(KEEP, "w", encoding="utf-8").write(NL.join(merged) + NL)
    print(f"keep list: {len(merged)} symbols")


def link(names):
    flags = compile_template()
    abi = abi_value()

    def link_one(name):
        folder = MODULES[name][0]
        mdir = os.path.join(OBJ_DIR, name)
        stub = os.path.join(mdir, "module_descriptor.cpp")
        stub_obj = os.path.join(mdir, "module_descriptor.cpp.o")
        extra_inc = extra_inc_for(name)
        # Recompile only the descriptor with the real ABI of this firmware.
        run(GXX, flags, ["-DSTICKY_MODULE_ABI=" + abi, "-mtext-section-literals", "-fPIC"] + extra_inc + ["-c", stub, "-o", stub_obj],
            name + ".stub.rsp")
        # crtbegin is not linked (-nostartfiles); static destructors registered
        # with __cxa_atexit still need a DSO handle to name this module.
        dso = os.path.join(mdir, "module_dso.c")
        open(dso, "w", encoding="utf-8").write("void *__dso_handle = &__dso_handle;" + NL)
        run(GCC, "", ["-mlongcalls", "-fPIC", "-Os", "-c", dso, "-o", dso + ".o"], name + ".dso.rsp")
        objs = [os.path.join(mdir, f) for f in sorted(os.listdir(mdir)) if f.endswith(".o")]
        shared = [l.strip() for l in open(os.path.join(mdir, "shared_data.txt"), encoding="utf-8") if l.strip()]
        vers = os.path.join(mdir, "exports.map")
        with open(vers, "w", encoding="utf-8") as f:
            f.write("{ global:" + NL + "  sticky_native_module_get;" + NL + "  sticky_native_module_entry;" + NL)
            for sym in shared:
                f.write('  "%s";' % sym + NL)
            f.write("local: *; };" + NL)
        out_dir = os.path.join(OUT_DIR, folder)
        os.makedirs(out_dir, exist_ok=True)
        out = os.path.join(out_dir, name + ".elf")
        run(GXX, "", ["-mlongcalls", "-shared", "-nostartfiles", "-nostdlib",
                      "-Wl,--version-script=" + vers, "-Wl,-Bsymbolic-functions",
                      "-Wl,-e,sticky_native_module_entry", "-Wl,--undefined=sticky_native_module_get",
                      "-Wl,-z,notext", "-Wl,-Map=" + os.path.join(mdir, "module.map"),
                      "-o", out] + objs, name + ".ld.rsp")
        subprocess.run([STRIP, "--strip-debug", "--strip-unneeded", out])
        # Trailer read by the App Store (native_module::fileAbi) to spot a module
        # built for another firmware without loading it. Past the section
        # headers, so the ELF loader never sees it.
        with open(out, "ab") as f:
            f.write(b"SMAB" + int(abi, 16).to_bytes(4, "little"))
        print(f"  LD {out} ({os.path.getsize(out):,} bytes, abi {abi})", flush=True)

    with concurrent.futures.ThreadPoolExecutor(JOBS) as pool:
        for f in [pool.submit(link_one, n) for n in names]:
            f.result()


if __name__ == "__main__":
    if len(sys.argv) < 2 or sys.argv[1] not in ("objects", "link"):
        sys.exit(__doc__)
    names = sys.argv[2:] or list(MODULES)
    os.makedirs(OBJ_DIR, exist_ok=True)
    {"objects": objects, "link": link}[sys.argv[1]](names)
