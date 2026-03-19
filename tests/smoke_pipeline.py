#!/usr/bin/env python3
from __future__ import annotations

import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def run(cmd: list[str]) -> int:
    print(f"[SMOKE] {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=ROOT, check=False).returncode


def main() -> None:
    checks = [
        [sys.executable, "run_all_benchmarks.py", "--help"],
        [sys.executable, "run_all_backends.py", "--help"],
        [sys.executable, "run_all_gpu_benchmarks.py", "--help"],
        [sys.executable, "analysis/roofline_model.py", "--help"],
        [sys.executable, "analysis/report.py", "--help"],
        [sys.executable, "run_firefly_optimization.py", "--help"],
        [sys.executable, "real_kernels/run_all_real_kernels.py", "--help"],
    ]

    ok = True
    for c in checks:
        ok = ok and (run(c) == 0)

    # szybki real-kernels CPU smoke
    ok = ok and (
        run(
            [
                sys.executable,
                "real_kernels/run_all_real_kernels.py",
                "--backend",
                "cpu",
                "--runs",
                "1",
                "--gemm-shapes",
                "64x64x64",
                "--reduction-sizes",
                "1000000",
                "--stencil-shapes",
                "256x256",
                "--stencil-iters",
                "2",
                "--spmv-sizes",
                "1000",
                "--spmv-nnz-per-row",
                "8",
                "--stencil3d-shapes",
                "16x32x32",
                "--stencil3d-iters",
                "2",
                "--fem-sizes",
                "10000",
                "--fem-n-qp",
                "4",
                "--with-fem-integration",
                "--fem-integration-sizes",
                "2000",
                "--fem-integration-element-type",
                "tet4",
                "--fem-integration-operator",
                "diffusion_mass",
                "--fem-integration-n-qp",
                "1",
            ]
        )
        == 0
    )

    if not ok:
        print("[SMOKE] FAILED")
        sys.exit(1)
    print("[SMOKE] OK")


if __name__ == "__main__":
    main()
