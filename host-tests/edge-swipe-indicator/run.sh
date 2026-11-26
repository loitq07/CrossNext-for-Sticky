#!/bin/sh
set -eu
cd "$(dirname "$0")/../.."
out="${TMPDIR:-/tmp}/edge-swipe-indicator-$(pwd | cksum | cut -d' ' -f1)"
mkdir -p "$out"
"${CXX:-c++}" -std=c++20 -O2 -Wall -Wextra -Werror -Wno-comment \
  -Ihost-tests/edge-swipe-indicator/stubs \
  -Isrc/components \
  host-tests/edge-swipe-indicator/test_edge_swipe_indicator.cpp \
  src/components/EdgeSwipeIndicator.cpp \
  -o "$out/tests"
"$out/tests"
