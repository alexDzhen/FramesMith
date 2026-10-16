#!/usr/bin/env sh
# Builds framesmith in release mode and runs the tests. Needs CMake and a C compiler.
set -eu

cd "$(dirname "$0")/.."

cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure

echo
echo "Done. Build a frame with:  ./build/framesmith --dst broadcast --type arp --payload \"00 01\""
echo "Sending frames (./build/framesmith --send IFACE) needs root or CAP_NET_RAW; see the README."
