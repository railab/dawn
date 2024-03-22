#!/bin/sh
# Licensed under the Apache License, Version 2.0

set -eu

IMAGE="${DAWN_DOCKER_IMAGE:-dawn-container}"
CPUS="${DAWN_DOCKER_CPUS:-4}"
MEMORY="${DAWN_DOCKER_MEMORY:-8g}"
JOBS="${DAWN_DOCKER_QA_JOBS:-2}"
LOG_MAX_SIZE="${DAWN_DOCKER_LOG_MAX_SIZE:-20m}"
LOG_MAX_FILE="${DAWN_DOCKER_LOG_MAX_FILE:-2}"
SOURCE="${DAWN_DOCKER_SOURCE:-image}"
LOCAL_SOURCE="${DAWN_DOCKER_LOCAL_SOURCE:-}"
DAWN_REPO="${DAWN_REPO:-https://github.com/railab/dawn.git}"
DAWN_REF="${DAWN_REF:-master}"
NUTTX_BRANCH="${NUTTX_BRANCH:-dawn}"

err() {
  printf "run-qa.sh: %s\n" "$*" >&2
}

need_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    err "missing command: $1"
    exit 127
  }
}

need_cmd docker

while [ $# -gt 0 ]; do
  case "$1" in
    --source)
      [ $# -ge 2 ] || {
        err "--source requires image or local"
        exit 2
      }
      SOURCE="$2"
      shift 2
      ;;
    --source=*)
      SOURCE="${1#--source=}"
      shift
      ;;
    --local)
      SOURCE="local"
      shift
      ;;
    --image)
      SOURCE="image"
      shift
      ;;
    --)
      shift
      break
      ;;
    *)
      break
      ;;
  esac
done

case "$SOURCE" in
  image|local) ;;
  *)
    err "invalid source: $SOURCE"
    err "expected: image or local"
    exit 2
    ;;
esac

if ! docker info >/dev/null 2>&1; then
  err "Docker daemon is not reachable"
  exit 1
fi

if ! docker system df >/dev/null 2>&1; then
  err "Docker storage accounting failed; refusing to start QA"
  err "Fix Docker's container/storage state before running the full suite"
  exit 1
fi

if [ "$SOURCE" = "local" ]; then
  if [ -z "$LOCAL_SOURCE" ]; then
    if command -v git >/dev/null 2>&1; then
      LOCAL_SOURCE="$(git rev-parse --show-toplevel 2>/dev/null || pwd)"
    else
      LOCAL_SOURCE="$(pwd)"
    fi
  fi

  LOCAL_SOURCE="$(cd "$LOCAL_SOURCE" && pwd -P)"

  exec docker run --rm --init \
    --cpus "$CPUS" \
    --memory "$MEMORY" \
    --memory-swap "$MEMORY" \
    --pids-limit 4096 \
    --log-driver local \
    --log-opt "max-size=$LOG_MAX_SIZE" \
    --log-opt "max-file=$LOG_MAX_FILE" \
    --privileged \
    -e "DAWN_DOCKER_QA_JOBS=$JOBS" \
    -e "DAWN_DOCKER_SOURCE=local" \
    -e "DAWN_DOCKER_LOCAL_SRC=/workspace/dawn-src" \
    -e "DAWN_HOME=/workspace/dawn-local" \
    -e "NUTTX_BRANCH=$NUTTX_BRANCH" \
    -v "$LOCAL_SOURCE:/workspace/dawn-src:ro" \
    "$IMAGE" \
    dawn-docker-qa "$@"
fi

exec docker run --rm --init \
  --cpus "$CPUS" \
  --memory "$MEMORY" \
  --memory-swap "$MEMORY" \
  --pids-limit 4096 \
  --log-driver local \
  --log-opt "max-size=$LOG_MAX_SIZE" \
  --log-opt "max-file=$LOG_MAX_FILE" \
  --privileged \
  -e "DAWN_DOCKER_QA_JOBS=$JOBS" \
  -e "DAWN_REPO=$DAWN_REPO" \
  -e "DAWN_REF=$DAWN_REF" \
  -e "NUTTX_BRANCH=$NUTTX_BRANCH" \
  "$IMAGE" \
  dawn-docker-qa "$@"
