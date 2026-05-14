#!/bin/sh
# Licensed under the Apache License, Version 2.0

set -eu

DAWN_HOME="${DAWN_HOME:-/opt/dawn}"
TESTENV_CMD="${TESTENV_CMD:-start}"
DAWN_DOCKER_QA_JOBS="${DAWN_DOCKER_QA_JOBS:-2}"
DAWN_DOCKER_SOURCE="${DAWN_DOCKER_SOURCE:-image}"
DAWN_DOCKER_LOCAL_SRC="${DAWN_DOCKER_LOCAL_SRC:-}"
DAWN_REPO="${DAWN_REPO:-https://github.com/railab/dawn.git}"
DAWN_REF="${DAWN_REF:-master}"
NUTTX_BRANCH="${NUTTX_BRANCH:-dawn}"

has_jobs=0
for arg in "$@"; do
  case "$arg" in
    -j|--jobs|--jobs=*)
      has_jobs=1
      ;;
  esac
done

if [ "$has_jobs" -eq 0 ]; then
  set -- --jobs "$DAWN_DOCKER_QA_JOBS" "$@"
fi

export CMAKE_BUILD_PARALLEL_LEVEL="${CMAKE_BUILD_PARALLEL_LEVEL:-$DAWN_DOCKER_QA_JOBS}"
export NINJA_STATUS="${NINJA_STATUS:-[%f/%t] }"

prepare_local_source() {
  if [ "$DAWN_DOCKER_SOURCE" != "local" ]; then
    return 0
  fi

  if [ -z "$DAWN_DOCKER_LOCAL_SRC" ] || [ ! -d "$DAWN_DOCKER_LOCAL_SRC" ]; then
    echo "dawn-docker-qa: local source directory is missing" >&2
    exit 1
  fi

  mkdir -p "$DAWN_HOME"

  rsync -a --delete \
    --exclude /.git \
    --exclude /.mypy_cache \
    --exclude /.tox \
    --exclude /.venv \
    --exclude /build \
    --exclude /external \
    --exclude /result \
    "$DAWN_DOCKER_LOCAL_SRC"/ "$DAWN_HOME"/
}

prepare_image_source() {
  if [ "$DAWN_DOCKER_SOURCE" != "image" ]; then
    return 0
  fi

  if [ -d "$DAWN_HOME/.git" ]; then
    return 0
  fi

  rm -rf "$DAWN_HOME"
  mkdir -p "$(dirname "$DAWN_HOME")"
  git clone "$DAWN_REPO" "$DAWN_HOME"
  cd "$DAWN_HOME"
  git checkout "$DAWN_REF"
}

normalize_external_symlink() {
  if [ -d external/apps ]; then
    rm -f external/apps/external
    ln -s "$(pwd -P)" external/apps/external
  fi
}

run_repo_init() {
  if [ -d .git ]; then
    BRANCH="$NUTTX_BRANCH" ./repo_init.sh
  else
    BRANCH="$NUTTX_BRANCH" ./repo_init.sh --skip-submodules
  fi
}

prepare_python_environment() {
  if [ ! -d .venv ]; then
    python -m venv .venv
  fi

  . .venv/bin/activate
  python -m pip install --upgrade pip setuptools wheel
  python -m pip install tox
  install-dawn-python-tools --project-root .
}

prepare_local_source
prepare_image_source
cd "$DAWN_HOME"

ensure_tun_device() {
  mkdir -p /dev/net

  if [ ! -e /dev/net/tun ]; then
    mknod /dev/net/tun c 10 200
  fi

  chmod 0666 /dev/net/tun

  if [ ! -c /dev/net/tun ]; then
    echo "dawn-docker-qa: /dev/net/tun is not a character device" >&2
    exit 1
  fi
}

normalize_clang_format() {
  if [ -f .clang-format ]; then
    sed -i 's/^Cpp11BracedListStyle: FunctionCall$/Cpp11BracedListStyle: true/' .clang-format
  fi
}

normalize_warning_flags() {
  if [ -d external ]; then
    grep -rl -- '-Wno-pointer-to-int-cast' external \
      | xargs -r sed -i 's/[[:space:]]*-Wno-pointer-to-int-cast//g'
  fi
}

cleanup() {
  sh ./testenv_init.sh stop >/dev/null 2>&1 || true
}

trap cleanup EXIT INT TERM

normalize_external_symlink
run_repo_init
prepare_python_environment
normalize_clang_format
normalize_warning_flags
ensure_tun_device
sh ./testenv_init.sh "$TESTENV_CMD"
dawnpy-tests "$@"
