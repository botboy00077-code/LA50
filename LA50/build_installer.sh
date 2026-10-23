#!/bin/bash
# Builds LA-50 and wraps it in an installer: LA-50-Installer.pkg
# Run from the LA50 folder:   bash build_installer.sh
set -euo pipefail
cd "$(dirname "$0")"

command -v cmake >/dev/null || { echo "CMake is missing. Install it from https://cmake.org/download/ (or: brew install cmake) and run this again."; exit 1; }
xcode-select -p >/dev/null 2>&1 || { echo "Run: xcode-select --install   (installs Apple's command line tools, no Xcode app needed), then run this again."; exit 1; }

echo "==> Building (first run downloads JUCE and takes a few minutes)"
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release -j 8

AU=$(find build -name "LA-50.component" -type d | head -1)
VST=$(find build -name "LA-50.vst3" -type d | head -1)
[ -n "$AU" ] || { echo "Build did not produce the Audio Unit."; exit 1; }

echo "==> Staging"
rm -rf stage && mkdir -p stage/Library/Audio/Plug-Ins/Components stage/Library/Audio/Plug-Ins/VST3
cp -R "$AU" stage/Library/Audio/Plug-Ins/Components/
[ -n "$VST" ] && cp -R "$VST" stage/Library/Audio/Plug-Ins/VST3/
codesign --force --deep -s - stage/Library/Audio/Plug-Ins/Components/LA-50.component
[ -n "$VST" ] && codesign --force --deep -s - stage/Library/Audio/Plug-Ins/VST3/LA-50.vst3

echo "==> Creating installer"
pkgbuild --root stage --identifier com.la50audio.la50.pkg --version 1.0.0 \
         --install-location / LA-50-Installer.pkg

echo
echo "Done: $(pwd)/LA-50-Installer.pkg"
echo "Double-click it to install. If macOS blocks it: right-click > Open, or System Settings > Privacy & Security > Open Anyway."
