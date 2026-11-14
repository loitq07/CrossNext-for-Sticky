<h1 align="center">
  <img src="site/assets/logo.svg#gh-light-mode-only" width="72" alt="CrossNext"><img src="site/assets/logo-dark.svg#gh-dark-mode-only" width="72" alt="CrossNext"><br>
  CrossNext for Seeed reTerminal Sticky
</h1>

<p align="center">
  A customized port of <b>CrossNext</b> bringing e-reading, productivity tools, study workflows, and games to the <b>Seeed reTerminal Sticky</b> (ESP32-S3 E-Ink).
</p>

<p align="center">
  <a href="https://discord.gg/kChkaFdEyB"><img src="https://img.shields.io/badge/Discord-Join%20Community-5865F2?style=for-the-badge&logo=discord&logoColor=white" alt="Discord"></a>
</p>

<p align="center">
  <a href="#-credits--acknowledgements">Credits</a> •
  <a href="#-key-features--hardware-adaptations">Features</a> •
  <a href="#-flashing--installation">Installation</a> •
  <a href="#-community--support">Community</a> •
  <a href="#-building-from-source">Build from Source</a> •
  <a href="#-license--attribution">License</a>
</p>

---

## 🌟 Credits & Acknowledgements

This distribution is built on top of incredible open-source foundations. Sincere gratitude and full credit go to:

- **[manuelx900](https://github.com/manuelx900)** for **[CrossNext](https://github.com/manuelx900/CrossNext)**: The core firmware, Toybox UI architecture, PET virtual companion, study tools, and e-ink games.
- **[CrossPoint Reader organization](https://github.com/crosspoint-reader/crosspoint-reader)** for **CrossPoint**: The underlying open-source e-reader operating system for ESP32.
- **[Seeed Studio](https://www.seeedstudio.com/)** for the **reTerminal Sticky** hardware platform (ESP32-S3, 3.7" E-Ink, touch panel, Wi-Fi/BLE) and the [freeink-sdk](https://github.com/Seeed-Studio).
- **[ma-r-s](https://github.com/ma-r-s/crossplay)** ([CrossPlay](https://github.com/ma-r-s/crossplay)) and community contributors ([CrossInk](https://github.com/uxjulia/CrossInk), [CrossDiTo](https://github.com/dito94/CrossDiTo), [VCodex](https://github.com/franssjz/cpr-vcodex)).

---

## ✨ Key Features & Hardware Adaptations

### 1. Hardware Adaptation for Seeed reTerminal Sticky
- **Hardware-tailored Settings**: Cleaned up system menus to hide controls not present on the reTerminal Sticky hardware (such as dual frontlight warmth/brightness sliders).
- **Power & Sleep Management**: Optimized wakeup routines and USB power charging behavior for the Sticky's hardware power rail.
- **Reader Home Navigation Fix**: Corrected reader activity stack behavior so pressing Home reliably returns to the shelf.

### 2. Multi-byte UTF-8 Typography Engine (`PixelText.h`)
- **UTF-8 Natural Casing**: Replaced single-byte ASCII title/sentence-casing in `copyNatural()` and `copyCaps()` with a universal multi-byte UTF-8 engine supporting Latin-1 Supplement, Latin Extended-A, and Latin Extended Additional blocks.
- **Clean Text Rendering**: Eliminates irregular capitalization artifacts across app lists, headers, and games (e.g. shelf headers now correctly capitalize full Unicode sequences, and list items display in clean Title Case).
- **Standard Keyboard Controls**: Keeps global control keys intact (`OK`, `Space`, `Shift`, `Del`, `ABC`, `?123`) without mistranslations on virtual input keyboards.

### 3. Comprehensive Localization
- Full interface localization across all 4,063 system, reader, tool, and game keys.
- Supported languages include **English**, **Spanish**, and **Vietnamese**, selectable directly from **Settings → System → Language**.

### 4. Read, Study & Play
- **Reader**: EPUB, TXT, Markdown, and XTC/XTCH support with customizable typography, bookmarks, and reading progress.
- **Study Suite**: Flashcards with spaced repetition (FSRS), vocabulary review, Anki import, and dictionary lookup.
- **PET Companion**: Virtual reading pet that grows with your reading goals, furnishing rooms, and tracking reading streaks.
- **Games Shelf**: 16 e-ink optimized games (Chess, Checkers, Go, Minesweeper, Picross, Wordle, 2048, The Intercept, Battleship, Solitaire, Hearts, Murdle, Sudoku, and more).

---

## 🚀 Flashing & Installation

Pre-built binaries are available on the [Releases page](https://github.com/loitq07/CrossNext-for-Sticky/releases).

### Release Assets
- `CrossNext-Sticky-v1.4.0-factory.bin`: Full factory image flashed at offset `0x0000` (contains bootloader, partition table, boot_app0, and application).
- `CrossNext-Sticky-v1.4.0-app.bin`: Application partition only, flashed at offset `0x10000`.
- `CrossNext-Sticky-v1.4.0.zip`: Complete distribution package including binaries, patch, and checksums.

### Flashing via `esptool.py` (Recommended)

Connect your reTerminal Sticky to your computer via USB-C and identify the serial port (e.g. `COM9` on Windows or `/dev/ttyUSB0` on Linux/macOS):

```bash
# Flash the complete factory firmware image
esptool.py --chip esp32s3 --port <PORT> --baud 921600 write_flash 0x0 CrossNext-Sticky-v1.4.0-factory.bin

# Erase the coredump region
esptool.py --chip esp32s3 --port <PORT> --baud 921600 erase_region 0xff0000 0x10000
```

> **Note**: When connected via USB after a reset, the reTerminal Sticky enters battery-charging standby. Press and hold the physical **Power** button on the device to wake it up into the main interface.

### Flashing via Web Browser (Web Flasher)
You can use [ESP Web Tools (esptool-js)](https://espressif.github.io/esptool-js/) in Chrome or Edge:
1. Connect the reTerminal Sticky via USB.
2. Select your serial port and choose `CrossNext-Sticky-v1.4.0-factory.bin` at offset `0x0`.
3. Click **Program**.

---

## 💬 Community & Support

Join our Discord server to connect with other users, ask questions, propose features, and report bugs:

- **Discord Server:** [https://discord.gg/kChkaFdEyB](https://discord.gg/kChkaFdEyB)
  - `#💡-feature-requests`: Suggest and upvote new feature ideas.
  - `#🐛-bug-reports`: Report bugs, display issues, or crashes with screenshots.
  - `#🔌-plugins-showcase`: Share and discover custom plugins for Plugin Hub.

---

## 🛠️ Building from Source

### Prerequisites
- Python 3.10+
- PlatformIO Core

### Build Steps

```bash
# Clone the repository
git clone https://github.com/loitq07/CrossNext-for-Sticky.git
cd CrossNext-for-Sticky

# Set up virtual environment
python -m venv .venv
source .venv/bin/activate  # On Windows: .venv\Scripts\activate
pip install platformio

# Generate i18n translation tables
python scripts/gen_i18n.py

# Build firmware for reTerminal Sticky
platformio run -e gh_release_sticky
```

The resulting factory image is generated at `.pio/build/gh_release_sticky/firmware.factory.bin`.

---

## 📄 License & Attribution

This project is licensed under the same open-source terms as upstream CrossNext and CrossPoint (GPL / MIT respectively). Please respect original licenses, copyright notices, and bundled asset attributions when redistributing or modifying.
