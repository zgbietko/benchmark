#!/usr/bin/env python3
from __future__ import annotations

import argparse
from pathlib import Path
import sys
from typing import Dict, List

ROOT = Path(__file__).resolve().parents[2]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from real_kernels.common import append_rows, base_meta, make_csv_path, run_warmups
from real_kernels.cpu_backend import CpuRealBackend
from real_kernels.cuda_backend import CudaRealBackend
from real_kernels.metal_backend import MetalRealBackend
from real_kernels.hip_backend import HipRealBackend
from real_kernels.opencl_backend import OpenCLRealBackend


def _parse_int_csv(raw: str) -> List[int]:
    return [int(x.strip()) for x in raw.split(",") if x.strip()]


def _parse_str_csv(raw: str) -> List[str]:
    return [str(x).strip() for x in raw.split(",") if str(x).strip()]


def _run_backend(
    backend: str,
    device_index: int,
    sizes: List[int],
    n_qp_choices: List[int],
    n_dofs_choices: List[int],
    variants: List[str],
    workspace_choices: List[int],
    scatter_choices: List[int],
    padding_choices: List[int],
    runs: int,
    warmups: int,
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

    csv_path = make_csv_path("assembly_like", backend, device_name, device_index)
    rows: List[Dict[str, object]] = []

    print(f"=== AUTHOR ASSEMBLY-LIKE ({backend}) ===")
    print(f"device: {device_name} (index {device_index})")
    print(f"runs: {runs}, warmups: {warmups}, dtype: {dtype}")

    for n_elem in sizes:
        for n_qp in n_qp_choices:
            for n_dofs in n_dofs_choices:
                for variant in variants:
                    for use_workspace in workspace_choices:
                        for scatter in scatter_choices:
                            for padding in padding_choices:
                                print(
                                    "\n--- "
                                    f"n_elem={n_elem}, n_qp={n_qp}, n_dofs={n_dofs}, "
                                    f"variant={variant}, ws={use_workspace}, scatter={scatter}, pad={padding} ---"
                                )

                                run_warmups(
                                    warmups,
                                    lambda: be.assembly_like(
                                        n_elements=n_elem,
                                        n_qp=n_qp,
                                        n_dofs=n_dofs,
                                        variant=variant,
                                        use_workspace=use_workspace,
                                        scatter_accumulate=scatter,
                                        padding=padding,
                                        dtype=dtype,
                                    ),
                                )

                                for run_idx in range(runs):
                                    status = "ok"
                                    err = ""
                                    elapsed = float("nan")
                                    gflops = float("nan")
                                    gbps = float("nan")
                                    ai = float("nan")
                                    try:
                                        elapsed, gflops, gbps, ai = be.assembly_like(
                                            n_elements=n_elem,
                                            n_qp=n_qp,
                                            n_dofs=n_dofs,
                                            variant=variant,
                                            use_workspace=use_workspace,
                                            scatter_accumulate=scatter,
                                            padding=padding,
                                            dtype=dtype,
                                        )
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
                                            "kernel": "assembly_like",
                                            "workload_name": "author_fem_assembly",
                                            "dtype": dtype,
                                            "n_elements": n_elem,
                                            "n_qp": n_qp,
                                            "n_dofs": n_dofs,
                                            "variant": variant,
                                            "use_workspace": int(bool(use_workspace)),
                                            "scatter_accumulate": int(bool(scatter)),
                                            "padding": int(bool(padding)),
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
    ap = argparse.ArgumentParser(description="Authorial parametric FEM-like assembly workload benchmark.")
    ap.add_argument(
        "--backend",
        choices=["cpu", "cuda", "metal", "hip", "opencl", "amd", "intel", "all"],
        default="all",
    )
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--runs", type=int, default=3)
    ap.add_argument("--warmups", type=int, default=0)
    ap.add_argument("--dtype", choices=["float32", "float64"], default="float32")
    ap.add_argument("--sizes", type=str, default="10000,30000,60000")
    ap.add_argument("--n-qp-choices", type=str, default="2,4,6")
    ap.add_argument("--n-dofs-choices", type=str, default="4,6,8")
    ap.add_argument("--variants", type=str, default="qss,sqs,ssq")
    ap.add_argument("--workspace-choices", type=str, default="0,1")
    ap.add_argument("--scatter-choices", type=str, default="0,1")
    ap.add_argument("--padding-choices", type=str, default="0,1")
    args = ap.parse_args()

    sizes = _parse_int_csv(args.sizes)
    n_qp_choices = _parse_int_csv(args.n_qp_choices)
    n_dofs_choices = _parse_int_csv(args.n_dofs_choices)
    variants = [v.lower() for v in _parse_str_csv(args.variants)]
    workspace_choices = _parse_int_csv(args.workspace_choices)
    scatter_choices = _parse_int_csv(args.scatter_choices)
    padding_choices = _parse_int_csv(args.padding_choices)

    backends = ["cpu", "cuda", "metal", "hip", "opencl"] if args.backend == "all" else [args.backend]
    for backend in backends:
        try:
            _run_backend(
                backend=backend,
                device_index=args.device_index,
                sizes=sizes,
                n_qp_choices=n_qp_choices,
                n_dofs_choices=n_dofs_choices,
                variants=variants,
                workspace_choices=workspace_choices,
                scatter_choices=scatter_choices,
                padding_choices=padding_choices,
                runs=args.runs,
                warmups=args.warmups,
                dtype=args.dtype,
            )
        except Exception as e:
            print(f"[WARN] backend={backend} skipped: {e}")


if __name__ == "__main__":
    main()
