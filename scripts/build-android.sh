#!/usr/bin/env bash
#
# build-android.sh - Cross-compile MyMemoryPool for Android (standalone).
#
# Not coupled to GitHub Actions. Run on your own machine:
#   export ANDROID_NDK=/path/to/android-ndk
#   ./scripts/build-android.sh
#
# Options:
#   --ndk <dir>        Android NDK root (overrides $ANDROID_NDK)
#   --abi <list>       Comma-separated ABIs (default: armeabi-v7a,arm64-v8a,x86,x86_64)
#   --api <level>      Minimum Android API level (default: 24)
#   --build-type <t>   Release | Debug (default: Release)
#   --out <dir>        Output directory (default: <repo>/build-android)
#   -h, --help         Show this help
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

NDK="${ANDROID_NDK:-}"
ABIS="armeabi-v7a,arm64-v8a,x86,x86_64"
API=24
BUILD_TYPE=Release
OUT="$ROOT_DIR/build-android"

while [[ $# -gt 0 ]]; do
  case "$1" in
    --ndk)        NDK="$2"; shift 2;;
    --abi)        ABIS="$2"; shift 2;;
    --api)        API="$2"; shift 2;;
    --build-type) BUILD_TYPE="$2"; shift 2;;
    --out)        OUT="$2"; shift 2;;
    -h|--help)    sed -n '2,40p' "$0"; exit 0;;
    *) echo "Unknown argument: $1" >&2; exit 1;;
  esac
done

if [[ -z "$NDK" ]]; then
  echo "ERROR: ANDROID_NDK is not set. Export it or pass --ndk <dir>." >&2
  exit 1
fi
if [[ ! -f "$NDK/build/cmake/android.toolchain.cmake" ]]; then
  echo "ERROR: android.toolchain.cmake not found under: $NDK" >&2
  exit 1
fi
if ! command -v cmake >/dev/null 2>&1; then
  echo "ERROR: cmake not found in PATH." >&2
  exit 1
fi

TOOLCHAIN="$NDK/build/cmake/android.toolchain.cmake"
IFS=',' read -ra ABI_LIST <<< "$ABIS"

echo "==> Android build: API $API, $BUILD_TYPE"
for abi in "${ABI_LIST[@]}"; do
  build_dir="$OUT/$abi"
  echo "==> [$abi] configuring..."
  cmake -S "$ROOT_DIR" -B "$build_dir" -DCMAKE_TOOLCHAIN_FILE="$TOOLCHAIN" -DANDROID_ABI="$abi" -DANDROID_PLATFORM="android-$API" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DCMAKE_POSITION_INDEPENDENT_CODE=ON -DBUILD_TESTING=OFF
  echo "==> [$abi] building..."
  cmake --build "$build_dir" --config "$BUILD_TYPE" -j
  lib="$build_dir/libmempool.a"
  if [[ ! -f "$lib" ]]; then
    echo "ERROR: expected output not found: $lib" >&2
    exit 1
  fi
  echo "    -> $lib"
done

echo "Done. Android libraries are in: $OUT"
