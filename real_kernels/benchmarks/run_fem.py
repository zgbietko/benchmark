#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys
from typing import Dict, List

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from real_kernels.common import append_rows, base_meta, make_csv_path
from real_kernels.cpu_backend import CpuRealBackend
from real_kernels.cuda_backend import CudaRealBackend
from real_kernels.metal_backend import MetalRealBackend


def _parse_sizes(raw: str) -> List[int]:
    return [int(x.strip()) for x in raw.split(",") if x.strip()]


def _run_backend(backend: str, device_index: int, sizes: List[int], n_qp: int, runs: int, dtype: str) -> None:
    if backend == "cpu":
        be = CpuRealBackend()
        device_name = be.device_name
    elif backend == "cuda":
        be = CudaRealBackend(device_index=device_index)
        device_name = be.device_name
    elif backend == "metal":
        be = MetalRealBackend(device_index=device_index)
        device_name = be.device_name
    else:
        raise ValueError(backend)

    csv_path = make_csv_path("fem", backend, device_name, device_index)
    rows: List[Dict[str, object]] = []
    print(f"=== REAL FEM ({backend}) ===")
    print(f"device: {device_name} (index {device_index})")
    print(f"runs  : {runs}, n_qp: {n_qp}")

    for n_elem in sizes:
        print(f"\n--- n_elements: {n_elem} ---")
        for run_idx in range(runs):
            status = "ok"
            err = ""
            elapsed = float("nan")
            gflops = float("nan")
            try:
                elapsed, gflops = be.fem_element(n_elements=n_elem, n_qp=n_qp, dtype=dtype)
            except Exception as e:
                status = "error"
                err = str(e)
            print(f"run {run_idx:2d}: status={status}, elapsed={elapsed:.6f}s, gflops={gflops:.2f}")
            rows.append(
                {
                    **base_meta(backend, device_name, device_index),
                    "kernel_group": "real_kernels",
                    "kernel": "fem",
                    "dtype": dtype,
                    "n_elements": n_elem,
                    "n_qp": n_qp,
                    "run_idx": run_idx,
                    "elapsed_s": elapsed,
                    "gflops": gflops,
                    "status": status,
                    "error": err,
                }
            )
    append_rows(csv_path, rows)
    print(f"\nCSV: {csv_path}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Real-kernel FEM-like integration benchmark.")
    ap.add_argument("--backend", choices=["cpu", "cuda", "metal", "all"], default="all")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--dtype", choices=["float32", "float64"], default="float32")
    ap.add_argument("--sizes", type=str, default="100000,500000,1000000")
    ap.add_argument("--n-qp", type=int, default=8)
    args = ap.parse_args()

    sizes = _parse_sizes(args.sizes)
    backends = ["cpu", "cuda", "metal"] if args.backend == "all" else [args.backend]
    for b in backends:
        try:
            _run_backend(b, args.device_index, sizes, args.n_qp, args.runs, args.dtype)
        except Exception as e:
            print(f"[WARN] backend={b} skipped: {e}")


if __name__ == "__main__":
    main()
