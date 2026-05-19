#!/usr/bin/env python3
from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]

STANDARD_DEFAULTS = {
    "assembly_sizes": "10000,30000,60000",
    "assembly_n_qp_choices": "2,4,6",
    "assembly_n_dofs_choices": "4,6,8",
    "assembly_variants": "qss,sqs,ssq",
    "assembly_workspace_choices": "0,1",
    "assembly_scatter_choices": "0,1",
    "assembly_padding_choices": "0,1",
    "saxpy_sizes": "1000000,5000000,10000000,50000000",
    "gemm_shapes": "512x512x512,1024x1024x1024",
    "reduction_sizes": "1000000,5000000,10000000,50000000",
    "stencil_shapes": "1024x1024,2048x2048",
    "stencil_iters": 50,
    "stencil3d_shapes": "64x128x128,128x128x128",
    "stencil3d_iters": 25,
    "spmv_sizes": "2000,5000,10000",
    "spmv_nnz_per_row": 16,
    "fem_sizes": "100000,500000,1000000",
    "fem_n_qp": 8,
    "fem_integration_sizes": "100000,500000,1000000",
    "fem_integration_n_qp": 4,
}

EXTENDED_DEFAULTS = {
    "assembly_sizes": "5000,10000,20000,50000,100000,200000",
    "assembly_n_qp_choices": "1,2,4,6,8",
    "assembly_n_dofs_choices": "4,6,8,12",
    "assembly_variants": "qss,sqs,ssq",
    "assembly_workspace_choices": "0,1",
    "assembly_scatter_choices": "0,1",
    "assembly_padding_choices": "0,1",
    "saxpy_sizes": "250000,500000,1000000,5000000,10000000,50000000",
    "gemm_shapes": "256x256x256,512x512x512,768x768x768,1024x1024x1024,1536x1536x1536",
    "reduction_sizes": "250000,500000,1000000,5000000,10000000,50000000,100000000",
    "stencil_shapes": "512x512,1024x1024,1536x1536,2048x2048,3072x3072",
    "stencil_iters": 80,
    "stencil3d_shapes": "64x96x96,64x128x128,96x128x128,128x128x128",
    "stencil3d_iters": 40,
    "spmv_sizes": "1000,2000,5000,10000,20000",
    "spmv_nnz_per_row": 16,
    "fem_sizes": "50000,100000,250000,500000,1000000,2000000",
    "fem_n_qp": 8,
    "fem_integration_sizes": "50000,100000,250000,500000,1000000,2000000",
    "fem_integration_n_qp": 6,
}


def _run(rel: str, args: list[str]) -> int:
    path = ROOT / rel
    if not path.exists():
        print(f"[WARN] missing: {path}")
        return 1
    cmd = [sys.executable, str(path)] + args
    print(f"\n=== RUN: {rel} ===")
    return subprocess.run(cmd, check=False).returncode


def _defaults_for_mode(mode: str) -> dict[str, object]:
    return EXTENDED_DEFAULTS if str(mode).strip().lower() == "extended" else STANDARD_DEFAULTS


def main() -> None:
    ap = argparse.ArgumentParser(description="Run all real-world kernels (Final).")
    ap.add_argument("--backend", choices=["cpu", "cuda", "metal", "all"], default="all")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--warmups", type=int, default=0)
    ap.add_argument("--benchmark-mode", choices=["standard", "extended"], default="standard")
    ap.add_argument("--assembly-sizes", type=str, default="")
    ap.add_argument("--assembly-n-qp-choices", type=str, default="")
    ap.add_argument("--assembly-n-dofs-choices", type=str, default="")
    ap.add_argument("--assembly-variants", type=str, default="")
    ap.add_argument("--assembly-workspace-choices", type=str, default="")
    ap.add_argument("--assembly-scatter-choices", type=str, default="")
    ap.add_argument("--assembly-padding-choices", type=str, default="")
    ap.add_argument("--saxpy-sizes", type=str, default="")
    ap.add_argument("--gemm-shapes", type=str, default="")
    ap.add_argument("--reduction-sizes", type=str, default="")
    ap.add_argument("--stencil-shapes", type=str, default="")
    ap.add_argument("--stencil-iters", type=int, default=0)
    ap.add_argument("--stencil3d-shapes", type=str, default="")
    ap.add_argument("--stencil3d-iters", type=int, default=0)
    ap.add_argument("--spmv-sizes", type=str, default="")
    ap.add_argument("--spmv-nnz-per-row", type=int, default=0)
    ap.add_argument("--fem-sizes", type=str, default="")
    ap.add_argument("--fem-n-qp", type=int, default=0)
    ap.add_argument("--fem-integration-sizes", type=str, default="")
    ap.add_argument("--fem-integration-n-qp", type=int, default=0)
    ap.add_argument("--fem-integration-element-type", choices=["tet4", "hex8", "prism6"], default="tet4")
    ap.add_argument(
        "--fem-integration-operator",
        choices=["diffusion", "mass", "convection", "diffusion_mass", "diffusion_convection_mass", "laplace", "test"],
        default="diffusion_mass",
    )
    ap.add_argument("--with-fem-integration", action="store_true")
    ap.add_argument("--skip-saxpy", action="store_true")
    ap.add_argument("--skip-assembly-like", action="store_true")
    ap.add_argument("--skip-gemm", action="store_true")
    ap.add_argument("--skip-reduction", action="store_true")
    ap.add_argument("--skip-stencil", action="store_true")
    ap.add_argument("--skip-stencil3d", action="store_true")
    ap.add_argument("--skip-spmv", action="store_true")
    ap.add_argument("--skip-fem", action="store_true")
    args = ap.parse_args()

    defaults = _defaults_for_mode(args.benchmark_mode)
    assembly_sizes = args.assembly_sizes or str(defaults["assembly_sizes"])
    assembly_n_qp_choices = args.assembly_n_qp_choices or str(defaults["assembly_n_qp_choices"])
    assembly_n_dofs_choices = args.assembly_n_dofs_choices or str(defaults["assembly_n_dofs_choices"])
    assembly_variants = args.assembly_variants or str(defaults["assembly_variants"])
    assembly_workspace_choices = args.assembly_workspace_choices or str(defaults["assembly_workspace_choices"])
    assembly_scatter_choices = args.assembly_scatter_choices or str(defaults["assembly_scatter_choices"])
    assembly_padding_choices = args.assembly_padding_choices or str(defaults["assembly_padding_choices"])
    saxpy_sizes = args.saxpy_sizes or str(defaults["saxpy_sizes"])
    gemm_shapes = args.gemm_shapes or str(defaults["gemm_shapes"])
    reduction_sizes = args.reduction_sizes or str(defaults["reduction_sizes"])
    stencil_shapes = args.stencil_shapes or str(defaults["stencil_shapes"])
    stencil_iters = int(args.stencil_iters or int(defaults["stencil_iters"]))
    stencil3d_shapes = args.stencil3d_shapes or str(defaults["stencil3d_shapes"])
    stencil3d_iters = int(args.stencil3d_iters or int(defaults["stencil3d_iters"]))
    spmv_sizes = args.spmv_sizes or str(defaults["spmv_sizes"])
    spmv_nnz_per_row = int(args.spmv_nnz_per_row or int(defaults["spmv_nnz_per_row"]))
    fem_sizes = args.fem_sizes or str(defaults["fem_sizes"])
    fem_n_qp = int(args.fem_n_qp or int(defaults["fem_n_qp"]))
    fem_integration_sizes = args.fem_integration_sizes or str(defaults["fem_integration_sizes"])
    fem_integration_n_qp = int(args.fem_integration_n_qp or int(defaults["fem_integration_n_qp"]))

    base_args = [
        "--backend",
        args.backend,
        "--device-index",
        str(args.device_index),
        "--runs",
        str(args.runs),
        "--warmups",
        str(max(int(args.warmups), 0)),
    ]

    ok = True
    if not args.skip_assembly_like:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_assembly_like.py",
                base_args
                + [
                    "--sizes",
                    assembly_sizes,
                    "--n-qp-choices",
                    assembly_n_qp_choices,
                    "--n-dofs-choices",
                    assembly_n_dofs_choices,
                    "--variants",
                    assembly_variants,
                    "--workspace-choices",
                    assembly_workspace_choices,
                    "--scatter-choices",
                    assembly_scatter_choices,
                    "--padding-choices",
                    assembly_padding_choices,
                ],
            )
            == 0
        )
    if not args.skip_saxpy:
        ok = ok and (
            _run("real_kernels/benchmarks/run_saxpy.py", base_args + ["--sizes", saxpy_sizes]) == 0
        )
    if not args.skip_gemm:
        ok = ok and (
            _run("real_kernels/benchmarks/run_gemm.py", base_args + ["--shapes", gemm_shapes]) == 0
        )
    if not args.skip_reduction:
        ok = ok and (
            _run("real_kernels/benchmarks/run_reduction.py", base_args + ["--sizes", reduction_sizes]) == 0
        )
    if not args.skip_stencil:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_stencil2d.py",
                base_args + ["--shapes", stencil_shapes, "--iters", str(stencil_iters)],
            )
            == 0
        )
    if not args.skip_spmv:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_spmv.py",
                base_args + ["--sizes", spmv_sizes, "--nnz-per-row", str(spmv_nnz_per_row)],
            )
            == 0
        )
    if not args.skip_stencil3d:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_stencil3d.py",
                base_args + ["--shapes", stencil3d_shapes, "--iters", str(stencil3d_iters)],
            )
            == 0
        )
    if not args.skip_fem:
        ok = ok and (
            _run(
                "real_kernels/benchmarks/run_fem.py",
                base_args + ["--sizes", fem_sizes, "--n-qp", str(fem_n_qp)],
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
                    fem_integration_sizes,
                    "--element-type",
                    args.fem_integration_element_type,
                    "--operator",
                    args.fem_integration_operator,
                    "--n-qp",
                    str(fem_integration_n_qp),
                ],
            )
            == 0
        )

    if not ok:
        sys.exit(1)

    print("\n[OK] real_kernels finished.")


if __name__ == "__main__":
    main()
