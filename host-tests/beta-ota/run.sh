#!/bin/sh
# Beta updates: ReleaseJsonParser list mode picks the newest release (prereleases
# included) that carries the Sticky OTA asset.
#   host-tests/beta-ota/run.sh            (CXX="zig c++" works on Windows)
set -e
cd "$(dirname "$0")"
BUILD_DIR="${TMPDIR:-/tmp}/beta-ota-tests-$(cd ../.. && pwd | cksum | cut -d" " -f1)"
mkdir -p "$BUILD_DIR"
${CXX:-c++} -std=c++17 -Wall -Wextra -Werror -DFREEINK_DEVICE_STICKY=1 \
  -I../../lib/JsonParser -I../../freeink-sdk/libs/network/JsonSax/include \
  ../../freeink-sdk/libs/network/JsonSax/src/StreamingJsonParser.cpp ../../lib/JsonParser/ReleaseJsonParser.cpp \
  test.cpp -o "$BUILD_DIR/test_beta_ota"
"$BUILD_DIR/test_beta_ota"
