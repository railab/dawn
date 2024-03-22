#!/bin/bash
#
# clang-tidy.sh - Run clang-tidy static analysis
#
# Usage:
#   ./tools/scripts/clang-tidy.sh check              # Check all files
#   ./tools/scripts/clang-tidy.sh check-file <file>  # Check a single file
#
# Environment:
#   BUILD_DIR       Build directory containing compile_commands.json
#   CLANG_TIDY      clang-tidy binary (default: clang-tidy)
#   CLANG_TIDY_ARGS Extra args passed to clang-tidy
#

set -e

CLANG_TIDY=${CLANG_TIDY:-clang-tidy}
COMMAND=${1:-check}
FILE_ARG=${2:-}

if ! command -v "$CLANG_TIDY" >/dev/null 2>&1; then
  echo "ERROR: clang-tidy not found. Install with: apt-get install clang-tidy"
  exit 1
fi
if ! command -v python3 >/dev/null 2>&1; then
  echo "ERROR: python3 not found. Install python3 to parse compile_commands.json."
  exit 1
fi

find_build_dir() {
  if [ -n "$BUILD_DIR" ]; then
    echo "$BUILD_DIR"
    return
  fi

  if [ -f "build_tests/compile_commands.json" ]; then
    echo "build_tests"
    return
  fi

  if [ -f "build/compile_commands.json" ]; then
    echo "build"
    return
  fi
}

BUILD_DIR=$(find_build_dir)
if [ -z "$BUILD_DIR" ] || [ ! -f "$BUILD_DIR/compile_commands.json" ]; then
  echo "ERROR: compile_commands.json not found."
  echo "Generate it in your build directory (CMake: -DCMAKE_EXPORT_COMPILE_COMMANDS=ON)."
  echo "Then rerun with: BUILD_DIR=path $0 $COMMAND"
  exit 1
fi

# Files in the project we want to analyze, taken from compile_commands.json
# (only translation units appear there). Restrict to dawn sources and the OOT
# app sources that ship in this repo.
compdb_files() {
  python3 - "$BUILD_DIR/compile_commands.json" <<'PY'
import json
import os
import sys

with open(sys.argv[1], "r", encoding="utf-8") as handle:
    data = json.load(handle)

roots = ("dawn/src/", "dawn/tests/", "external/apps/external/src/")
ext = (".c", ".cpp", ".cxx", ".cc")

cwd = os.getcwd() + os.sep
seen = set()
for entry in data:
    path = entry.get("file")
    if not path:
        continue
    norm = os.path.normpath(path)
    rel = norm[len(cwd):] if norm.startswith(cwd) else norm
    if not rel.endswith(ext):
        continue
    if not any(rel.startswith(r) for r in roots):
        continue
    if rel in seen:
        continue
    seen.add(rel)
    print(rel)
PY
}

run_tidy() {
  local file="$1"
  $CLANG_TIDY $CLANG_TIDY_ARGS -p "$BUILD_DIR" "$file"
}

case "$COMMAND" in
  check)
    echo "Running clang-tidy for all files..."
    FILES=$(compdb_files)
    FAILED=0
    ANALYZED=0

    for file in $FILES; do
      if [ ! -f "$file" ]; then
        continue
      fi
      ANALYZED=$((ANALYZED + 1))

      if ! run_tidy "$file"; then
        FAILED=1
      fi
    done

    echo "Analyzed $ANALYZED file(s) from compile database."
    if [ $FAILED -eq 1 ]; then
      echo ""
      echo "clang-tidy found issues."
      exit 1
    fi
    echo "[OK] clang-tidy found no issues."
    exit 0
    ;;

  check-file)
    if [ -z "$FILE_ARG" ]; then
      echo "ERROR: check-file requires a file argument"
      echo "Usage: $0 check-file <file>"
      exit 1
    fi

    if [ ! -f "$FILE_ARG" ]; then
      echo "ERROR: File not found: $FILE_ARG"
      exit 1
    fi

    echo "Running clang-tidy for: $FILE_ARG"
    if ! run_tidy "$FILE_ARG"; then
      echo ""
      echo "clang-tidy found issues in $FILE_ARG"
      exit 1
    fi

    echo "[OK] clang-tidy found no issues in $FILE_ARG"
    exit 0
    ;;

  *)
    echo "Usage: $0 {check|check-file}"
    echo ""
    echo "Commands:"
    echo "  check           - Run clang-tidy on all files in the compile database"
    echo "  check-file FILE - Run clang-tidy on a single file"
    echo ""
    echo "Examples:"
    echo "  BUILD_DIR=build_tests $0 check"
    echo "  $0 check-file dawn/src/io/adc_fetch.cxx"
    exit 1
    ;;
esac
