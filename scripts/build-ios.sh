#!/usr/bin/env bash
#
# build-ios.sh - Build MyMemoryPool for iOS and package an xcframework (standalone).
#
# Not coupled to GitHub Actions. Run on a Mac:
#   ./scripts/build-ios.sh
#
# Options:
#   --build-type <t>     Release | Debug (default: Release)
#   --min-ios <ver>      Deployment target (default: 15.0)
#   --out <dir>          Output directory (default: <repo>/build-ios)
#   --no-xcframework     Skip xcframework packaging (keep per-sysroot .a only)
#   -h, --help           Show this help
#
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

BUILD_TYPE=Release
MIN_IOS=15.0
OUT="$ROOT_DIR/build-ios"
MAKE_XCFW=1

while [[ $# -gt 0 ]]; do
  case "$1" in
    --build-type) BUILD_TYPE="$2"; shift 2;;
    --min-ios)    MIN_IOS="$2"; shift 2;;
    --out)        OUT="$2"; shift 2;;
    --no-xcframework) MAKE_XCFW=0; shift;;
    -h|--help)    sed -n '2,40p' "$0"; exit 0;;
    *) echo "Unknown argument: $1" >&2; exit 1;;
  esac
done

if [[ "$(uname -s)" != "Darwin" ]]; then
  echo "ERROR: iOS builds require macOS (this is $(uname -s))." >&2
  exit 1
fi
for tool in cmake xcodebuild xcrun; do
  if ! command -v "$tool" >/dev/null 2>&1; then
    echo "ERROR: '$tool' not found in PATH." >&2
    exit 1
  fi
done

build_one() {
  local sysroot="$1" arch="$2" dest="$3"
  echo "==> [$sysroot $arch] configuring..."
  cmake -S "$ROOT_DIR" -B "$dest" -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT="$sysroot" -DCMAKE_OSX_ARCHITECTURES="$arch" -DCMAKE_OSX_DEPLOYMENT_TARGET="$MIN_IOS" -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -DBUILD_TESTING=OFF
  echo "==> [$sysroot $arch] building..."
  cmake --build "$dest" --config "$BUILD_TYPE" -j
}

build_one iphoneos "arm64" "$OUT/iphoneos"
build_one iphonesimulator "arm64;x86_64" "$OUT/iphonesimulator"

DEVICE_LIB="$OUT/iphoneos/libmempool.a"
SIM_LIB="$OUT/iphonesimulator/libmempool.a"
for f in "$DEVICE_LIB" "$SIM_LIB"; do
  if [[ ! -f "$f" ]]; then
    echo "ERROR: expected output not found: $f" >&2
    exit 1
  fi
done

if [[ "$MAKE_XCFW" -eq 1 ]]; then
  HEADERS_DIR="$OUT/headers"
  mkdir -p "$HEADERS_DIR"
  cp "$ROOT_DIR/mempool.h" "$HEADERS_DIR/"
  echo "==> creating xcframework..."
  xcodebuild -create-xcframework -library "$DEVICE_LIB" -headers "$HEADERS_DIR" -library "$SIM_LIB" -headers "$HEADERS_DIR" -output "$OUT/mempool.xcframework"
  echo "    -> $OUT/mempool.xcframework"
fi

echo "Done. Libraries: $DEVICE_LIB, $SIM_LIB"
