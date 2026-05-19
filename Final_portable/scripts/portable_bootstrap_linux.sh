#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="${ROOT}/.portable_env"
PYTHON_BIN="${PYTHON_BIN:-python3}"
WITH_APT=0
BUILD_OPTIONAL_LIBS=1
RUN_COMPAT=1
CUDA_WHEEL="auto"

usage() {
  cat <<'EOF'
Usage: scripts/portable_bootstrap_linux.sh [options]

Portable bootstrap for Linux hosts.
Default behavior:
- creates local .portable_env inside the bundle
- installs Python deps locally
- builds CPU lib and optional CUDA/HIP libs when toolchains are present
- writes compatibility report to portable/host_compat.{json,md}

Options:
  --with-apt              Run Ubuntu apt bootstrap before local setup
  --python BIN            Python interpreter (default: python3)
  --venv PATH             Override local env path (default: .portable_env in bundle root)
  --skip-build            Do not build optional native libs
  --skip-compat           Do not write compatibility report
  --cuda-wheel NAME       auto|skip|cupy-cuda11x|cupy-cuda12x
  -h, --help              Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --with-apt)
      WITH_APT=1
      shift
      ;;
    --python)
      PYTHON_BIN="$2"
      shift 2
      ;;
    --venv)
      VENV_DIR="$2"
      shift 2
      ;;
    --skip-build)
      BUILD_OPTIONAL_LIBS=0
      shift
      ;;
    --skip-compat)
      RUN_COMPAT=0
      shift
      ;;
    --cuda-wheel)
      CUDA_WHEEL="$2"
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
  echo "[ERROR] portable bootstrap is prepared for Linux hosts." >&2
  exit 2
fi

mkdir -p "${ROOT}/portable"
mkdir -p "${ROOT}/.cache/matplotlib"
export MPLCONFIGDIR="${ROOT}/.cache/matplotlib"

SETUP_ARGS=(--venv "${VENV_DIR}" --python "${PYTHON_BIN}" --cuda-wheel "${CUDA_WHEEL}")
if [[ ${WITH_APT} -eq 0 ]]; then
  SETUP_ARGS+=(--no-apt)
fi
if [[ ${BUILD_OPTIONAL_LIBS} -eq 1 ]]; then
  SETUP_ARGS+=(--build-optional-libs)
fi

echo "[INFO] Preparing local portable environment in: ${VENV_DIR}"
bash "${ROOT}/scripts/setup_ubuntu_filip.sh" "${SETUP_ARGS[@]}"

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

if command -v hipcc >/dev/null 2>&1; then
  echo "[INFO] hipcc detected -> building HIP microbench library"
  (cd "${ROOT}/gpu/hip/lib" && bash ./build_hip.sh) || true
fi

if [[ ${RUN_COMPAT} -eq 1 ]]; then
  echo "[INFO] Writing compatibility report"
  python "${ROOT}/scripts/portable_compat_report.py" \
    --json-out "${ROOT}/portable/host_compat.json" \
    --md-out "${ROOT}/portable/host_compat.md" \
    --quiet
  echo "[OK] Compatibility report: ${ROOT}/portable/host_compat.md"
fi

echo "[OK] Portable bootstrap finished"
echo "[INFO] Activate: source \"${VENV_DIR}/bin/activate\""
echo "[INFO] Run tests: bash \"${ROOT}/scripts/run_portable_linux.sh\" --package full"
