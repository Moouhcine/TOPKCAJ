#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

export LD_LIBRARY_PATH="/usr/local/lib:${LD_LIBRARY_PATH:-}"

if [ ! -x backend/casino_server ] || [ ! -x backend/player ]; then
  echo "[run_demo] Building backend..."
  make -C backend
fi

if [ ! -x viewer/viewer ]; then
  echo "[run_demo] Building viewer..."
  make -C viewer
fi

"$ROOT/scripts/clean_ipc.sh" || true

SEED=$(date +%s)
PLAYERS=6

"$ROOT/backend/casino_server" --players "$PLAYERS" --seed "$SEED" &
SERVER_PID=$!

echo "[run_demo] casino_server PID=$SERVER_PID"

declare -a PLAYER_PIDS
for i in $(seq 0 $((PLAYERS - 1))); do
  "$ROOT/backend/player" "$i" &
  PLAYER_PIDS+=("$!")
  sleep 0.05
done

cleanup() {
  echo "[run_demo] Stopping processes..."
  kill "$SERVER_PID" ${PLAYER_PIDS[*]} 2>/dev/null || true
  wait "$SERVER_PID" ${PLAYER_PIDS[@]} 2>/dev/null || true
  "$ROOT/scripts/clean_ipc.sh" || true
}
trap cleanup EXIT

echo "[run_demo] Launching viewer..."
"$ROOT/viewer/viewer"
