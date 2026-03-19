#!/usr/bin/env python3
from __future__ import annotations

import csv
from collections import defaultdict
import os
from pathlib import Path
from statistics import mean, pstdev


ROOT = Path(__file__).resolve().parents[1]
DATA_DIR = ROOT / "data" / "real_kernels"
RUNS_DIR = ROOT / "data" / "runs"


def _to_float(x: object) -> float | None:
    try:
        if x is None:
            return None
        s = str(x).strip()
        if s == "":
            return None
        return float(s)
    except Exception:
        return None


def _latest_session_dir() -> Path | None:
    latest = RUNS_DIR / "latest"
    if latest.exists():
        try:
            p = latest.resolve()
            if p.exists():
                return p
        except Exception:
            pass
    latest_txt = RUNS_DIR / "latest.txt"
    if latest_txt.exists():
        try:
            name = latest_txt.read_text(encoding="utf-8").strip()
        except Exception:
            name = ""
        if name:
            p = RUNS_DIR / name
            if p.exists():
                return p
    return None


def _data_dir(scope: str, session: str) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        p = Path(run_root)
        if (p / "real_kernels").exists():
            return p / "real_kernels"
    if scope == "session":
        if session and session != "latest":
            return RUNS_DIR / session / "real_kernels"
        latest = _latest_session_dir()
        if latest is not None:
            return latest / "real_kernels"
    return DATA_DIR


def main() -> None:
    import argparse

    ap = argparse.ArgumentParser(description="Summarize real-kernel benchmark CSV files.")
    ap.add_argument("--scope", choices=["auto", "global", "session"], default="auto")
    ap.add_argument("--session", default="latest")
    args = ap.parse_args()

    scope = args.scope
    if scope == "auto":
        scope = "session" if os.environ.get("BENCH_RUN_DIR", "").strip() else "global"

    data_dir = _data_dir(scope, args.session)
    print(f"[INFO] Real kernels summary source: {data_dir}")

    if not data_dir.exists():
        print(f"[INFO] no dir: {data_dir}")
        return

    files = sorted(data_dir.glob("*.csv"))
    if not files:
        print(f"[INFO] no files in: {data_dir}")
        return

    gemm = defaultdict(list)
    red = defaultdict(list)
    stencil = defaultdict(list)
    spmv = defaultdict(list)
    stencil3d = defaultdict(list)
    fem = defaultdict(list)
    fem_int = defaultdict(list)

    for p in files:
        with p.open("r", encoding="utf-8", newline="") as f:
            r = csv.DictReader(f)
            for row in r:
                status = str(row.get("status", "ok"))
                if status != "ok":
                    continue
                backend = str(row.get("backend", "unknown"))
                device = str(row.get("device_name", "unknown"))
                kernel = str(row.get("kernel", ""))

                if kernel == "gemm":
                    key = (backend, device, int(float(row.get("m", 0))), int(float(row.get("n", 0))), int(float(row.get("k", 0))))
                    g = _to_float(row.get("gflops"))
                    if g is not None:
                        gemm[key].append(g)
                elif kernel == "reduction":
                    key = (backend, device, int(float(row.get("n", 0))))
                    b = _to_float(row.get("throughput_gbps"))
                    if b is not None:
                        red[key].append(b)
                elif kernel == "stencil2d":
                    key = (
                        backend,
                        device,
                        int(float(row.get("h", 0))),
                        int(float(row.get("w", 0))),
                        int(float(row.get("iters_inner", 0))),
                    )
                    b = _to_float(row.get("throughput_gbps"))
                    if b is not None:
                        stencil[key].append(b)
                elif kernel == "spmv":
                    key = (
                        backend,
                        device,
                        int(float(row.get("n", 0))),
                        int(float(row.get("nnz_per_row", 0))),
                    )
                    g = _to_float(row.get("gflops"))
                    if g is not None:
                        spmv[key].append(g)
                elif kernel == "stencil3d":
                    key = (
                        backend,
                        device,
                        int(float(row.get("d", 0))),
                        int(float(row.get("h", 0))),
                        int(float(row.get("w", 0))),
                        int(float(row.get("iters_inner", 0))),
                    )
                    b = _to_float(row.get("throughput_gbps"))
                    if b is not None:
                        stencil3d[key].append(b)
                elif kernel == "fem":
                    key = (
                        backend,
                        device,
                        int(float(row.get("n_elements", 0))),
                        int(float(row.get("n_qp", 0))),
                    )
                    g = _to_float(row.get("gflops"))
                    if g is not None:
                        fem[key].append(g)
                elif kernel == "fem_integration":
                    key = (
                        backend,
                        device,
                        str(row.get("element_type", "tet4")),
                        str(row.get("operator", "diffusion_mass")),
                        int(float(row.get("n_elements", 0))),
                        int(float(row.get("n_qp", 0))),
                    )
                    g = _to_float(row.get("gflops"))
                    b = _to_float(row.get("throughput_gbps"))
                    ai = _to_float(row.get("ai_flop_per_byte"))
                    if g is not None and b is not None:
                        fem_int[key].append((g, b, ai if ai is not None else float("nan")))

    print("=== REAL KERNELS SUMMARY ===")
    if gemm:
        print("\n[GEMM] GFLOP/s mean ± sigma")
        for k in sorted(gemm.keys()):
            vals = gemm[k]
            mu = mean(vals)
            sd = pstdev(vals) if len(vals) > 1 else 0.0
            backend, device, m, n, kk = k
            print(f"- {backend:6s} | {device:25s} | {m:5d}x{n:5d}x{kk:5d} : {mu:9.2f} ± {sd:7.2f}")
    else:
        print("\n[GEMM] no data")

    if red:
        print("\n[Reduction] GB/s mean ± sigma")
        for k in sorted(red.keys()):
            vals = red[k]
            mu = mean(vals)
            sd = pstdev(vals) if len(vals) > 1 else 0.0
            backend, device, n = k
            print(f"- {backend:6s} | {device:25s} | n={n:10d} : {mu:9.2f} ± {sd:7.2f}")
    else:
        print("\n[Reduction] no data")

    if stencil:
        print("\n[Stencil2D] GB/s mean ± sigma")
        for k in sorted(stencil.keys()):
            vals = stencil[k]
            mu = mean(vals)
            sd = pstdev(vals) if len(vals) > 1 else 0.0
            backend, device, h, w, iters = k
            print(f"- {backend:6s} | {device:25s} | {h:5d}x{w:5d} iters={iters:4d} : {mu:9.2f} ± {sd:7.2f}")
    else:
        print("\n[Stencil2D] no data")

    if spmv:
        print("\n[SpMV] GFLOP/s mean ± sigma")
        for k in sorted(spmv.keys()):
            vals = spmv[k]
            mu = mean(vals)
            sd = pstdev(vals) if len(vals) > 1 else 0.0
            backend, device, n, nnz = k
            print(f"- {backend:6s} | {device:25s} | n={n:8d} nnz/row={nnz:3d} : {mu:9.2f} ± {sd:7.2f}")
    else:
        print("\n[SpMV] no data")

    if stencil3d:
        print("\n[Stencil3D] GB/s mean ± sigma")
        for k in sorted(stencil3d.keys()):
            vals = stencil3d[k]
            mu = mean(vals)
            sd = pstdev(vals) if len(vals) > 1 else 0.0
            backend, device, d, h, w, iters = k
            print(f"- {backend:6s} | {device:25s} | {d:4d}x{h:4d}x{w:4d} iters={iters:4d} : {mu:9.2f} ± {sd:7.2f}")
    else:
        print("\n[Stencil3D] no data")

    if fem:
        print("\n[FEM] GFLOP/s mean ± sigma")
        for k in sorted(fem.keys()):
            vals = fem[k]
            mu = mean(vals)
            sd = pstdev(vals) if len(vals) > 1 else 0.0
            backend, device, n_elem, n_qp = k
            print(f"- {backend:6s} | {device:25s} | n_elem={n_elem:8d} n_qp={n_qp:3d} : {mu:9.2f} ± {sd:7.2f}")
    else:
        print("\n[FEM] no data")

    if fem_int:
        print("\n[FEM Integration] mean ± sigma")
        for k in sorted(fem_int.keys()):
            vals = fem_int[k]
            gvals = [v[0] for v in vals]
            bvals = [v[1] for v in vals]
            avals = [v[2] for v in vals if v[2] == v[2]]
            gmu = mean(gvals)
            gsd = pstdev(gvals) if len(gvals) > 1 else 0.0
            bmu = mean(bvals)
            bsd = pstdev(bvals) if len(bvals) > 1 else 0.0
            amu = mean(avals) if avals else float("nan")
            backend, device, etype, op, n_elem, n_qp = k
            print(
                f"- {backend:6s} | {device:25s} | {etype:4s}/{op:14s} n_elem={n_elem:8d} n_qp={n_qp:3d} : "
                f"GF={gmu:8.2f}±{gsd:6.2f}, GB/s={bmu:8.2f}±{bsd:6.2f}, AI={amu:6.3f}"
            )
    else:
        print("\n[FEM Integration] no data")


if __name__ == "__main__":
    main()
