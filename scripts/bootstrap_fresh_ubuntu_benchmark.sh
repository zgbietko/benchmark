#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VENV_DIR="${ROOT}/.venv"
PYTHON_BIN="${PYTHON_BIN:-python3}"
INSTALL_GUI=1
INSTALL_NVIDIA_DRIVER="auto"
INSTALL_ONEAPI_EXACT=1
INSTALL_CUDA_TOOLKIT=1
RUN_PREFLIGHT=1
BUILD_OPTIONAL_LIBS=1
TARGET_USER="${SUDO_USER:-${USER}}"
CUDA_APT_PKG="${CUDA_APT_PKG:-nvidia-cuda-toolkit}"
SETUP_GIT=1
SETUP_GITHUB_SSH=1
GIT_NAME="${GIT_NAME:-}"
GIT_EMAIL="${GIT_EMAIL:-}"

usage() {
  cat <<'EOF'
Usage: scripts/bootstrap_fresh_ubuntu_benchmark.sh [options]

Bootstrap this benchmark repo on a fresh Ubuntu install.

Default profile:
  - installs Ubuntu build/python/OpenCL prerequisites
  - installs Intel OpenCL runtime when available
  - installs minimal Intel oneAPI pieces for exact Filip runs
  - creates/updates .venv and Python deps
  - builds native CPU library
  - installs NVIDIA driver automatically only if nvidia-smi is missing
  - installs CUDA toolkit
  - configures Git identity and GitHub SSH key for push/pull

Options:
  --venv PATH                Virtualenv path (default: .venv in repo root)
  --python BIN               Python interpreter (default: python3)
  --no-gui                   Skip tkinter package
  --skip-nvidia-driver       Never touch NVIDIA driver packages
  --force-nvidia-driver      Always run ubuntu-drivers install --gpgpu
  --skip-oneapi-exact        Skip Intel oneAPI compiler+MKL install
  --skip-cuda-toolkit        Do not install CUDA toolkit
  --cuda-apt-pkg NAME        CUDA toolkit package name (default: nvidia-cuda-toolkit)
  --skip-git                 Do not configure Git identity
  --skip-github-ssh          Do not generate/configure GitHub SSH key
  --git-name NAME            Set git user.name without prompting
  --git-email EMAIL          Set git user.email without prompting
  --skip-preflight           Skip final preflight checks
  --skip-build-optional-libs Do not try to build native CPU/CUDA libs
  -h, --help                 Show help
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
    --no-gui)
      INSTALL_GUI=0
      shift
      ;;
    --skip-nvidia-driver)
      INSTALL_NVIDIA_DRIVER="no"
      shift
      ;;
    --force-nvidia-driver)
      INSTALL_NVIDIA_DRIVER="yes"
      shift
      ;;
    --skip-oneapi-exact)
      INSTALL_ONEAPI_EXACT=0
      shift
      ;;
    --skip-cuda-toolkit)
      INSTALL_CUDA_TOOLKIT=0
      shift
      ;;
    --cuda-apt-pkg)
      CUDA_APT_PKG="$2"
      shift 2
      ;;
    --skip-git)
      SETUP_GIT=0
      shift
      ;;
    --skip-github-ssh)
      SETUP_GITHUB_SSH=0
      shift
      ;;
    --git-name)
      GIT_NAME="$2"
      shift 2
      ;;
    --git-email)
      GIT_EMAIL="$2"
      shift 2
      ;;
    --skip-preflight)
      RUN_PREFLIGHT=0
      shift
      ;;
    --skip-build-optional-libs)
      BUILD_OPTIONAL_LIBS=0
      shift
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

ensure_ubuntu() {
  if [[ ! -r /etc/os-release ]]; then
    echo "[ERROR] /etc/os-release not found" >&2
    exit 2
  fi
  # shellcheck disable=SC1091
  source /etc/os-release
  if [[ "${ID:-}" != "ubuntu" ]]; then
    echo "[ERROR] This bootstrap is prepared for Ubuntu. Detected: ${ID:-unknown}" >&2
    exit 2
  fi
}

install_base_packages() {
  echo "[INFO] Installing Ubuntu base packages"
  run_root apt-get update
  local base_pkgs=(
    build-essential
    cmake
    pkg-config
    python3-dev
    python3-venv
    python3-pip
    git
    wget
    curl
    gpg
    ca-certificates
    software-properties-common
    ubuntu-drivers-common
    pciutils
    clinfo
    ocl-icd-libopencl1
    ocl-icd-opencl-dev
    opencl-headers
    libboost-system-dev
    libboost-filesystem-dev
    libboost-regex-dev
    libconfig-dev
    tcsh
  )
  if [[ ${INSTALL_GUI} -eq 1 ]]; then
    base_pkgs+=(python3-tk)
  fi
  run_root apt-get install -y "${base_pkgs[@]}"
  apt_install_if_available intel-opencl-icd intel-level-zero-gpu libze1 level-zero csh
}

configure_groups() {
  if id "${TARGET_USER}" >/dev/null 2>&1; then
    echo "[INFO] Adding ${TARGET_USER} to render/video groups"
    run_root usermod -aG render,video "${TARGET_USER}" || true
  fi
}

install_nvidia_driver_if_needed() {
  if [[ "${INSTALL_NVIDIA_DRIVER}" == "no" ]]; then
    echo "[INFO] NVIDIA driver installation skipped by option"
    return 0
  fi

  if command -v nvidia-smi >/dev/null 2>&1 && [[ "${INSTALL_NVIDIA_DRIVER}" == "auto" ]]; then
    echo "[INFO] NVIDIA driver already present (nvidia-smi found)"
    return 0
  fi

  echo "[INFO] Installing NVIDIA driver via ubuntu-drivers --gpgpu"
  run_root ubuntu-drivers install --gpgpu || {
    echo "[WARN] ubuntu-drivers install --gpgpu failed"
    echo "       You may need to install the driver manually after reboot."
  }
}

install_cuda_toolkit_if_requested() {
  if [[ ${INSTALL_CUDA_TOOLKIT} -eq 0 ]]; then
    echo "[INFO] CUDA toolkit install skipped (disk-saving default)"
    return 0
  fi

  echo "[INFO] Installing CUDA toolkit package: ${CUDA_APT_PKG}"
  apt_install_if_available "${CUDA_APT_PKG}"
}

install_oneapi_minimal() {
  if [[ ${INSTALL_ONEAPI_EXACT} -eq 0 ]]; then
    echo "[INFO] oneAPI exact toolchain install skipped by option"
    return 0
  fi

  echo "[INFO] Installing minimal Intel oneAPI toolchain for exact Filip runs"
  install_oneapi_repo
  run_root apt-get update
  local compiler_pkg=""
  if apt-cache show intel-oneapi-compiler-dpcpp-cpp-and-cpp-classic >/dev/null 2>&1; then
    compiler_pkg="intel-oneapi-compiler-dpcpp-cpp-and-cpp-classic"
  elif apt-cache show intel-oneapi-compiler-dpcpp-cpp >/dev/null 2>&1; then
    compiler_pkg="intel-oneapi-compiler-dpcpp-cpp"
  else
    echo "[ERROR] No supported Intel oneAPI compiler package found in the configured Intel APT repo." >&2
    exit 2
  fi
  run_root apt-get install -y "${compiler_pkg}" intel-oneapi-mkl-devel
}

source_oneapi_safe() {
  local oneapi_file=""
  if [[ -f /opt/intel/oneapi/setvars.sh ]]; then
    oneapi_file="/opt/intel/oneapi/setvars.sh"
  else
    oneapi_file="$(find /opt/intel/oneapi -maxdepth 2 -type f -name oneapi-vars.sh 2>/dev/null | sort | tail -n 1 || true)"
  fi

  if [[ -z "${oneapi_file}" ]]; then
    return 1
  fi

  export OCL_ICD_FILENAMES="${OCL_ICD_FILENAMES-}"
  set +u
  # shellcheck disable=SC1090
  source "${oneapi_file}" > /dev/null
  set -u
  return 0
}

ensure_venv_and_python_deps() {
  echo "[INFO] Creating/updating Python environment"
  "${PYTHON_BIN}" -m venv "${VENV_DIR}"
  # shellcheck disable=SC1091
  source "${VENV_DIR}/bin/activate"
  python -m pip install --upgrade pip setuptools wheel
  python -m pip install -r "${ROOT}/requirements-ubuntu.txt"
}

prompt_if_empty() {
  local var_name="$1"
  local prompt_text="$2"
  local current_value="$3"
  if [[ -n "${current_value}" ]]; then
    printf '%s' "${current_value}"
    return 0
  fi
  if [[ -t 0 ]]; then
    local value=""
    read -r -p "${prompt_text}" value
    printf '%s' "${value}"
    return 0
  fi
  printf '%s' ""
}

configure_git_identity() {
  if [[ ${SETUP_GIT} -eq 0 ]]; then
    echo "[INFO] Git identity setup skipped by option"
    return 0
  fi

  local resolved_name resolved_email
  resolved_name="$(prompt_if_empty "GIT_NAME" "Git user.name [Mateusz Nytko]: " "${GIT_NAME}")"
  resolved_email="$(prompt_if_empty "GIT_EMAIL" "Git user.email [nytko.mateusz@gmail.com]: " "${GIT_EMAIL}")"

  if [[ -z "${resolved_name}" ]]; then
    resolved_name="Mateusz Nytko"
  fi
  if [[ -z "${resolved_email}" ]]; then
    resolved_email="nytko.mateusz@gmail.com"
  fi

  git config --global user.name "${resolved_name}"
  git config --global user.email "${resolved_email}"
  git config --global pull.rebase false

  echo "[INFO] Git identity configured:"
  echo "       user.name=${resolved_name}"
  echo "       user.email=${resolved_email}"
}

github_ssh_key_path() {
  local home_dir
  home_dir="$(getent passwd "${TARGET_USER}" | cut -d: -f6)"
  if [[ -z "${home_dir}" ]]; then
    home_dir="${HOME}"
  fi
  printf '%s/.ssh/id_ed25519\n' "${home_dir}"
}

setup_github_ssh_key() {
  if [[ ${SETUP_GITHUB_SSH} -eq 0 ]]; then
    echo "[INFO] GitHub SSH setup skipped by option"
    return 0
  fi

  local key_path pub_path ssh_dir comment home_dir
  key_path="$(github_ssh_key_path)"
  pub_path="${key_path}.pub"
  ssh_dir="$(dirname "${key_path}")"
  home_dir="$(dirname "${ssh_dir}")"
  comment="$(git config --global --get user.email || true)"
  if [[ -z "${comment}" ]]; then
    comment="${TARGET_USER}@$(hostname)"
  fi

  run_root mkdir -p "${ssh_dir}"
  run_root chown -R "${TARGET_USER}:${TARGET_USER}" "${ssh_dir}"
  run_root chmod 700 "${ssh_dir}"

  if [[ ! -f "${key_path}" ]]; then
    echo "[INFO] Generating GitHub SSH key for ${TARGET_USER}"
    run_root -u "${TARGET_USER}" ssh-keygen -t ed25519 -C "${comment}" -f "${key_path}" -N ""
  else
    echo "[INFO] Reusing existing SSH key: ${key_path}"
  fi

  local ssh_config="${ssh_dir}/config"
  if ! run_root -u "${TARGET_USER}" grep -q "Host github.com" "${ssh_config}" 2>/dev/null; then
    cat <<EOF | run_root -u "${TARGET_USER}" tee -a "${ssh_config}" > /dev/null
Host github.com
  HostName github.com
  User git
  IdentityFile ${key_path}
  IdentitiesOnly yes
EOF
    run_root chmod 600 "${ssh_config}"
  fi

  mkdir -p "${ROOT}/scripts/generated"
  run_root cp "${pub_path}" "${ROOT}/scripts/generated/github_id_ed25519.pub"
  run_root chown "${TARGET_USER}:${TARGET_USER}" "${ROOT}/scripts/generated/github_id_ed25519.pub"

  if git -C "${ROOT}" remote get-url origin >/dev/null 2>&1; then
    local origin
    origin="$(git -C "${ROOT}" remote get-url origin)"
    if [[ "${origin}" =~ ^https://github.com/([^/]+)/(.+)\.git$ ]]; then
      local owner="${BASH_REMATCH[1]}"
      local repo="${BASH_REMATCH[2]}"
      git -C "${ROOT}" remote set-url origin "git@github.com:${owner}/${repo}.git"
    fi
  fi

  echo "[INFO] GitHub public key saved to:"
  echo "       ${ROOT}/scripts/generated/github_id_ed25519.pub"
  echo "[WARN] Add this key to GitHub before the first push:"
  echo "       https://github.com/settings/keys"
}

build_project_bits() {
  local args=(--venv "${VENV_DIR}" --python "${PYTHON_BIN}" --no-apt)
  if [[ ${INSTALL_GUI} -eq 0 ]]; then
    args+=(--no-gui)
  fi
  if [[ ${BUILD_OPTIONAL_LIBS} -eq 1 ]]; then
    args+=(--build-optional-libs)
  fi
  echo "[INFO] Running project setup helper"
  bash "${ROOT}/scripts/setup_ubuntu_filip.sh" "${args[@]}"
}

write_activation_helper() {
  mkdir -p "${ROOT}/scripts/generated"
  cat > "${ROOT}/scripts/generated/activate_benchmark_env.sh" <<EOF
#!/usr/bin/env bash
set -euo pipefail
source "${VENV_DIR}/bin/activate"
if [[ -f /opt/intel/oneapi/setvars.sh ]]; then
  export OCL_ICD_FILENAMES="\${OCL_ICD_FILENAMES-}"
  set +u
  # shellcheck disable=SC1091
  source /opt/intel/oneapi/setvars.sh > /dev/null
  set -u
fi
export MPLCONFIGDIR="${ROOT}/.cache/matplotlib"
echo "[OK] benchmark env active"
EOF
  chmod +x "${ROOT}/scripts/generated/activate_benchmark_env.sh"
}

run_preflight_checks() {
  if [[ ${RUN_PREFLIGHT} -eq 0 ]]; then
    return 0
  fi

  echo "[INFO] Running preflight"
  # shellcheck disable=SC1091
  source "${VENV_DIR}/bin/activate"
  source_oneapi_safe || true

  python "${ROOT}/run_fem_parametric_preflight.py" --backend cpu,intel --platform-profile auto || true
  python "${ROOT}/run_device_discovery.py" --backends intel,opencl || true

  if command -v nvidia-smi >/dev/null 2>&1; then
    python "${ROOT}/run_fem_parametric_preflight.py" --backend cuda --platform-profile auto || true
    python "${ROOT}/run_device_discovery.py" --backends cuda || true
  fi
}

print_summary() {
  echo
  echo "[OK] Fresh Ubuntu bootstrap completed."
  echo "[INFO] Activation helper:"
  echo "       source \"${ROOT}/scripts/generated/activate_benchmark_env.sh\""
  if [[ ${SETUP_GITHUB_SSH} -eq 1 ]]; then
    echo "[INFO] GitHub SSH public key:"
    echo "       ${ROOT}/scripts/generated/github_id_ed25519.pub"
  fi
  echo "[INFO] Portable workflow:"
  echo "       python run_workflow.py --workflow filip_original --backend cpu --filip-mode portable_sweep --filip-case laplace_prism"
  echo "[INFO] Exact Filip Intel/OpenCL workflow:"
  echo "       python run_workflow.py --workflow filip_original --backend intel --filip-mode exact_reference --filip-case laplace_prism"
  if [[ ${INSTALL_CUDA_TOOLKIT} -eq 0 ]]; then
    echo "[WARN] CUDA toolkit was intentionally skipped."
    echo "       If you want to build the CUDA backend later, rerun with:"
    echo "       ./scripts/bootstrap_fresh_ubuntu_benchmark.sh --skip-oneapi-exact --skip-preflight"
  else
    echo "[INFO] CUDA toolkit package requested:"
    echo "       ${CUDA_APT_PKG}"
  fi
  echo "[WARN] If this was the first time adding ${TARGET_USER} to render/video groups, log out and back in before Intel OpenCL tests."
}

need_cmd "${PYTHON_BIN}"
need_cmd apt-get
need_cmd wget
need_cmd gpg
ensure_ubuntu

echo "[INFO] Repo root: ${ROOT}"
echo "[INFO] Disk usage before bootstrap:"
df -h /

install_base_packages
configure_groups
install_nvidia_driver_if_needed
install_cuda_toolkit_if_requested
install_oneapi_minimal
configure_git_identity
setup_github_ssh_key
ensure_venv_and_python_deps
build_project_bits
write_activation_helper
run_preflight_checks
print_summary
