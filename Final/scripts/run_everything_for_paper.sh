#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT_DIR"

if [[ -f ".venv/bin/activate" ]]; then
  # shellcheck disable=SC1091
  source ".venv/bin/activate"
fi

REPLAY_ROOT="${1:-${FILIP_REPLAY_DUMP_ROOT:-}}"
DEVICE_INDEX="${DEVICE_INDEX:-0}"

echo "[INFO] repo root: $ROOT_DIR"
if [[ -n "$REPLAY_ROOT" ]]; then
  echo "[INFO] using replay dump root: $REPLAY_ROOT"
fi

PIPELINE_CMD=(
  python3 run_workflow.py
  --workflow full_thesis_pipeline
  --platform-profile auto
  --backend auto
  --device-index "$DEVICE_INDEX"
)

if [[ -n "$REPLAY_ROOT" ]]; then
  PIPELINE_CMD+=(--filip-replay-dump-root "$REPLAY_ROOT")
fi

echo "[RUN] ${PIPELINE_CMD[*]}"
"${PIPELINE_CMD[@]}"

echo "[RUN] python3 analysis/generate_plots.py"
python3 analysis/generate_plots.py

LATEST_FILIP_RUN="$(find "$ROOT_DIR/data/optimization" -maxdepth 1 -type d -name '*__filip_original__backend-*' | sort | tail -n 1 || true)"
if [[ -n "$LATEST_FILIP_RUN" && -d "$LATEST_FILIP_RUN" ]]; then
  echo "[RUN] python3 analysis/filip_article_plots.py --optimization-dir \"$LATEST_FILIP_RUN\""
  python3 analysis/filip_article_plots.py --optimization-dir "$LATEST_FILIP_RUN"
else
  echo "[WARN] no filip_original optimization run found; skipping Filip article plots"
fi

echo "[RUN] python3 analysis/build_plot_zip.py"
python3 analysis/build_plot_zip.py

echo
echo "[OK] pipeline complete"
echo "[INFO] thesis_full: $ROOT_DIR/data/thesis_full"
echo "[INFO] thesis-core figures: $ROOT_DIR/analysis/figures/thesis_core"
if [[ -n "$LATEST_FILIP_RUN" && -d "$LATEST_FILIP_RUN" ]]; then
  echo "[INFO] latest Filip thesis-core: $LATEST_FILIP_RUN/figures/thesis_core"
  echo "[INFO] latest Filip appendix: $LATEST_FILIP_RUN/figures/appendix"
fi
