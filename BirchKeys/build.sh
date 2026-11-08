#!/usr/bin/env bash
# Builds the Birch Keys standalone app and VST3 (Release) into ./build
set -euo pipefail
cd "$(dirname "$0")"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo
echo "Standalone: build/BirchKeys_artefacts/Release/Standalone/Birch Keys"
echo "VST3:       build/BirchKeys_artefacts/Release/VST3/Birch Keys.vst3"
