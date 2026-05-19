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

exec "${PY_BIN}" run_desktop_gui.py "$@"
