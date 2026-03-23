#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="${ROOT}/.venv"
PYTHON_BIN="${PYTHON_BIN:-python3}"
WITH_APT=1
INSTALL_GUI=1
BUILD_OPTIONAL_LIBS=0
CUDA_WHEEL="auto"

usage() {
  cat <<'EOF'
Usage: scripts/setup_ubuntu_filip.sh [options]

Options:
  --venv PATH              Virtualenv path (default: .venv in repo root)
  --python BIN             Python interpreter (default: python3)
  --no-apt                 Skip apt packages
  --no-gui                 Skip tkinter package
  --build-optional-libs    Try to build cpu/lib and gpu/cuda/lib if toolchains exist
  --cuda-wheel NAME        auto|skip|cupy-cuda11x|cupy-cuda12x
  -h, --help               Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --venv)
      VENV_DIR="$2"
      shift 2
      ;;
    --python)
      PYTHON_BIN="$2"
      shift 2
      ;;
    --no-apt)
      WITH_APT=0
      shift
      ;;
    --no-gui)
      INSTALL_GUI=0
      shift
      ;;
    --build-optional-libs)
      BUILD_OPTIONAL_LIBS=1
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

need_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "[ERROR] Missing command: $1" >&2
    exit 2
  fi
}

run_root() {
  if [[ "$(id -u)" -eq 0 ]]; then
    "$@"
  else
    sudo "$@"
  fi
}

apt_install_if_available() {
  local install_list=()
  local pkg
  for pkg in "$@"; do
    if apt-cache show "$pkg" >/dev/null 2>&1; then
      install_list+=("$pkg")
    fi
  done
  if [[ ${#install_list[@]} -gt 0 ]]; then
    run_root apt-get install -y "${install_list[@]}"
  fi
}

detect_cuda_wheel() {
  if [[ "${CUDA_WHEEL}" != "auto" ]]; then
    echo "${CUDA_WHEEL}"
    return 0
  fi

  local version=""
  if command -v nvcc >/dev/null 2>&1; then
    version="$(nvcc --version | sed -n 's/.*release \([0-9][0-9]*\)\.\([0-9][0-9]*\).*/\1.\2/p' | tail -n 1)"
  fi
  if [[ -z "${version}" ]] && command -v nvidia-smi >/dev/null 2>&1; then
    version="$(nvidia-smi | sed -n 's/.*CUDA Version: \([0-9][0-9]*\.[0-9][0-9]*\).*/\1/p' | head -n 1)"
  fi

  case "${version%%.*}" in
    12) echo "cupy-cuda12x" ;;
    11) echo "cupy-cuda11x" ;;
    *) echo "" ;;
  esac
}

need_cmd "${PYTHON_BIN}"

if [[ ${WITH_APT} -eq 1 ]]; then
  need_cmd apt-get
  echo "[INFO] Installing Ubuntu packages..."
  run_root apt-get update
  BASE_PKGS=(
    build-essential
    python3-dev
    python3-venv
    python3-pip
    git
    clinfo
    ocl-icd-libopencl1
    ocl-icd-opencl-dev
    opencl-headers
    pciutils
    libboost-system-dev
    libboost-filesystem-dev
    libboost-regex-dev
    libconfig-dev
  )
  if [[ ${INSTALL_GUI} -eq 1 ]]; then
    BASE_PKGS+=(python3-tk)
  fi
  run_root apt-get install -y "${BASE_PKGS[@]}"
  apt_install_if_available intel-opencl-icd intel-level-zero-gpu libze1 level-zero
fi

echo "[INFO] Creating virtualenv: ${VENV_DIR}"
"${PYTHON_BIN}" -m venv "${VENV_DIR}"
source "${VENV_DIR}/bin/activate"

python -m pip install --upgrade pip setuptools wheel
python -m pip install -r "${ROOT}/requirements-ubuntu.txt"

CUDA_PACKAGE="$(detect_cuda_wheel)"
if [[ -n "${CUDA_PACKAGE}" ]] && [[ "${CUDA_PACKAGE}" != "skip" ]]; then
  echo "[INFO] Installing CUDA Python package: ${CUDA_PACKAGE}"
  python -m pip install "${CUDA_PACKAGE}"
else
  echo "[WARN] CUDA wheel was not auto-detected. If you need CUDA Filip runs, install manually:"
  echo "       python -m pip install cupy-cuda12x"
fi

python - <<'PY'
mods = ("numpy", "matplotlib", "pyopencl", "pynvml")
for name in mods:
    try:
        __import__(name)
        print(f"[OK] import {name}")
    except Exception as exc:
        print(f"[WARN] import {name}: {type(exc).__name__}: {exc}")
try:
    import cupy  # type: ignore
    print("[OK] import cupy")
except Exception as exc:
    print(f"[WARN] import cupy: {type(exc).__name__}: {exc}")
PY

echo "[INFO] Building native microbench libraries where toolchains are available..."
if command -v gcc >/dev/null 2>&1; then
  (cd "${ROOT}/cpu/lib" && bash ./build_linux.sh) || true
fi
if command -v nvcc >/dev/null 2>&1; then
  (cd "${ROOT}/gpu/cuda/lib" && bash ./build_cuda.sh) || true
elif [[ "${CUDA_PACKAGE}" != "skip" ]]; then
  echo "[WARN] nvcc not found, so libgpubench_cuda.so was not built."
  echo "       CUDA discovery inside this project will stay unavailable until nvcc/toolkit is installed."
fi

if [[ ${BUILD_OPTIONAL_LIBS} -eq 1 ]]; then
  echo "[INFO] Optional build flag enabled. No additional Linux native libraries beyond CPU/CUDA are required here."
fi

echo
echo "[OK] Ubuntu Filip environment is prepared."
echo "[INFO] Activate it with:"
echo "       source \"${VENV_DIR}/bin/activate\""
echo "[INFO] Preflight:"
echo "       python run_fem_parametric_preflight.py --backend cpu,cuda,intel --platform-profile auto"
echo "[INFO] Device discovery:"
echo "       python run_device_discovery.py --backends cuda,opencl"
echo "[INFO] Full validation:"
echo "       python scripts/run_ubuntu_filip_validation.py --profile paper"
echo "[INFO] Exact Filip reference (requires original mod_2022 OpenCL toolchain):"
echo "       python run_workflow.py --workflow filip_original --backend intel --filip-mode exact_reference --filip-case prism_pair"
