#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def _run(rel: str, args: list[str]) -> int:
    path = ROOT / rel
    if not path.exists():
        print(f"[WARN] missing: {path}")
        return 1
    cmd = [sys.executable, str(path)] + args
    print(f"\n=== RUN: {rel} ===")
    return subprocess.run(cmd, check=False).returncode


def main() -> None:
    ap = argparse.ArgumentParser(description="Run all real-world kernels (separate module).")
    ap.add_argument("--backend", choices=["cpu", "cuda", "metal", "all"], default="all")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--gemm-shapes", type=str, default="512x512x512,1024x1024x1024")
    ap.add_argument("--reduction-sizes", type=str, default="1000000,5000000,10000000,50000000")
    ap.add_argument("--stencil-shapes", type=str, default="1024x1024,2048x2048")
    ap.add_argument("--stencil-iters", type=int, default=50)
    ap.add_argument("--stencil3d-shapes", type=str, default="64x128x128,128x128x128")
    ap.add_argument("--stencil3d-iters", type=int, default=25)
    ap.add_argument("--spmv-sizes", type=str, default="2000,5000,10000")
    ap.add_argument("--spmv-nnz-per-row", type=int, default=16)
    ap.add_argument("--fem-sizes", type=str, default="100000,500000,1000000")
    ap.add_argument("--fem-n-qp", type=int, default=8)
    ap.add_argument("--fem-integration-sizes", type=str, default="100000,500000,1000000")
    ap.add_argument("--fem-integration-n-qp", type=int, default=4)
    ap.add_argument("--fem-integration-element-type", choices=["tet4", "hex8"], default="tet4")
    ap.add_argument(
        "--fem-integration-operator",
        choices=["diffusion", "mass", "convection", "diffusion_mass", "diffusion_convection_mass"],
        default="diffusion_mass",
    )
    ap.add_argument("--with-fem-integration", action="store_true")
    ap.add_argument("--skip-gemm", action="store_true")
    ap.add_argument("--skip-reduction", action="store_true")
    ap.add_argument("--skip-stencil", action="store_true")
    ap.add_argument("--skip-stencil3d", action="store_true")
    ap.add_argument("--skip-spmv", action="store_true")
    ap.add_argument("--skip-fem", action="store_true")
    args = ap.parse_args()

    base_args = [
        "--backend",
        args.backend,
        "--device-index",
        str(args.device_index),
        "--runs",
        str(args.runs),
    ]

    ok = True
    if not args.skip_gemm:
        ok = ok and (
            _run("real_kernels/benchmarks/run_gemm.py", base_args + ["--shapes", args.gemm_shapes]) == 0
        )
    if not args.skip_reduction:
        ok = ok and (
            _run("real_kernels/benchmarks/run_reduction.py", base_args + ["--sizes", args.reduction_sizes]) == 0
        )
    if not args.skip_stencil:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_stencil2d.py",
                base_args + ["--shapes", args.stencil_shapes, "--iters", str(args.stencil_iters)],
            )
            == 0
        )
    if not args.skip_spmv:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_spmv.py",
                base_args
                + ["--sizes", args.spmv_sizes, "--nnz-per-row", str(args.spmv_nnz_per_row)],
            )
            == 0
        )
    if not args.skip_stencil3d:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_stencil3d.py",
                base_args + ["--shapes", args.stencil3d_shapes, "--iters", str(args.stencil3d_iters)],
            )
            == 0
        )
    if not args.skip_fem:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_fem.py",
                base_args + ["--sizes", args.fem_sizes, "--n-qp", str(args.fem_n_qp)],
            )
            == 0
        )
    if args.with_fem_integration:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_fem_integration.py",
                base_args
                + [
                    "--sizes",
                    args.fem_integration_sizes,
                    "--element-type",
                    args.fem_integration_element_type,
                    "--operator",
                    args.fem_integration_operator,
                    "--n-qp",
                    str(args.fem_integration_n_qp),
                ],
            )
            == 0
        )

    if not ok:
        sys.exit(1)

    print("\n[OK] real_kernels finished.")


if __name__ == "__main__":
    main()
