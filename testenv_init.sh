#!/bin/sh
# Licensed under the Apache License, Version 2.0

#!/bin/sh
set -eu

CANDEV="${CANDEV:-can0}"
SIM="${SIM:-/tmp/ttySIM0}"
NX="${NX:-/tmp/ttyNX0}"
PIDFILE="${PIDFILE:-/tmp/socat_pty_bridge.pid}"
LOGFILE="${LOGFILE:-/tmp/socat_pty_bridge.log}"
BRIDGE="${BRIDGE:-br0}"
TAP="${TAP:-tap0}"
BRIDGE_IP="${BRIDGE_IP:-192.168.8.1/24}"

UID_NUM="$(id -u)"
GID_NUM="$(id -g)"

need_cmd() { command -v "$1" >/dev/null 2>&1 || { echo "Missing command: $1" >&2; exit 127; }; }

check_sudo() {
  if ! command -v sudo >/dev/null 2>&1; then
    echo "ERROR: sudo is required for CAN interface setup." >&2
    exit 1
  fi

  echo "This script needs sudo privileges to configure vcan interface ($CANDEV)."

  # Check if we already have cached sudo credentials
  if sudo -n true 2>/dev/null; then
    echo "Sudo credentials already available."
  else
    echo "You may be prompted for your password."
    if ! sudo -v; then
      echo "ERROR: Unable to obtain sudo privileges." >&2
      exit 1
    fi
  fi
}

can_setup() {
  sudo sh -c "
    set -eu
    if ! ip link show '$CANDEV' >/dev/null 2>&1; then
      ip link add dev '$CANDEV' type vcan
    fi
    ip link set '$CANDEV' up
  "
}

qemu_net_setup() {
  sudo sh -c "
    set -eu
    # 1. Create the bridge
    if ! ip link show '$BRIDGE' >/dev/null 2>&1; then
      ip link add name '$BRIDGE' type bridge
    fi
    ip link set '$BRIDGE' up

    # 2. Create the tap interface
    if ! ip link show '$TAP' >/dev/null 2>&1; then
      ip tuntap add dev '$TAP' mode tap
      ip link set '$TAP' master '$BRIDGE'
    fi
    ip link set '$TAP' up

    # 3. Assign IP to the *bridge*, not to tap0
    if ! ip addr show dev '$BRIDGE' | grep -q '${BRIDGE_IP%/*}'; then
      ip addr add '$BRIDGE_IP' dev '$BRIDGE'
    fi
    sudo arp -s 192.168.8.104 52:54:00:12:34:56
  "
}

qemu_net_cleanup() {
  sudo sh -c "
    ip link set '$TAP' down 2>/dev/null || true
    ip tuntap del dev '$TAP' mode tap 2>/dev/null || true
    ip link set '$BRIDGE' down 2>/dev/null || true
    ip link del name '$BRIDGE' type bridge 2>/dev/null || true
  "
}

start_socat() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    echo "socat already running (pid $(cat "$PIDFILE"))"
    return 0
  fi

  rm -f "$SIM" "$NX" "$PIDFILE"

  setsid sh -c "
    exec socat \
      PTY,link='$SIM',raw,echo=0,mode=0660,uid=$UID_NUM,gid=$GID_NUM \
      PTY,link='$NX',raw,echo=0,mode=0660,uid=$UID_NUM,gid=$GID_NUM \
      >>'$LOGFILE' 2>&1
  " </dev/null &

  echo "$!" >"$PIDFILE"

  # Wait for PTYs
  i=0
  while [ $i -lt 50 ]; do
    [ -e "$SIM" ] && [ -e "$NX" ] && break
    i=$((i + 1))
    sleep 0.1
  done

  if [ ! -e "$SIM" ] || [ ! -e "$NX" ]; then
    echo "ERROR: PTYs not created. Check $LOGFILE" >&2
    exit 1
  fi

  stty -F "$SIM" raw
  stty -F "$NX" raw
}

print_summary() {
  echo "Started successfully:"
  echo "  CAN interface : $CANDEV"
  echo "  Network       : $BRIDGE ($BRIDGE_IP) <-> $TAP"
  echo "  PTYs          : $SIM <-> $NX"
  if [ -f "$PIDFILE" ]; then
    echo "  socat PID     : $(cat "$PIDFILE")"
  fi
}

stop_socat() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    kill "$(cat "$PIDFILE")"
    echo "Stopped socat (pid $(cat "$PIDFILE"))"
  else
    echo "socat not running"
  fi
  rm -f "$PIDFILE"
}

status() {
  if [ -f "$PIDFILE" ] && kill -0 "$(cat "$PIDFILE")" 2>/dev/null; then
    echo "socat running (pid $(cat "$PIDFILE"))"
  else
    echo "socat not running"
  fi
  ip link show "$CANDEV" 2>/dev/null || echo "$CANDEV not present/up"
  ip link show "$BRIDGE" 2>/dev/null || echo "$BRIDGE not present/up"
  ip link show "$TAP" 2>/dev/null || echo "$TAP not present/up"
  [ -e "$SIM" ] && ls -l "$SIM" || echo "missing: $SIM"
  [ -e "$NX" ] && ls -l "$NX" || echo "missing: $NX"
}

main() {
  need_cmd ip
  need_cmd socat
  need_cmd stty

  cmd="${1:-start}"
  case "$cmd" in
    start)
      check_sudo
      can_setup
      qemu_net_setup
      start_socat
      print_summary
      ;;
    stop)
      stop_socat
      qemu_net_cleanup
      ;;
    restart)
      check_sudo
      stop_socat
      qemu_net_cleanup
      can_setup
      qemu_net_setup
      start_socat
      print_summary
      ;;
    status)
      status
      ;;
    *)
      echo "Usage: $0 {start|stop|restart|status}" >&2
      exit 2
      ;;
  esac
}

main "$@"
