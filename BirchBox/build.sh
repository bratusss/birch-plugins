#!/usr/bin/env bash
# Builds the Birch Box standalone app and VST3 (Release) into ./build
set -euo pipefail
cd "$(dirname "$0")"

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release

echo
echo "Standalone: build/BirchBox_artefacts/Release/Standalone/Birch Box"
echo "VST3:       build/BirchBox_artefacts/Release/VST3/Birch Box.vst3"
