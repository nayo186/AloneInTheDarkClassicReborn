#!/bin/bash
# Builds the players' version for macOS and packs it for GitHub Releases
# (the macOS side of makeRelease.ps1 and makeRelease.sh).
#
#   tools/makeRelease-macos.sh 1.0
#
# - Its own build folder (TatouSource/build/release-macos, or $BUILD_DIR).
# - One program for both kinds of Mac: Apple Silicon (arm64) and Intel
#   (x86_64), macOS 11 or newer.
# - CLASSIC_REBORN_DEV_TOOLS=OFF: no item window (F9) nor object editor (F10).
# - CLASSIC_REBORN_VERSION=<version>: the launcher shows it and compares it
#   with the latest release to offer updates.
# - The archive holds only Launcher, Tatou, README.md, LICENSE and
#   Gelasio-OFL.txt. It is a .tar.gz, so both programs keep their
#   "executable" mark.
# The result goes to release/ClassicReborn-<version>-macos.tar.gz.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>" >&2
    exit 1
fi
version="$1"
repo="$(cd "$(dirname "$0")/.." && pwd)"
source="$repo/TatouSource"
build="${BUILD_DIR:-$source/build/release-macos}"
output="$build/Fitd/Release"
releaseFolder="$repo/release"
name="ClassicReborn-$version-macos"

echo "== Configuring ($build)"
cmake -G Ninja -S "$source" -B "$build" -DCMAKE_BUILD_TYPE=Release -DCLASSIC_REBORN_DEV_TOOLS=OFF -DCLASSIC_REBORN_VERSION="$version" \
    -DCMAKE_OSX_ARCHITECTURES="arm64;x86_64" -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0 > /dev/null

echo "== Building Release"
cmake --build "$build" --target Launcher Fitd

# The development tools must not be in the executable
echo "== Checking that F9 and F10 are not included"
for marker in "(prueba, F9)" "(prueba, F10)"; do
    if grep -qF "$marker" "$output/Tatou"; then
        echo "Tatou still contains the development tools ($marker)" >&2
        exit 1
    fi
done

echo "== Packing"
temporary="$(mktemp -d)"
staging="$temporary/$name"
mkdir -p "$staging" "$releaseFolder"
cp "$output/Launcher" "$output/Tatou" "$staging/"
cp "$repo/README.md" "$repo/LICENSE" "$staging/"
cp "$source/Shared/assets/fonts/OFL.txt" "$staging/Gelasio-OFL.txt"
chmod 755 "$staging/Launcher" "$staging/Tatou"
cp "$source/Shared/assets/fonts/Inter-OFL.txt" "$staging/"
chmod 644 "$staging/README.md" "$staging/LICENSE" "$staging/Gelasio-OFL.txt" "$staging/Inter-OFL.txt"

archive="$releaseFolder/$name.tar.gz"
rm -f "$archive"
tar -czf "$archive" -C "$temporary" "$name"
rm -rf "$temporary"

echo "== Done: $archive ($(du -h "$archive" | cut -f1))"
