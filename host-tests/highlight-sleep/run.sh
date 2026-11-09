#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
out="${TMPDIR:-/tmp}/highlight-sleep-tests"
mkdir -p "$out"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror host-tests/highlight-sleep/test_index.cpp -o "$out/index"
"$out/index"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror \
  -Ihost-tests/highlight-sleep/stubs -Ihost-tests/reading-tools/stubs -Isrc \
  host-tests/highlight-sleep/test_render.cpp src/apps_local/quotes/HighlightSleep.cpp \
  src/apps_local/quotes/QuotesStore.cpp -o "$out/render"
"$out/render"
