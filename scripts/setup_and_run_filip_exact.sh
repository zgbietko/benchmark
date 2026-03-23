#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CASE="laplace_prism"
BACKEND="intel"
SKIP_INSTALL=0
SKIP_RUN=0
PYTHON_BIN=""
ARCH_LAPLACE=""
ARCH_TEST=""

usage() {
  cat <<'EOF'
Usage: scripts/setup_and_run_filip_exact.sh [options]

Install Intel oneAPI Base Toolkit on Ubuntu, load oneAPI environment, activate
the repo virtualenv if present, and run Filip exact-reference benchmark.

Options:
  --case NAME         laplace_prism | test_prism | prism_pair (default: laplace_prism)
  --backend NAME      intel | opencl | auto (default: intel)
  --skip-install      Skip apt/repository installation steps
  --skip-run          Prepare environment only, do not launch the benchmark
  --python BIN        Python interpreter to use if .venv is missing (default: python3)
  --arch-laplace STR  Override --arch-laplace for run_workflow.py
  --arch-test STR     Override --arch-test for run_workflow.py
  -h, --help          Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --case)
      CASE="$2"
      shift 2
      ;;
    --backend)
      BACKEND="$2"
      shift 2
      ;;
    --skip-install)
      SKIP_INSTALL=1
      shift
      ;;
    --skip-run)
      SKIP_RUN=1
      shift
      ;;
    --python)
      PYTHON_BIN="$2"
      shift 2
      ;;
    --arch-laplace)
      ARCH_LAPLACE="$2"
      shift 2
      ;;
    --arch-test)
      ARCH_TEST="$2"
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

case "${CASE}" in
  laplace_prism|test_prism|prism_pair) ;;
  *)
    echo "[ERROR] Unsupported case: ${CASE}" >&2
    exit 2
    ;;
esac

case "${BACKEND}" in
  intel|opencl|auto) ;;
  *)
    echo "[ERROR] Unsupported backend: ${BACKEND}" >&2
    exit 2
    ;;
esac

run_root() {
  if [[ "$(id -u)" -eq 0 ]]; then
    "$@"
  else
    sudo "$@"
  fi
}

install_oneapi_repo() {
  local keyring="/usr/share/keyrings/oneapi-archive-keyring.gpg"
  local listfile="/etc/apt/sources.list.d/oneAPI.list"
  if [[ ! -f "${keyring}" ]]; then
    wget -O- https://apt.repos.intel.com/intel-gpg-keys/GPG-PUB-KEY-INTEL-SW-PRODUCTS.PUB \
      | gpg --dearmor | run_root tee "${keyring}" > /dev/null
  fi
  if [[ ! -f "${listfile}" ]] || ! grep -q "apt.repos.intel.com/oneapi" "${listfile}" 2>/dev/null; then
    echo "deb [signed-by=${keyring}] https://apt.repos.intel.com/oneapi all main" \
      | run_root tee "${listfile}" > /dev/null
  fi
}

install_packages() {
  echo "[INFO] Installing Ubuntu prerequisites and Intel oneAPI Base Toolkit"
  run_root apt-get update
  run_root apt-get install -y wget gpg ca-certificates software-properties-common
  install_oneapi_repo
  run_root apt-get update
  run_root apt-get install -y intel-oneapi-base-toolkit tcsh
}

source_oneapi() {
  if [[ -f /opt/intel/oneapi/setvars.sh ]]; then
    # shellcheck disable=SC1091
    source /opt/intel/oneapi/setvars.sh > /dev/null
    return 0
  fi

  local latest_vars
  latest_vars="$(find /opt/intel/oneapi -maxdepth 2 -type f -name oneapi-vars.sh 2>/dev/null | sort | tail -n 1 || true)"
  if [[ -n "${latest_vars}" ]]; then
    # shellcheck disable=SC1090
    source "${latest_vars}" > /dev/null
    return 0
  fi

  echo "[ERROR] Could not find /opt/intel/oneapi/setvars.sh or oneapi-vars.sh" >&2
  exit 2
}

activate_python() {
  if [[ -f "${ROOT}/.venv/bin/activate" ]]; then
    # shellcheck disable=SC1091
    source "${ROOT}/.venv/bin/activate"
    return 0
  fi

  if [[ -n "${PYTHON_BIN}" ]]; then
    export PYTHON_FALLBACK="${PYTHON_BIN}"
    return 0
  fi

  if command -v python3 >/dev/null 2>&1; then
    export PYTHON_FALLBACK="python3"
    return 0
  fi

  echo "[ERROR] No Python runtime found. Create .venv or pass --python BIN" >&2
  exit 2
}

python_cmd() {
  if [[ -n "${VIRTUAL_ENV:-}" ]]; then
    command -v python
  else
    printf '%s\n' "${PYTHON_FALLBACK}"
  fi
}

run_benchmark() {
  local py
  py="$(python_cmd)"
  local cmd=(
    "${py}" "${ROOT}/run_workflow.py"
    --workflow filip_original
    --backend "${BACKEND}"
    --filip-mode exact_reference
    --filip-case "${CASE}"
  )
  if [[ -n "${ARCH_LAPLACE}" ]]; then
    cmd+=(--arch-laplace "${ARCH_LAPLACE}")
  fi
  if [[ -n "${ARCH_TEST}" ]]; then
    cmd+=(--arch-test "${ARCH_TEST}")
  fi

  echo "[INFO] Running exact Filip benchmark:"
  printf '       %q ' "${cmd[@]}"
  echo
  "${cmd[@]}"
}

echo "[INFO] Repo root: ${ROOT}"

if [[ ${SKIP_INSTALL} -eq 0 ]]; then
  install_packages
fi

source_oneapi

if ! command -v icx >/dev/null 2>&1; then
  echo "[ERROR] icx is still not available after loading oneAPI environment." >&2
  echo "        Check Intel installation or reopen the shell and source /opt/intel/oneapi/setvars.sh" >&2
  exit 2
fi

echo "[OK] icx detected: $(command -v icx)"
icx --version | head -n 1 || true

activate_python

if [[ ${SKIP_RUN} -eq 1 ]]; then
  echo "[OK] Environment prepared. Benchmark not launched because --skip-run was set."
  exit 0
fi

run_benchmark
