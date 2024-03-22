#!/bin/bash
#
# check-format.sh - Check or fix code formatting with clang-format
#
# Usage:
#   ./tools/scripts/check-format.sh check              # Check formatting (dry-run)
#   ./tools/scripts/check-format.sh fix                # Fix formatting (modifies files)
#   ./tools/scripts/check-format.sh check-diff         # Check only changed files
#   ./tools/scripts/check-format.sh check-file <file>  # Check single file
#   ./tools/scripts/check-format.sh fix-file <file>    # Fix single file
#

set -e

CLANG_FORMAT=${CLANG_FORMAT:-clang-format}
COMMAND=${1:-check}
FILE_ARG=${2:-}

# Check if clang-format is available
if ! command -v $CLANG_FORMAT &> /dev/null; then
  echo "ERROR: clang-format not found. Install with: apt-get install clang-format"
  exit 1
fi

# Find all C++ files in dawn source
find_cpp_files() {
    find dawn/include dawn/src dawn/tests dawn/apps \
         -type f \( -name "*.cpp" -o -name "*.cxx" -o -name "*.cc" \
         -o -name "*.h" -o -name "*.hxx" -o -name "*.hpp" \) \
         2>/dev/null || true
}

# Find changed files (git tracked)
find_changed_files() {
  git diff --name-only HEAD | grep -E '\.(cpp|cxx|cc|h|hxx|hpp)$' || true
}

case "$COMMAND" in
  check)
    echo "Checking code formatting for all files..."
    FILES=$(find_cpp_files)
    FAILED=0
    FAILED_FILES=()

    for file in $FILES; do
      if [ ! -f "$file" ]; then
        continue
      fi

      if ! $CLANG_FORMAT --dry-run -Werror "$file" >/dev/null 2>&1; then
        FAILED_FILES+=("$file")
        FAILED=1
      fi
    done

    if [ $FAILED -eq 1 ]; then
      echo ""
      echo "Found formatting issues in ${#FAILED_FILES[@]} file(s):"
      for file in "${FAILED_FILES[@]}"; do
        echo ""
        echo "=========================================="
        echo "File: $file"
        echo "=========================================="
        ORIGINAL=$(cat "$file")
        FORMATTED=$($CLANG_FORMAT "$file")
        echo "Diff (- current, + formatted):"
        diff -u <(echo "$ORIGINAL") <(echo "$FORMATTED") | head -30 || true
        echo ""
      done
      echo "Fix all formatting issues with:"
      echo "  ./tools/scripts/check-format.sh fix"
      exit 1
    else
      echo "[OK] All files formatted correctly!"
      exit 0
    fi
    ;;

  fix)
    echo "Fixing code formatting for all files..."
    FILES=$(find_cpp_files)
    FIXED=0
    VERBOSE=${VERBOSE:-0}

    for file in $FILES; do
      if [ ! -f "$file" ]; then
        continue
      fi

      # Check if file needs formatting
      if ! $CLANG_FORMAT --dry-run -Werror "$file" >/dev/null 2>&1; then
        if [ $VERBOSE -eq 1 ]; then
          echo ""
          echo "Fixing: $file"
          ORIGINAL=$(cat "$file")
          $CLANG_FORMAT -i "$file"
          FORMATTED=$(cat "$file")
          echo "Changes (- before, + after):"
          diff -u <(echo "$ORIGINAL") <(echo "$FORMATTED") | head -20 || true
        else
          echo "  Fixing: $file"
          $CLANG_FORMAT -i "$file"
        fi
        FIXED=$((FIXED + 1))
      fi
    done

    echo ""
    echo "[OK] Fixed $FIXED files"
    echo "To see changes, run: VERBOSE=1 ./tools/scripts/check-format.sh fix"
    exit 0
    ;;

  check-diff)
    echo "Checking formatting for changed files..."
    FILES=$(find_changed_files)

    if [ -z "$FILES" ]; then
      echo "[OK] No changed files to check"
      exit 0
    fi

    FAILED=0
    FAILED_FILES=()

    for file in $FILES; do
      if [ ! -f "$file" ]; then
        continue
      fi

      if ! $CLANG_FORMAT --dry-run -Werror "$file" >/dev/null 2>&1; then
        FAILED_FILES+=("$file")
        FAILED=1
      fi
    done

    if [ $FAILED -eq 1 ]; then
      echo ""
      echo "Found formatting issues in ${#FAILED_FILES[@]} changed file(s):"
      for file in "${FAILED_FILES[@]}"; do
        echo ""
        echo "=========================================="
        echo "File: $file"
        echo "=========================================="
        ORIGINAL=$(cat "$file")
        FORMATTED=$($CLANG_FORMAT "$file")
        echo "Diff (- current, + formatted):"
        diff -u <(echo "$ORIGINAL") <(echo "$FORMATTED") | head -30 || true
        echo ""
      done
      echo "Fix all formatting issues with:"
      echo "  ./tools/scripts/check-format.sh fix-diff"
      exit 1
    else
      echo "[OK] All changed files formatted correctly!"
      exit 0
    fi
    ;;

  fix-diff)
    echo "Fixing formatting for changed files..."
    FILES=$(find_changed_files)

    if [ -z "$FILES" ]; then
      echo "[OK] No changed files to fix"
      exit 0
    fi

    FIXED=0
    VERBOSE=${VERBOSE:-0}

    for file in $FILES; do
      if [ ! -f "$file" ]; then
        continue
      fi

      if ! $CLANG_FORMAT --dry-run -Werror "$file" >/dev/null 2>&1; then
        if [ $VERBOSE -eq 1 ]; then
          echo ""
          echo "Fixing: $file"
          ORIGINAL=$(cat "$file")
          $CLANG_FORMAT -i "$file"
          FORMATTED=$(cat "$file")
          echo "Changes (- before, + after):"
          diff -u <(echo "$ORIGINAL") <(echo "$FORMATTED") | head -20 || true
        else
          echo "  Fixing: $file"
          $CLANG_FORMAT -i "$file"
        fi
        FIXED=$((FIXED + 1))
      fi
    done

    echo ""
    echo "[OK] Fixed $FIXED files"
    echo "To see changes, run: VERBOSE=1 ./tools/scripts/check-format.sh fix-diff"
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

    echo "Checking formatting for: $FILE_ARG"

    if ! $CLANG_FORMAT --dry-run -Werror "$FILE_ARG" >/dev/null 2>&1; then
      echo ""
      echo "=========================================="
      echo "File: $FILE_ARG"
      echo "=========================================="
      ORIGINAL=$(cat "$FILE_ARG")
      FORMATTED=$($CLANG_FORMAT "$FILE_ARG")
      echo "Diff (- current, + formatted):"
      diff -u <(echo "$ORIGINAL") <(echo "$FORMATTED") | head -50 || true
      echo ""
      echo "Fix with:"
      echo "  ./tools/scripts/check-format.sh fix-file $FILE_ARG"
      exit 1
    else
      echo "[OK] File formatted correctly!"
      exit 0
    fi
    ;;

  fix-file)
    if [ -z "$FILE_ARG" ]; then
      echo "ERROR: fix-file requires a file argument"
      echo "Usage: $0 fix-file <file>"
      exit 1
    fi

    if [ ! -f "$FILE_ARG" ]; then
      echo "ERROR: File not found: $FILE_ARG"
      exit 1
    fi

    echo "Checking if formatting is needed for: $FILE_ARG"

    if ! $CLANG_FORMAT --dry-run -Werror "$FILE_ARG" >/dev/null 2>&1; then
      echo ""
      ORIGINAL=$(cat "$FILE_ARG")
      echo "Fixing: $FILE_ARG"

      $CLANG_FORMAT -i "$FILE_ARG"
      FORMATTED=$(cat "$FILE_ARG")

      echo "Changes made (- before, + after):"
      echo "=========================================="
      diff -u <(echo "$ORIGINAL") <(echo "$FORMATTED") | head -50 || true
      echo "=========================================="
      echo ""
      echo "[OK] File formatted successfully!"
      echo "Review changes and stage if needed:"
      echo "  git add $FILE_ARG"
      exit 0
    else
      echo "[OK] File already formatted correctly!"
      exit 0
    fi
    ;;

  *)
    echo "Usage: $0 {check|fix|check-diff|fix-diff|check-file|fix-file}"
    echo ""
    echo "Commands:"
    echo "  check           - Check formatting for all files (dry-run)"
    echo "  fix             - Fix formatting for all files (modifies files)"
    echo "  check-diff      - Check formatting for changed files"
    echo "  fix-diff        - Fix formatting for changed files"
    echo "  check-file FILE - Check formatting for a single file"
    echo "  fix-file FILE   - Fix formatting for a single file"
    echo ""
    echo "Examples:"
    echo "  $0 check-file dawn/src/io/adc_fetch.cxx"
    echo "  $0 fix-file dawn/include/dawn/prog/process.hxx"
    exit 1
    ;;
esac
