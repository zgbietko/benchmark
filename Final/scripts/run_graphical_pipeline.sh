#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [[ -f ".venv/bin/activate" ]]; then
  # shellcheck disable=SC1091
  source .venv/bin/activate
fi

HOST="${PIPELINE_WEB_HOST:-127.0.0.1}"
PORT="${PIPELINE_WEB_PORT:-8765}"

echo "Panel Final: http://${HOST}:${PORT}/"
exec python3 web/pipeline_server.py --host "$HOST" --port "$PORT" "$@"
