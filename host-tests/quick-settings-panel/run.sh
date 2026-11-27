#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/../.."
out="${TMPDIR:-/tmp}/quick-settings-panel"
mkdir -p "$out"
"${CXX:-c++}" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-missing-field-initializers \
 -Ihost-tests/quick-settings-panel/stubs -Itest/language_pack/stubs -Itest/library_index_file/stubs -Ilib/BlePageTurner/include -Ifreeink-sdk/libs/network/BleKeyboardHost/include \
 -Ifreeink-sdk/libs/ui/FreeInkUI/include -Ifreeink-sdk/libs/assets/Icons/include -Ilib/EpdFont -Ilib/I18n -Ilib/Memory \
 host-tests/quick-settings-panel/test_quick_settings_panel.cpp src/apps_local/ui/PixelMenu.cpp src/apps_local/ui/QuickMenuCore.cpp \
 lib/I18n/I18n.cpp lib/I18n/I18nStrings.cpp lib/I18n/LanguagePack.cpp freeink-sdk/libs/ui/FreeInkUI/src/FreeInkUI.cpp -o "$out/test"
"$out/test" "$@"
