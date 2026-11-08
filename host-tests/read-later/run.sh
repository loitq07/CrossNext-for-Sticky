#!/bin/sh
set -eu
cd "$(dirname "$0")"
OUT="${TMPDIR:-/tmp}/read-later-tests"
mkdir -p "$OUT"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -I../../lib/FsHelpers -I../../src \
  test_core.cpp ../../src/apps_local/home/HomeMenuCore.cpp -o "$OUT/core"
"$OUT/core"
JSON="${ARDUINOJSON_ROOT:-../../.pio/libdeps/gh_release_sticky/ArduinoJson/src}"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -Istubs -I../reading-suite/stubs \
  -I../../lib/FsHelpers -I../../src -I"$JSON" test_recents.cpp ../../src/RecentBooksStore.cpp -o "$OUT/recents"
"$OUT/recents"
