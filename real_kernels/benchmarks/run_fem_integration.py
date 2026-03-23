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
from real_kernels.hip_backend import HipRealBackend
from real_kernels.opencl_backend import OpenCLRealBackend


def _parse_sizes(raw: str) -> List[int]:
    return [int(x.strip()) for x in raw.split(",") if x.strip()]


def _run_backend(
    backend: str,
    device_index: int,
    sizes: List[int],
    n_qp: int,
    element_type: str,
    operator: str,
    runs: int,
    dtype: str,
) -> None:
    if backend == "cpu":
        be = CpuRealBackend()
        device_name = be.device_name
    elif backend == "cuda":
        be = CudaRealBackend(device_index=device_index)
        device_name = be.device_name
    elif backend == "metal":
        be = MetalRealBackend(device_index=device_index)
        device_name = be.device_name
    elif backend == "hip":
        be = HipRealBackend(device_index=device_index)
        device_name = be.device_name
    elif backend == "opencl":
        be = OpenCLRealBackend(device_index=device_index)
        device_name = be.device_name
    elif backend == "amd":
        try:
            be = HipRealBackend(device_index=device_index)
            device_name = be.device_name
            backend = "hip"
        except Exception:
            be = OpenCLRealBackend(device_index=device_index)
            device_name = be.device_name
            backend = "opencl"
    elif backend == "intel":
        be = OpenCLRealBackend(device_index=device_index)
        device_name = be.device_name
        backend = "opencl"
    else:
        raise ValueError(backend)

    # v2 path to avoid mixing with earlier schema versions
    csv_path = make_csv_path("fem_integration_v2", backend, device_name, device_index)
    rows: List[Dict[str, object]] = []
    print(f"=== REAL FEM INTEGRATION ({element_type}) ({backend}) ===")
    print(f"device: {device_name} (index {device_index})")
    print(f"runs  : {runs}, n_qp: {n_qp}, operator: {operator}")

    for n_elem in sizes:
        print(f"\n--- n_elements: {n_elem} ---")
        for run_idx in range(runs):
            status = "ok"
            err = ""
            elapsed = float("nan")
            gflops = float("nan")
            gbps = float("nan")
            ai = float("nan")
            try:
                if element_type == "tet4":
                    elapsed, gflops, gbps = be.fem_integration_tet4(
                        n_elements=n_elem,
                        n_qp=n_qp,
                        operator=operator,
                        dtype=dtype,
                    )
                elif element_type == "hex8":
                    elapsed, gflops, gbps = be.fem_integration_hex8(
                        n_elements=n_elem,
                        n_qp=n_qp,
                        operator=operator,
                        dtype=dtype,
                    )
                elif element_type == "prism6":
                    elapsed, gflops, gbps = be.fem_integration_prism6(
                        n_elements=n_elem,
                        n_qp=n_qp,
                        operator=operator,
                        dtype=dtype,
                    )
                else:
                    raise ValueError(f"Unsupported element_type: {element_type}")
                ai = gflops / gbps if gbps > 0 else float("nan")
            except Exception as e:
                status = "error"
                err = str(e)

            print(
                f"run {run_idx:2d}: status={status}, elapsed={elapsed:.6f}s, "
                f"gflops={gflops:.2f}, gbps={gbps:.2f}, ai={ai:.3f}"
            )
            rows.append(
                {
                    **base_meta(backend, device_name, device_index),
                    "kernel_group": "real_kernels",
                    "kernel": "fem_integration",
                    "element_type": element_type,
                    "operator": operator,
                    "dtype": dtype,
                    "n_elements": n_elem,
                    "n_qp": n_qp,
                    "run_idx": run_idx,
                    "elapsed_s": elapsed,
                    "gflops": gflops,
                    "throughput_gbps": gbps,
                    "ai_flop_per_byte": ai,
                    "status": status,
                    "error": err,
                }
            )

    append_rows(csv_path, rows)
    print(f"\nCSV: {csv_path}")


def main() -> None:
    ap = argparse.ArgumentParser(description="Real-kernel FEM integration benchmark (tet4/hex8/prism6).")
    ap.add_argument(
        "--backend",
        choices=["cpu", "cuda", "metal", "hip", "opencl", "amd", "intel", "all"],
        default="all",
    )
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--dtype", choices=["float32", "float64"], default="float32")
    ap.add_argument("--sizes", type=str, default="20000,100000,500000")
    ap.add_argument("--element-type", choices=["tet4", "hex8", "prism6"], default="tet4")
    ap.add_argument(
        "--operator",
        choices=["diffusion", "mass", "convection", "diffusion_mass", "diffusion_convection_mass", "laplace", "test"],
        default="diffusion_mass",
    )
    ap.add_argument("--n-qp", type=int, default=4)
    args = ap.parse_args()

    sizes = _parse_sizes(args.sizes)
    backends = ["cpu", "cuda", "metal", "hip", "opencl"] if args.backend == "all" else [args.backend]
    for b in backends:
        try:
            _run_backend(
                backend=b,
                device_index=args.device_index,
                sizes=sizes,
                n_qp=args.n_qp,
                element_type=args.element_type,
                operator=args.operator,
                runs=args.runs,
                dtype=args.dtype,
            )
        except Exception as e:
            print(f"[WARN] backend={b} skipped: {e}")


if __name__ == "__main__":
    main()
