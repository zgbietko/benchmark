#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_DEFAULT="${ROOT}/Kod Filipa/mod_2022"
OUT_DEFAULT="${ROOT}/legacy/filip_exact_bundle/mod_2022"
SRC="${SRC_DEFAULT}"
OUT="${OUT_DEFAULT}"
MAKE_TAR=0
TAR_PATH=""

usage() {
  cat <<'EOF'
Usage: scripts/export_filip_exact_bundle.sh [options]

Create a minimal copy of Filip's mod_2022 tree for exact OpenCL reference runs.
This keeps only the source tree and the two prism workspaces used by the exact runner.

Options:
  --src PATH        Source mod_2022 directory (default: repo/Kod Filipa/mod_2022)
  --out PATH        Output directory for slim copy (default: repo/legacy/filip_exact_bundle/mod_2022)
  --tar PATH        Also create tar.gz archive at PATH
  -h, --help        Show help
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --src)
      SRC="$2"
      shift 2
      ;;
    --out)
      OUT="$2"
      shift 2
      ;;
    --tar)
      MAKE_TAR=1
      TAR_PATH="$2"
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

if ! command -v rsync >/dev/null 2>&1; then
  echo "[ERROR] rsync is required" >&2
  exit 2
fi

if [[ ! -d "${SRC}" ]]; then
  echo "[ERROR] Source mod_2022 directory not found: ${SRC}" >&2
  exit 2
fi

mkdir -p "$(dirname "${OUT}")"
rm -rf "${OUT}"
mkdir -p "${OUT}"

echo "[INFO] Exporting slim exact-reference bundle"
echo "[INFO] Source: ${SRC}"
echo "[INFO] Output: ${OUT}"

mkdir -p "${OUT}/src" "${OUT}/work"

rsync -a \
  --exclude='.git/' \
  --exclude='.git_backup_embedded/' \
  --exclude='.metadata/' \
  --exclude='.settings/' \
  --exclude='**/.DS_Store' \
  --exclude='**/*.o' \
  "${SRC}/src/" "${OUT}/src/"

for work_name in diff_in_box test_scalar; do
  rsync -a \
    --exclude='result*.csv' \
    --exclude='result*.txt' \
    --exclude='Wyniki/' \
    --exclude='exec/' \
    --exclude='kernele/' \
    --exclude='**/.DS_Store' \
    "${SRC}/work/${work_name}/" "${OUT}/work/${work_name}/"
done

cat > "${OUT}/README_exact_bundle.md" <<'EOF'
# Filip exact bundle

This directory is a Git-safe minimal snapshot of `mod_2022` for exact OpenCL
reference runs used by:

- `run_filip_reference_exact.py`
- `run_workflow.py --workflow filip_original --filip-mode exact_reference`

It intentionally contains only:

- `src/`
- `work/diff_in_box/`
- `work/test_scalar/`

That is enough for:

- `laplace_prism`
- `test_prism`
- `prism_pair`

It does not include historical results, unrelated workspaces, build outputs,
or the original embedded `.git` metadata.
EOF

cat > "${OUT}/.gitignore" <<'EOF'
bin/
obj/
traces/
work/diff_in_box/result*
work/test_scalar/result*
work/diff_in_box/*.log
work/test_scalar/*.log
EOF

echo
echo "[INFO] Slim bundle size:"
du -sh "${OUT}"

if [[ ${MAKE_TAR} -eq 1 ]]; then
  if [[ -z "${TAR_PATH}" ]]; then
    echo "[ERROR] --tar requires a path" >&2
    exit 2
  fi
  mkdir -p "$(dirname "${TAR_PATH}")"
  tar -czf "${TAR_PATH}" -C "$(dirname "${OUT}")" "$(basename "${OUT}")"
  echo "[INFO] Archive created: ${TAR_PATH}"
  du -sh "${TAR_PATH}"
fi

echo
echo "[OK] Bundle prepared."
echo "[INFO] On the target machine use:"
echo "       python run_filip_reference_exact.py --modfem-dir \"${OUT}\" --backend intel --benchmark-case prism_pair"
