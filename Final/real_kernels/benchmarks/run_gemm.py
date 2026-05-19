#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys
from typing import Dict, List, Tuple

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from real_kernels.common import append_rows, base_meta, make_csv_path, run_warmups
from real_kernels.cpu_backend import CpuRealBackend
from real_kernels.cuda_backend import CudaRealBackend
from real_kernels.metal_backend import MetalRealBackend


def _parse_shapes(raw: str) -> List[Tuple[int, int, int]]:
    # format: "512x512x512,1024x1024x1024"
    out: List[Tuple[int, int, int]] = []
    for part in raw.split(","):
        p = part.strip().lower()
        if not p:
            continue
        m, n, k = [int(x) for x in p.split("x")]
        out.append((m, n, k))
    return out


def _run_backend(backend: str, device_index: int, shapes: List[Tuple[int, int, int]], runs: int, warmups: int, dtype: str) -> None:
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

    csv_path = make_csv_path("gemm", backend, device_name, device_index)
    rows: List[Dict[str, object]] = []

    print(f"=== REAL GEMM ({backend}) ===")
    print(f"device: {device_name} (index {device_index})")
    print(f"runs  : {runs}, warmups: {warmups}")

    for (m, n, k) in shapes:
        print(f"\n--- shape: {m}x{n}x{k} ---")
        run_warmups(warmups, lambda: be.gemm(m=m, n=n, k=k, dtype=dtype))
        for run_idx in range(runs):
            status = "ok"
            err = ""
            elapsed = float("nan")
            gflops = float("nan")
            try:
                elapsed, gflops = be.gemm(m=m, n=n, k=k, dtype=dtype)
            except Exception as e:
                status = "error"
                err = str(e)

            print(f"run {run_idx:2d}: status={status}, elapsed={elapsed:.6f}s, gflops={gflops:.2f}")
            row = {
                **base_meta(backend, device_name, device_index),
                "kernel_group": "real_kernels",
                "kernel": "gemm",
                "dtype": dtype,
                "m": m,
                "n": n,
                "k": k,
                "run_idx": run_idx,
                "elapsed_s": elapsed,
                "gflops": gflops,
                "status": status,
                "error": err,
            }
            rows.append(row)

    append_rows(csv_path, rows)
    print(f"\nCSV: {csv_path}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Real-kernel GEMM benchmark (separate module).")
    ap.add_argument("--backend", choices=["cpu", "cuda", "metal", "all"], default="all")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--warmups", type=int, default=0)
    ap.add_argument("--dtype", choices=["float32", "float64"], default="float32")
    ap.add_argument("--shapes", type=str, default="512x512x512,1024x1024x1024")
    args = ap.parse_args()

    shapes = _parse_shapes(args.shapes)
    backends = ["cpu", "cuda", "metal"] if args.backend == "all" else [args.backend]

    for b in backends:
        try:
            _run_backend(
                backend=b,
                device_index=args.device_index,
                shapes=shapes,
                runs=args.runs,
                warmups=args.warmups,
                dtype=args.dtype,
            )
        except Exception as e:
            print(f"[WARN] backend={b} skipped: {e}")


if __name__ == "__main__":
    main()
