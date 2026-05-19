#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

OUT_LIB="libmicrobench.dylib"
BASE_FLAGS=(-O3 -std=c11 -fPIC -dynamiclib microbench.c -o "$OUT_LIB" -pthread -ffp-contract=fast -fstrict-aliasing -funroll-loops)

echo "Kompiluję do: $SCRIPT_DIR/$OUT_LIB"

if clang "${BASE_FLAGS[@]}" -mcpu=native >/dev/null 2>&1; then
  echo "Używam strojenia: -mcpu=native"
  clang "${BASE_FLAGS[@]}" -mcpu=native
elif clang "${BASE_FLAGS[@]}" -march=native >/dev/null 2>&1; then
  echo "Używam strojenia: -march=native"
  clang "${BASE_FLAGS[@]}" -march=native
else
  echo "Używam strojenia: fallback generic"
  clang "${BASE_FLAGS[@]}"
fi

echo "Gotowe. Plik:"
ls -l "$OUT_LIB"
