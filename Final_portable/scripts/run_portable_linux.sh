#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="${ROOT}/.portable_env"
PACKAGE="full"
WORKFLOW=""
PROFILE="paper"
MODE="standard"
BACKEND="auto"
DEVICE_INDEX="0"
CPU_THREADS="0"
FILIP_CASE="portable"
FILIP_MODE="portable_sweep"
MODFEM_DIR=""
WITH_FIGURES=1
WITH_ZIP=1
WITH_APT_BOOTSTRAP="auto"
REPLAY_ROOT="${FILIP_REPLAY_DUMP_ROOT:-}"

usage() {
  cat <<'EOF'
Usage: scripts/run_portable_linux.sh [options]

Portable launcher for Linux hosts.

Packages:
  benchmarks   -> cpu_benchmark + gpu_benchmark (if available)
  real-kernels -> cpu_real_kernels + gpu_real_kernels (if available)
  filip        -> fem_option_validation + filip_original
  full         -> full_thesis_pipeline with Filip portable case

Options:
  --package NAME         benchmarks|real-kernels|filip|full (default: full)
  --workflow NAME        Direct run_workflow target, e.g. ai_accel or filip_original
  --profile NAME         quick|paper|full (default: paper)
  --mode NAME            standard|extended (default: standard)
  --backend NAME         auto|cpu|cuda|hip|opencl|amd|intel (default: auto)
  --device-index N       GPU/OpenCL device index (default: 0)
  --cpu-threads N        Cap CPU thread count for all stages (default: 0 = auto)
  --filip-case NAME      portable|laplace_prism|test_prism|prism_pair (default: portable)
  --filip-mode NAME      portable_sweep|exact_reference (default: portable_sweep)
  --modfem-dir PATH      Optional mod_2022 root for exact_reference on Linux/OpenCL
  --no-figures           Do not regenerate publication figures after runs
  --no-zip               Do not build ZIP after a full campaign
  --with-apt-bootstrap   Allow apt-based bootstrap on Ubuntu if env is missing
  --no-apt-bootstrap     Force bootstrap without apt packages
  --replay-root PATH     Optional replay root for exact-related workflows in full pipeline
  -h, --help             Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --package)
      PACKAGE="$2"
      shift 2
      ;;
    --workflow)
      WORKFLOW="$2"
      shift 2
      ;;
    --profile)
      PROFILE="$2"
      shift 2
      ;;
    --mode)
      MODE="$2"
      shift 2
      ;;
    --backend)
      BACKEND="$2"
      shift 2
      ;;
    --device-index)
      DEVICE_INDEX="$2"
      shift 2
      ;;
    --cpu-threads)
      CPU_THREADS="$2"
      shift 2
      ;;
    --filip-case)
      FILIP_CASE="$2"
      shift 2
      ;;
    --filip-mode)
      FILIP_MODE="$2"
      shift 2
      ;;
    --modfem-dir)
      MODFEM_DIR="$2"
      shift 2
      ;;
    --no-figures)
      WITH_FIGURES=0
      shift
      ;;
    --no-zip)
      WITH_ZIP=0
      shift
      ;;
    --with-apt-bootstrap)
      WITH_APT_BOOTSTRAP="yes"
      shift
      ;;
    --no-apt-bootstrap)
      WITH_APT_BOOTSTRAP="no"
      shift
      ;;
    --replay-root)
      REPLAY_ROOT="$2"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "[ERROR] Unknown option: $1" >&2
      usage
      exit 2
      ;;
  esac
done

if [[ "$(uname -s)" != "Linux" ]]; then
  echo "[ERROR] Portable launcher is prepared for Linux hosts." >&2
  exit 2
fi

if [[ ! -f "${VENV_DIR}/bin/activate" ]]; then
  use_apt=0
  case "${WITH_APT_BOOTSTRAP}" in
    yes)
      use_apt=1
      ;;
    no)
      use_apt=0
      ;;
    auto)
      if command -v apt-get >/dev/null 2>&1; then
        use_apt=1
      fi
      ;;
    *)
      use_apt=0
      ;;
  esac
  BOOTSTRAP_ARGS=()
  if [[ ${use_apt} -eq 1 ]]; then
    BOOTSTRAP_ARGS+=(--with-apt)
  fi
  echo "[INFO] Missing .portable_env -> bootstrapping portable environment"
  set +e
  bash "${ROOT}/scripts/portable_bootstrap_linux.sh" "${BOOTSTRAP_ARGS[@]}"
  bootstrap_rc=$?
  set -e
  if [[ ${bootstrap_rc} -ne 0 ]] && [[ ${use_apt} -eq 0 ]] && command -v apt-get >/dev/null 2>&1; then
    echo "[WARN] Bootstrap without apt failed (rc=${bootstrap_rc}). Retrying with apt bootstrap..."
    bash "${ROOT}/scripts/portable_bootstrap_linux.sh" --with-apt
  elif [[ ${bootstrap_rc} -ne 0 ]]; then
    echo "[ERROR] Portable bootstrap failed with code ${bootstrap_rc}." >&2
    exit ${bootstrap_rc}
  fi
fi

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"
export MPLCONFIGDIR="${ROOT}/.cache/matplotlib"
mkdir -p "${ROOT}/portable"
set +e
python "${ROOT}/scripts/portable_compat_report.py" \
  --json-out "${ROOT}/portable/host_compat.json" \
  --md-out "${ROOT}/portable/host_compat.md" \
  --quiet
compat_rc=$?
set -e
if [[ ${compat_rc} -ne 0 ]]; then
  echo "[WARN] Compatibility report failed (rc=${compat_rc}). Continuing with run."
fi

if [[ -f "${ROOT}/portable/host_compat.json" ]]; then
  python - "${ROOT}/portable/host_compat.json" <<'PY'
import json
import sys
from pathlib import Path

path = Path(sys.argv[1])
try:
    payload = json.loads(path.read_text(encoding="utf-8"))
except Exception as exc:
    print(f"[WARN] Cannot read host_compat.json: {exc}")
    raise SystemExit(0)

gpu = payload.get("gpu", {}) or {}
statuses = gpu.get("backend_status", {}) or {}
available = [name for name, data in statuses.items() if bool((data or {}).get("available"))]
print(f"[INFO] GPU backends available: {', '.join(available) if available else 'none'}")
for name in ("cuda", "hip", "opencl", "metal"):
    item = statuses.get(name, {}) or {}
    if bool(item.get("available")):
        continue
    reason = str(item.get("reason") or "").strip()
    if reason:
        print(f"[INFO] backend={name} unavailable -> {reason}")
PY
fi

COMMON_ARGS=(
  --profile "${PROFILE}"
  --platform-profile auto
  --backend "${BACKEND}"
  --benchmark-mode "${MODE}"
  --device-index "${DEVICE_INDEX}"
  --benchmarks-max-cpu-threads "${CPU_THREADS}"
  --real-kernels-max-cpu-threads "${CPU_THREADS}"
  --filip-max-cpu-threads "${CPU_THREADS}"
)

run_py() {
  echo "[RUN] $*"
  "$@"
}

run_workflow_cmd() {
  local workflow="$1"
  shift || true
  local cmd=(
    python "${ROOT}/run_workflow.py"
    --workflow "${workflow}"
    "${COMMON_ARGS[@]}"
  )
  if [[ "${workflow}" == "filip_original" ]]; then
    cmd+=(--filip-case "${FILIP_CASE}" --filip-mode "${FILIP_MODE}")
  fi
  if [[ "${workflow}" == "full_thesis_pipeline" ]]; then
    cmd+=(--filip-case "${FILIP_CASE}")
  fi
  if [[ -n "${MODFEM_DIR}" ]] && [[ "${workflow}" == "filip_original" ]]; then
    cmd+=(--filip-modfem-dir "${MODFEM_DIR}")
  fi
  if [[ -n "${REPLAY_ROOT}" ]] && [[ "${workflow}" == "filip_original" || "${workflow}" == "full_thesis_pipeline" ]]; then
    cmd+=(--filip-replay-dump-root "${REPLAY_ROOT}")
  fi
  run_py "${cmd[@]}" "$@"
}

latest_full_campaign() {
  find "${ROOT}/data/thesis_full" -maxdepth 1 -type d -name '*__full_thesis__*' | sort | tail -n 1 || true
}

latest_filip_original() {
  find "${ROOT}/data/optimization" -maxdepth 1 -type d -name '*__filip_original__backend-*' | sort | tail -n 1 || true
}

if [[ -n "${WORKFLOW}" ]]; then
  run_workflow_cmd "${WORKFLOW}"
else
  case "${PACKAGE}" in
    benchmarks)
      run_workflow_cmd cpu_benchmark
      if [[ "${BACKEND}" != "cpu" ]]; then
        run_workflow_cmd gpu_benchmark || true
      fi
      ;;
    real-kernels|real_kernels)
      run_workflow_cmd cpu_real_kernels
      if [[ "${BACKEND}" != "cpu" ]]; then
        run_workflow_cmd gpu_real_kernels || true
      fi
      ;;
    filip)
      run_workflow_cmd fem_option_validation
      run_workflow_cmd filip_original
      ;;
    full)
      run_workflow_cmd full_thesis_pipeline
      ;;
    *)
      echo "[ERROR] Unsupported package: ${PACKAGE}" >&2
      usage
      exit 2
      ;;
  esac
fi

if [[ ${WITH_FIGURES} -eq 1 ]]; then
  echo "[INFO] Regenerating thesis-core figures"
  run_py python "${ROOT}/analysis/generate_plots.py" || true
  FILIP_RUN="$(latest_filip_original)"
  if [[ -n "${FILIP_RUN}" && -d "${FILIP_RUN}" ]]; then
    run_py python "${ROOT}/analysis/filip_article_plots.py" --optimization-dir "${FILIP_RUN}" || true
  fi
fi

if [[ ${WITH_ZIP} -eq 1 && ( "${PACKAGE}" == "full" || "${WORKFLOW}" == "full_thesis_pipeline" ) ]]; then
  CAMPAIGN_DIR="$(latest_full_campaign)"
  if [[ -n "${CAMPAIGN_DIR}" && -d "${CAMPAIGN_DIR}" ]]; then
    echo "[INFO] Building ZIP for latest full campaign"
    run_py python "${ROOT}/analysis/build_plot_zip.py" --campaign-dir "${CAMPAIGN_DIR}" || true
  fi
fi

echo "[OK] Portable run finished"
echo "[INFO] Compatibility report: ${ROOT}/portable/host_compat.md"
echo "[INFO] Data root: ${ROOT}/data"
