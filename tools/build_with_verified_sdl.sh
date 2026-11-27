#!/usr/bin/env bash
# Reproducible offline SDL3 bootstrap for Ember Clash.
# Accepts only the exact upstream SDL3 release-3.2.10 codeload archive
# whose SHA-256 was measured during the 2026-11-26 recovery.
set -euo pipefail

if [[ $# -lt 1 || $# -gt 2 ]]; then
  echo "Usage: $0 /path/to/SDL-release-3.2.10.tar.gz [build-dir]" >&2
  exit 2
fi

ARCHIVE=$(realpath "$1")
BUILD_DIR=${2:-build-sdl3-offline}
REPO_ROOT=$(cd "$(dirname "$0")/.." && pwd)
EXPECTED_SHA256=8a6ddc3e70de897f7921da275ac71e375de89dad802aa2932649d1fea9068478
ACTUAL_SHA256=$(sha256sum "$ARCHIVE" | awk '{print $1}')
if [[ "$ACTUAL_SHA256" != "$EXPECTED_SHA256" ]]; then
  echo "ERROR: SDL3 archive hash mismatch" >&2
  echo "Expected: $EXPECTED_SHA256" >&2
  echo "Actual:   $ACTUAL_SHA256" >&2
  exit 3
fi

WORK=$(mktemp -d)
trap 'rm -rf "$WORK"' EXIT
# Reject unexpected archive layouts before extraction.
if ! tar -tzf "$ARCHIVE" | awk 'NR==1 { if ($0 !~ /^SDL-release-3\.2\.10\//) exit 1 } END { if (NR==0) exit 1 }'; then
  echo "ERROR: unexpected SDL archive layout" >&2
  exit 4
fi
tar -xzf "$ARCHIVE" -C "$WORK"
SDL_SOURCE="$WORK/SDL-release-3.2.10"
test -f "$SDL_SOURCE/CMakeLists.txt"

cmake -S "$REPO_ROOT" -B "$BUILD_DIR" -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DEMBER_SDL3_SOURCE_DIR="$SDL_SOURCE"
cmake --build "$BUILD_DIR" --parallel
ctest --test-dir "$BUILD_DIR" --output-on-failure
