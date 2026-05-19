#!/usr/bin/env bash
set -euo pipefail

usage() {
  cat <<'EOF'
Usage: scripts/cleanup_oneapi_heavy_install.sh [options]

Remove the heavy Intel oneAPI Base Toolkit meta-package and clean apt caches.
This is useful if a previous exact-reference setup consumed too much disk space.

Options:
  --purge-all-oneapi   Also purge all installed intel-oneapi-* packages
  -h, --help           Show help
EOF
}

PURGE_ALL=0

while [[ $# -gt 0 ]]; do
  case "$1" in
    --purge-all-oneapi)
      PURGE_ALL=1
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

run_root() {
  if [[ "$(id -u)" -eq 0 ]]; then
    "$@"
  else
    sudo "$@"
  fi
}

echo "[INFO] Disk usage before cleanup:"
df -h /
du -sh /var/cache/apt/archives 2>/dev/null || true
du -sh /opt/intel 2>/dev/null || true

if dpkg -s intel-oneapi-base-toolkit >/dev/null 2>&1; then
  echo "[INFO] Removing intel-oneapi-base-toolkit meta-package"
  run_root apt-get remove -y intel-oneapi-base-toolkit
fi

if [[ ${PURGE_ALL} -eq 1 ]]; then
  mapfile -t oneapi_pkgs < <(dpkg-query -W -f='${Package}\n' 'intel-oneapi-*' 2>/dev/null | sort -u || true)
  if [[ ${#oneapi_pkgs[@]} -gt 0 ]]; then
    echo "[INFO] Purging all installed Intel oneAPI packages"
    run_root apt-get purge -y "${oneapi_pkgs[@]}"
  fi
fi

echo "[INFO] Running autoremove and cache cleanup"
run_root apt-get autoremove -y --purge
run_root apt-get clean
run_root rm -rf /var/lib/apt/lists/*

echo "[INFO] Disk usage after cleanup:"
df -h /
du -sh /var/cache/apt/archives 2>/dev/null || true
du -sh /opt/intel 2>/dev/null || true

echo
echo "[OK] Cleanup finished."
echo "[INFO] For exact Filip runs, reinstall only the minimal set with:"
echo "       ./scripts/setup_and_run_filip_exact.sh --skip-run"
