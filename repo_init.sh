#!/bin/sh
# Licensed under the Apache License, Version 2.0
#
# repo_init.sh -- bootstrap the Dawn workspace.
#
# Initialises git submodules under tools/, clones the required NuttX
# repositories into external/, and creates the symlink NuttX needs to
# discover Dawn as an out-of-tree app package.
#
# Run from the root of the dawn repo:
#
#   ./repo_init.sh [options]
#
# Options:
#   -b, --branch BRANCH      branch for NuttX repositories (default: dawn)
#   -f, --force              wipe and recreate external/
#   -v, --verbose            verbose output
#       --nuttx-url URL      override NuttX repository URL
#       --apps-url URL       override NuttX-apps repository URL
#       --skip-submodules    skip `git submodule update`
#   -h, --help               show this help

set -eu

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'

NUTTX_REPO="${NUTTX_REPO:-https://github.com/railab/nuttx}"
APPS_REPO="${APPS_REPO:-https://github.com/railab/nuttx-apps}"
BRANCH="${BRANCH:-dawn}"

FORCE=0
VERBOSE=0
SKIP_SUBMODULES=0

info()    { printf "${BLUE}i${NC} %s\n" "$*"; }
ok()      { printf "${GREEN}v${NC} %s\n" "$*"; }
warn()    { printf "${YELLOW}!${NC} %s\n" "$*" >&2; }
err()     { printf "${RED}x${NC} %s\n" "$*" >&2; }
verbose() { [ "$VERBOSE" -eq 1 ] && printf "  %s\n" "$*" || :; }

usage() {
  sed -n '2,21p' "$0" | sed 's/^# \{0,1\}//'
  exit "${1:-0}"
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    err "Missing command: $1"
    exit 127
  }
}

parse_args() {
  while [ $# -gt 0 ]; do
    case "$1" in
      -b|--branch)        BRANCH="$2"; shift 2;;
      -f|--force)         FORCE=1; shift;;
      -v|--verbose)       VERBOSE=1; shift;;
      --nuttx-url)        NUTTX_REPO="$2"; shift 2;;
      --apps-url)         APPS_REPO="$2"; shift 2;;
      --skip-submodules)  SKIP_SUBMODULES=1; shift;;
      -h|--help)          usage 0;;
      *) err "Unknown option: $1"; usage 2;;
    esac
  done
}

validate_repo_root() {
  # The script must run at the dawn repo root.
  if [ ! -d boards ] || [ ! -d dawn ] || [ ! -d Documentation ]; then
    err "Not in dawn repo root (missing boards/, dawn/, or Documentation/)"
    err "Run this script from the top level of your dawn checkout."
    exit 1
  fi
  if [ ! -f .gitmodules ]; then
    warn ".gitmodules not found; submodule step will be a no-op"
  fi
}

init_submodules() {
  if [ "$SKIP_SUBMODULES" -eq 1 ]; then
    info "Skipping git submodule update (--skip-submodules)"
    return 0
  fi
  info "Initialising git submodules (tools/)..."
  if [ "$VERBOSE" -eq 1 ]; then
    git submodule update --init --recursive
  else
    git submodule update --init --recursive --quiet
  fi
  ok "Submodules ready"
}

setup_external_dir() {
  info "Setting up external/..."
  if [ -d external ]; then
    if [ "$FORCE" -eq 1 ]; then
      warn "Removing existing external/"
      rm -rf external
    else
      err "external/ already exists"
      info "Use --force to wipe and re-clone, or remove it manually"
      exit 1
    fi
  fi
  mkdir -p external
  ok "Created external/"
}

clone_nuttx_repos() {
  info "Cloning NuttX repositories (branch: $BRANCH)..."
  verbose "NuttX: $NUTTX_REPO"
  verbose "Apps:  $APPS_REPO"

  quiet=""
  [ "$VERBOSE" -eq 0 ] && quiet="--quiet"

  # shellcheck disable=SC2086
  git clone $quiet --branch "$BRANCH" "$NUTTX_REPO" external/nuttx \
    || { err "Failed to clone nuttx"; exit 1; }
  ok "Cloned nuttx"

  # shellcheck disable=SC2086
  git clone $quiet --branch "$BRANCH" "$APPS_REPO" external/apps \
    || { err "Failed to clone nuttx-apps"; exit 1; }
  ok "Cloned nuttx-apps"
}

create_symlink() {
  info "Creating external/apps/external -> dawn/ symlink..."
  dawn_src="$(cd dawn && pwd -P)"
  ln -s "$dawn_src" external/apps/external
  ok "Symlink created"
}

validate_setup() {
  info "Validating setup..."
  errors=0

  [ -d external/nuttx/.git ] || { err "external/nuttx/.git missing"; errors=$((errors + 1)); }
  [ -d external/apps/.git ]  || { err "external/apps/.git missing"; errors=$((errors + 1)); }

  if [ ! -L external/apps/external ]; then
    err "external/apps/external symlink missing"
    errors=$((errors + 1))
  elif [ ! -d external/apps/external/src ]; then
    err "Symlink target does not look like dawn/ (no src/)"
    errors=$((errors + 1))
  fi

  if [ $errors -ne 0 ]; then
    err "Setup validation failed ($errors error(s))"
    exit 1
  fi
  ok "Setup valid"
}

print_next_steps() {
  cat <<EOF

${GREEN}========================================${NC}
${GREEN}  Workspace initialisation complete${NC}
${GREEN}========================================${NC}

Recommended next steps (venv-based workflow):

  1. Create and activate a virtual environment:
       python -m venv .venv
       source .venv/bin/activate

  2. Install dawnpy (core CLI) in editable mode:
       pip install -e tools/dawnpy

  3. Install the transport / test tooling you need:
       pip install -e tools/dawnpy-serial \\
                    -e tools/dawnpy-can \\
                    -e tools/dawnpy-udp \\
                    -e tools/dawnpy-modbus \\
                    -e tools/dawnpy-ble \\
                    -e tools/dawnpy-tests
       pip install -r ntfc/requirements.txt

  4. Build the simulator tests:
       python -m dawnpy build build_tests boards/sim/sim/sim/configs/tests
       ./build_tests/nuttx

  5. Or run the full QA flow:
       dawnpy-tests

For more, see Documentation/quickstart.rst and
Documentation/tools/dawnpy.rst.
EOF
}

main() {
  need_cmd git
  command -v cmake >/dev/null 2>&1 || warn "cmake not found (required to build)"
  command -v ninja >/dev/null 2>&1 || warn "ninja not found (recommended)"

  parse_args "$@"
  validate_repo_root
  init_submodules
  setup_external_dir
  clone_nuttx_repos
  create_symlink
  validate_setup
  print_next_steps
}

main "$@"
