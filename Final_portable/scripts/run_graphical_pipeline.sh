#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

PY_BIN="python3"
if [[ -f ".venv/bin/activate" ]]; then
  # shellcheck disable=SC1091
  source .venv/bin/activate
  PY_BIN="${ROOT}/.venv/bin/python"
elif [[ -f ".portable_env/bin/activate" ]]; then
  # shellcheck disable=SC1091
  source .portable_env/bin/activate
  PY_BIN="${ROOT}/.portable_env/bin/python"
fi

HOST="${PIPELINE_WEB_HOST:-127.0.0.1}"
PORT="${PIPELINE_WEB_PORT:-8765}"

echo "Panel Final: http://${HOST}:${PORT}/"
exec "${PY_BIN}" web/pipeline_server.py --host "$HOST" --port "$PORT" "$@"
