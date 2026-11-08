#!/bin/bash
# Builds the players' version for Android and leaves it ready for GitHub
# Releases (the Android side of makeRelease.ps1, makeRelease.sh and
# makeRelease-macos.sh).
#
#   tools/makeRelease-android.sh 1.6.0
#
# - Gradle's release build of TatouSource/android: the engine and the
#   launcher as libmain.so for arm64 phones (Android 7 or newer).
# - CLASSIC_REBORN_DEV_TOOLS=OFF (no item window nor object editor) and
#   CLASSIC_REBORN_VERSION=<version>, the app's version too (Android's number
#   for it grows with each release: 1.6.0 -> 10600).
# - Signed with the project's key: Android only updates the app with one
#   signed with the same key. The key comes from these variables (the
#   release workflow fills them from GitHub's secrets):
#     CLASSIC_REBORN_KEYSTORE           the .jks file
#     CLASSIC_REBORN_KEYSTORE_PASSWORD  its password
#     CLASSIC_REBORN_KEY_ALIAS          the key's alias
#     CLASSIC_REBORN_KEY_PASSWORD       the key's password
# - Needs JDK 17 and the Android SDK (ANDROID_HOME) with the NDK and CMake
#   of TatouSource/android/app/build.gradle.
# The result goes to release/ClassicReborn-<version>-android.apk.

set -euo pipefail

if [ $# -ne 1 ]; then
    echo "Usage: $0 <version>" >&2
    exit 1
fi
version="$1"
repo="$(cd "$(dirname "$0")/.." && pwd)"
project="$repo/TatouSource/android"
releaseFolder="$repo/release"
name="ClassicReborn-$version-android.apk"

if [ -z "${CLASSIC_REBORN_KEYSTORE:-}" ] || [ ! -f "$CLASSIC_REBORN_KEYSTORE" ]; then
    echo "The project's signing key is missing (CLASSIC_REBORN_KEYSTORE)" >&2
    exit 1
fi

echo "== Building the release app"
(cd "$project" && bash ./gradlew assembleRelease --no-daemon -PclassicRebornVersion="$version")

apk="$project/app/build/outputs/apk/release/app-release.apk"
if [ ! -f "$apk" ]; then
    echo "No signed app was made ($apk)" >&2
    exit 1
fi

mkdir -p "$releaseFolder"
cp "$apk" "$releaseFolder/$name"
echo "== Done: release/$name ($(du -h "$releaseFolder/$name" | cut -f1))"
