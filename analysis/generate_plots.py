#!/usr/bin/env python3
from __future__ import annotations

import csv
import os
import re
from collections import defaultdict
from pathlib import Path
from statistics import mean


ROOT = Path(__file__).resolve().parents[1]
CPU_DIR = ROOT / "data" / "cpu"
GPU_DIR = ROOT / "data" / "gpu"
REAL_DIR = ROOT / "data" / "real_kernels"
PLOTS_DIR = ROOT / "analysis" / "plots"


def _try_import_matplotlib():
    # Avoid failures when ~/.matplotlib is not writable.
    mpl_cfg = ROOT / ".cache" / "matplotlib"
    mpl_cfg.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(mpl_cfg))
    try:
        import matplotlib.pyplot as plt  # type: ignore
        return plt
    except Exception as e:
        print(f"[WARN] matplotlib unavailable: {e}")
        return None


def _to_float(v: object) -> float | None:
    try:
        if v is None:
            return None
        s = str(v).strip()
        if s == "":
            return None
        return float(s)
    except Exception:
        return None


def _to_int(v: object) -> int | None:
    x = _to_float(v)
    if x is None:
        return None
    try:
        return int(x)
    except Exception:
        return None


def _read_rows(paths: list[Path]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for p in paths:
        try:
            with p.open("r", encoding="utf-8", newline="") as f:
                r = csv.DictReader(f)
                rows.extend(r)
        except Exception:
            continue
    return rows


def _latest_session_dir() -> Path | None:
    runs_root = ROOT / "data" / "runs"
    latest_link = runs_root / "latest"
    if latest_link.exists():
        try:
            p = latest_link.resolve()
            if p.exists():
                return p
        except Exception:
            pass
    latest_txt = runs_root / "latest.txt"
    if latest_txt.exists():
        try:
            name = latest_txt.read_text(encoding="utf-8").strip()
            if name:
                p = runs_root / name
                if p.exists():
                    return p
        except Exception:
            pass
    return None


def _canonical_stem(stem: str) -> str:
    s = re.sub(r"__normalized_\d{8}_\d{6}$", "", stem)
    s = re.sub(r"__user-[^_]+__ts-\d{8}_\d{6}$", "", s)
    return s


def _pick_latest_unique(paths: list[Path]) -> list[Path]:
    latest: dict[str, Path] = {}
    for p in paths:
        key = _canonical_stem(p.stem)
        prev = latest.get(key)
        if prev is None or p.stat().st_mtime > prev.stat().st_mtime:
            latest[key] = p
    return sorted(latest.values())


def _data_dirs() -> tuple[Path, Path, Path]:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        p = Path(run_root)
        return p / "cpu", p / "gpu", p / "real_kernels"
    latest = _latest_session_dir()
    if latest is not None:
        cpu = latest / "cpu"
        gpu = latest / "gpu"
        real = latest / "real_kernels"
        if any(cpu.glob("*.csv")) or any(gpu.glob("*.csv")) or any(real.glob("*.csv")):
            return cpu, gpu, real
    return CPU_DIR, GPU_DIR, REAL_DIR


def plot_gpu_bandwidth(plt, gpu_dir: Path) -> None:
    rows = _read_rows(_pick_latest_unique(sorted(gpu_dir.glob("*bandwidth*.csv"))))
    grouped: dict[tuple[str, int], list[float]] = defaultdict(list)
    for r in rows:
        backend = str(r.get("backend", "unknown"))
        # Handle mixed/legacy schemas:
        # - preferred: size_bytes
        # - fallback : size_mb
        # Some historical rows are malformed (shifted columns), so we skip
        # entries where size cannot be parsed.
        size = _to_int(r.get("size_bytes"))
        if size is None or size <= 0:
            size_mb = _to_int(r.get("size_mb"))
            size = (size_mb * 1024 * 1024) if (size_mb is not None and size_mb > 0) else 0
        gbps = _to_float(r.get("throughput_gbps") or r.get("gbps"))
        if size > 0 and gbps is not None:
            grouped[(backend, size)].append(gbps)

    if not grouped:
        print("[INFO] plot_gpu_bandwidth: no data")
        return

    by_backend: dict[str, list[tuple[float, float]]] = defaultdict(list)
    for (backend, size), vals in grouped.items():
        by_backend[backend].append((size / (1024.0 * 1024.0), mean(vals)))

    plt.figure(figsize=(8, 5))
    for backend, points in sorted(by_backend.items()):
        points = sorted(points, key=lambda x: x[0])
        x = [p[0] for p in points]
        y = [p[1] for p in points]
        plt.plot(x, y, marker="o", label=backend)
    plt.xscale("log", base=2)
    plt.xlabel("Buffer size [MB]")
    plt.ylabel("Bandwidth [GB/s]")
    plt.title("GPU Bandwidth vs Size")
    plt.legend()
    plt.grid(True, alpha=0.3)
    out = PLOTS_DIR / "gpu_bandwidth_vs_size.png"
    plt.tight_layout()
    plt.savefig(out, dpi=150)
    plt.close()
    print(f"[OK] {out}")


def plot_gpu_latency(plt, gpu_dir: Path) -> None:
    rows = _read_rows(_pick_latest_unique(sorted(gpu_dir.glob("*pointer_latency*.csv"))))
    grouped: dict[tuple[str, int], list[float]] = defaultdict(list)
    for r in rows:
        backend = str(r.get("backend", "unknown"))
        size = _to_int(r.get("size_bytes")) or 0
        lat = _to_float(r.get("latency_ns"))
        if size > 0 and lat is not None:
            grouped[(backend, size)].append(lat)
    if not grouped:
        print("[INFO] plot_gpu_latency: no data")
        return
    by_backend: dict[str, list[tuple[float, float]]] = defaultdict(list)
    for (backend, size), vals in grouped.items():
        by_backend[backend].append((size / 1024.0, mean(vals)))
    plt.figure(figsize=(8, 5))
    for backend, points in sorted(by_backend.items()):
        points = sorted(points, key=lambda x: x[0])
        x = [p[0] for p in points]
        y = [p[1] for p in points]
        plt.plot(x, y, marker="o", label=backend)
    plt.xscale("log", base=2)
    plt.xlabel("Working-set size [KB]")
    plt.ylabel("Latency [ns/access]")
    plt.title("GPU Pointer-Chase Latency")
    plt.legend()
    plt.grid(True, alpha=0.3)
    out = PLOTS_DIR / "gpu_pointer_latency.png"
    plt.tight_layout()
    plt.savefig(out, dpi=150)
    plt.close()
    print(f"[OK] {out}")


def plot_cpu_stream(plt, cpu_dir: Path) -> None:
    rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*stream*.csv"))))
    grouped: dict[tuple[str, int], list[float]] = defaultdict(list)
    for r in rows:
        kernel = str(r.get("kernel", "unknown"))
        if kernel != "triad":
            continue
        size_mb = _to_int(r.get("size_mb")) or 0
        gbps = _to_float(r.get("gbps") or r.get("throughput_gbps"))
        if size_mb > 0 and gbps is not None:
            grouped[(kernel, size_mb)].append(gbps)

    if not grouped:
        print("[INFO] plot_cpu_stream: no data")
        return

    points = sorted(((size, mean(vals)) for (_k, size), vals in grouped.items()), key=lambda x: x[0])
    x = [p[0] for p in points]
    y = [p[1] for p in points]

    plt.figure(figsize=(8, 5))
    plt.plot(x, y, marker="o")
    plt.xscale("log", base=2)
    plt.xlabel("Array size [MB]")
    plt.ylabel("STREAM triad [GB/s]")
    plt.title("CPU STREAM Triad vs Size")
    plt.grid(True, alpha=0.3)
    out = PLOTS_DIR / "cpu_stream_triad_vs_size.png"
    plt.tight_layout()
    plt.savefig(out, dpi=150)
    plt.close()
    print(f"[OK] {out}")


def plot_real_kernels(plt, real_dir: Path) -> None:
    rows = _read_rows(_pick_latest_unique(sorted(real_dir.glob("*.csv"))))
    if not rows:
        print("[INFO] plot_real_kernels: no data")
        return

    compute_series: dict[str, dict[str, list[float]]] = defaultdict(lambda: defaultdict(list))
    memory_series: dict[str, dict[str, list[float]]] = defaultdict(lambda: defaultdict(list))
    for r in rows:
        if str(r.get("status", "ok")) != "ok":
            continue
        b = str(r.get("backend", "unknown"))
        k = str(r.get("kernel", ""))
        if k == "gemm":
            g = _to_float(r.get("gflops"))
            if g is not None:
                compute_series["gemm"][b].append(g)
        elif k == "spmv":
            g = _to_float(r.get("gflops"))
            if g is not None:
                compute_series["spmv"][b].append(g)
        elif k == "fem":
            g = _to_float(r.get("gflops"))
            if g is not None:
                compute_series["fem"][b].append(g)
        elif k == "fem_integration":
            g = _to_float(r.get("gflops"))
            if g is not None:
                compute_series["fem_integration"][b].append(g)
        elif k == "reduction":
            v = _to_float(r.get("throughput_gbps"))
            if v is not None:
                memory_series["reduction"][b].append(v)
        elif k == "stencil2d":
            v = _to_float(r.get("throughput_gbps"))
            if v is not None:
                memory_series["stencil2d"][b].append(v)
        elif k == "stencil3d":
            v = _to_float(r.get("throughput_gbps"))
            if v is not None:
                memory_series["stencil3d"][b].append(v)

    backends = sorted(
        {
            backend
            for series in list(compute_series.values()) + list(memory_series.values())
            for backend in series.keys()
        }
    )
    if not backends:
        print("[INFO] plot_real_kernels: no valid rows")
        return

    import numpy as np  # type: ignore

    compute_kernels = [k for k in ("gemm", "spmv", "fem", "fem_integration") if k in compute_series]
    memory_kernels = [k for k in ("reduction", "stencil2d", "stencil3d") if k in memory_series]
    nrows = 2 if compute_kernels and memory_kernels else 1
    fig, axes = plt.subplots(nrows, 1, figsize=(9, 4 + 2.8 * nrows))
    if not isinstance(axes, np.ndarray):
        axes = np.array([axes])

    subplot_idx = 0
    x = np.arange(len(backends))

    if compute_kernels:
        ax = axes[subplot_idx]
        subplot_idx += 1
        width = 0.8 / max(1, len(compute_kernels))
        for idx, kernel in enumerate(compute_kernels):
            vals = [
                mean(compute_series[kernel][backend]) if compute_series[kernel].get(backend) else 0.0
                for backend in backends
            ]
            offs = x + (idx - (len(compute_kernels) - 1) / 2.0) * width
            ax.bar(offs, vals, width=width, label=f"{kernel} [GFLOP/s]")
        ax.set_xticks(x, backends)
        ax.set_title("Real Kernels Compute")
        ax.grid(True, axis="y", alpha=0.3)
        ax.legend()

    if memory_kernels:
        ax = axes[subplot_idx]
        width = 0.8 / max(1, len(memory_kernels))
        for idx, kernel in enumerate(memory_kernels):
            vals = [
                mean(memory_series[kernel][backend]) if memory_series[kernel].get(backend) else 0.0
                for backend in backends
            ]
            offs = x + (idx - (len(memory_kernels) - 1) / 2.0) * width
            ax.bar(offs, vals, width=width, label=f"{kernel} [GB/s]")
        ax.set_xticks(x, backends)
        ax.set_title("Real Kernels Memory")
        ax.grid(True, axis="y", alpha=0.3)
        ax.legend()

    out = PLOTS_DIR / "real_kernels_overview.png"
    fig.tight_layout()
    fig.savefig(out, dpi=150)
    plt.close(fig)
    print(f"[OK] {out}")


def main() -> None:
    PLOTS_DIR.mkdir(parents=True, exist_ok=True)
    plt = _try_import_matplotlib()
    if plt is None:
        return

    cpu_dir, gpu_dir, real_dir = _data_dirs()
    print(f"[INFO] Plot source CPU:  {cpu_dir}")
    print(f"[INFO] Plot source GPU:  {gpu_dir}")
    print(f"[INFO] Plot source REAL: {real_dir}")
    plot_gpu_bandwidth(plt, gpu_dir=gpu_dir)
    plot_gpu_latency(plt, gpu_dir=gpu_dir)
    plot_cpu_stream(plt, cpu_dir=cpu_dir)
    plot_real_kernels(plt, real_dir=real_dir)


if __name__ == "__main__":
    main()
