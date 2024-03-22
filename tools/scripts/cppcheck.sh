#!/bin/sh
# Licensed under the Apache License, Version 2.0
#
# Run cppcheck with the Dawn project configuration.

set -eu

SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
REPO_ROOT=$(CDPATH= cd -- "$SCRIPT_DIR/../.." && pwd -P)
PROJECT_FILE="$SCRIPT_DIR/dawn.cppcheck"
SUPPRESSIONS_FILE="$SCRIPT_DIR/cppcheck.suppressions"
BUILD_DIR="${CPPCHECK_BUILD_DIR:-$REPO_ROOT/cppcheck-build-dir}"
ENABLE="${CPPCHECK_ENABLE:-warning,performance,portability}"
OUTPUT_FILE=""
TEMP_OUTPUT=0

err() {
  printf "cppcheck.sh: %s\n" "$*" >&2
}

ok() {
  printf "\033[0;32mOK\033[0m %s\n" "$*"
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    err "missing command: $1"
    err "install cppcheck and run this script again"
    exit 127
  }
}

warn_if_missing_external() {
  if [ ! -d "$REPO_ROOT/external/nuttx/include" ] \
    || [ ! -d "$REPO_ROOT/external/apps/include" ]; then
    err "warning: NuttX include directories are missing"
    err "warning: run ./repo_init.sh from the repository root for full analysis"
  fi
}

need_cmd cppcheck

for arg in "$@"; do
  case "$arg" in
    --version|-h|--help|--errorlist)
      exec cppcheck "$@"
      ;;
  esac
done

for arg in "$@"; do
  case "$arg" in
    --output-file=*)
      OUTPUT_FILE="${arg#--output-file=}"
      ;;
  esac
done

prev=""
for arg in "$@"; do
  if [ "$prev" = "--output-file" ]; then
    OUTPUT_FILE="$arg"
    break
  fi
  prev="$arg"
done

if [ ! -f "$PROJECT_FILE" ]; then
  err "missing project file: $PROJECT_FILE"
  exit 1
fi

warn_if_missing_external
mkdir -p "$BUILD_DIR"

cd "$REPO_ROOT"

if [ -z "$OUTPUT_FILE" ]; then
  OUTPUT_FILE=$(mktemp "${TMPDIR:-/tmp}/dawn-cppcheck.XXXXXX")
  TEMP_OUTPUT=1
fi

set +e
if [ "$TEMP_OUTPUT" -eq 1 ]; then
  cppcheck \
    --quiet \
    --enable="$ENABLE" \
    --check-level=exhaustive \
    --project="${PROJECT_FILE#$REPO_ROOT/}" \
    --suppressions-list="${SUPPRESSIONS_FILE#$REPO_ROOT/}" \
    --cppcheck-build-dir="${BUILD_DIR#$REPO_ROOT/}" \
    --relative-paths=. \
    --template="{file}:{line}: {severity}: [{id}] {message}" \
    --template-location="" \
    --output-file="$OUTPUT_FILE" \
    "$@"
else
  cppcheck \
    --quiet \
    --enable="$ENABLE" \
    --check-level=exhaustive \
    --project="${PROJECT_FILE#$REPO_ROOT/}" \
    --suppressions-list="${SUPPRESSIONS_FILE#$REPO_ROOT/}" \
    --cppcheck-build-dir="${BUILD_DIR#$REPO_ROOT/}" \
    --relative-paths=. \
    --template="{file}:{line}: {severity}: [{id}] {message}" \
    --template-location="" \
    "$@"
fi
ret=$?
set -e

if [ -s "$OUTPUT_FILE" ]; then
  cat "$OUTPUT_FILE" >&2
else
  ok "cppcheck passed"
fi

if [ "$TEMP_OUTPUT" -eq 1 ]; then
  rm -f "$OUTPUT_FILE"
fi

exit "$ret"
