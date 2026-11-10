#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
out="${TMPDIR:-/tmp}/menu-fonts-tests"
mkdir -p "$out"
"${CXX:-c++}" -std=c++17 -Wall -Wextra -Werror -Wno-missing-field-initializers -Wno-unused-const-variable \
  -Ihost-tests/menu-fonts/stubs -Ilib/EpdFont -Ilib/Utf8 \
  host-tests/menu-fonts/test.cpp lib/EpdFont/EpdFont.cpp lib/EpdFont/EpdFontFamily.cpp lib/Utf8/Utf8.cpp \
  -o "$out/test"
"$out/test"
