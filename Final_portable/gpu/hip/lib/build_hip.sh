#!/usr/bin/env bash
# Builds HIP microbench shared library (AMD / ROCm).
#
# Usage:
#   cd apple_microbench/gpu/hip/lib
#   ./build_hip.sh
#
# Requires hipcc in PATH (ROCm).

set -euo pipefail

SRC="gpubench_hip.cu"

if ! command -v hipcc >/dev/null 2>&1 ; then
    echo "ERROR: hipcc not found in PATH. Install ROCm/HIP first." >&2
    exit 1
fi

UNAME_OUT="$(uname -s)"
case "${UNAME_OUT}" in
    Linux*)
        LIB_NAME="libgpubench_hip.so"
        ;;
    Darwin*)
        LIB_NAME="libgpubench_hip.dylib"
        ;;
    MINGW*|MSYS*|CYGWIN*)
        LIB_NAME="gpubench_hip.dll"
        ;;
    *)
        echo "WARNING: Unknown OS ${UNAME_OUT}, defaulting to .so"
        LIB_NAME="libgpubench_hip.so"
        ;;
esac

echo "Building ${LIB_NAME} from ${SRC} ..."

hipcc -O3 -std=c++17 -fPIC -shared "${SRC}" -o "${LIB_NAME}"

echo "[OK] Built: $(pwd)/${LIB_NAME}"
