#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import subprocess
import sys
from datetime import datetime
from pathlib import Path
from collections import Counter


ROOT = Path(__file__).resolve().parents[1]
REPORTS = ROOT / "reports"
GPU_DIR = ROOT / "data" / "gpu"


def _run_capture(rel: str, args: list[str]) -> tuple[int, str]:
    path = ROOT / rel
    if not path.exists():
        return 1, f"[missing] {path}\n"
    cmd = [sys.executable, str(path)] + args
    p = subprocess.run(cmd, cwd=ROOT, check=False, capture_output=True, text=True)
    out = (p.stdout or "") + ("\n" + p.stderr if p.stderr else "")
    return p.returncode, out


def _parse_gpu_filename(fp: Path) -> tuple[str, str, str, str]:
    stem = fp.stem
    parts = stem.split("__")
    benchmark = parts[0].replace("gpu_", "", 1) if parts else "unknown"
    backend = "unknown"
    gpu = "unknown"
    dev = "unknown"
    for part in parts[1:]:
        if part.startswith("backend-"):
            backend = part.split("backend-", 1)[1] or backend
        elif part.startswith("gpu-"):
            gpu = part.split("gpu-", 1)[1] or gpu
        elif part.startswith("dev"):
            dev = part
    return benchmark, backend, gpu, dev


def _select_gpu_files(mode: str) -> list[Path]:
    files = sorted(GPU_DIR.glob("*.csv"))
    if mode == "all":
        return files
    latest: dict[tuple[str, str, str, str], Path] = {}
    mtime: dict[tuple[str, str, str, str], float] = {}
    for fp in files:
        key = _parse_gpu_filename(fp)
        t = fp.stat().st_mtime
        if key not in mtime or t > mtime[key]:
            mtime[key] = t
            latest[key] = fp
    return sorted(latest.values())


def _gpu_data_quality(mode: str) -> tuple[int, int, int, Counter]:
    files = _select_gpu_files(mode)
    total_rows = 0
    bad_shape_rows = 0
    parse_dropped = 0
    energy_sources: Counter = Counter()

    for fp in files:
        with fp.open("r", encoding="utf-8", newline="") as f:
            raw_rows = list(csv.reader(f))
        if not raw_rows:
            continue
        header_len = len(raw_rows[0])
        for row in raw_rows[1:]:
            total_rows += 1
            if len(row) != header_len:
                bad_shape_rows += 1

        with fp.open("r", encoding="utf-8", newline="") as f:
            dr = csv.DictReader(f)
            for row in dr:
                src = (row.get("energy_source") or "unknown").strip() or "unknown"
                energy_sources[src] += 1
                size = row.get("size_bytes", "").strip()
                gbps = (row.get("throughput_gbps") or row.get("gbps") or "").strip()
                gflops = (row.get("gflops") or row.get("throughput_gflops") or "").strip()
                if size and gbps:
                    continue
                if gflops:
                    continue
                parse_dropped += 1
    return total_rows, bad_shape_rows, parse_dropped, energy_sources


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate markdown report from benchmark data.")
    ap.add_argument("--mode", choices=["latest", "all"], default="latest")
    ap.add_argument("--roofline-target", choices=["cpu", "gpu", "both"], default="gpu")
    ap.add_argument("--roofline-backend", default="cuda")
    ap.add_argument("--roofline-ai", type=float, default=8.0)
    ap.add_argument("--roofline-bytes", type=float, default=1e9)
    ap.add_argument("--with-plots", action="store_true")
    args = ap.parse_args()

    REPORTS.mkdir(parents=True, exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = REPORTS / f"report_{ts}.md"

    sections: list[str] = []
    sections.append(f"# Benchmark Report ({ts})\n")
    sections.append(f"- mode: `{args.mode}`")
    sections.append(f"- roofline target: `{args.roofline_target}`")
    sections.append("")

    rc, txt = _run_capture("analysis/cpu_summary.py", ["--mode", args.mode])
    sections.append("## CPU Summary")
    sections.append(f"- exit_code: `{rc}`")
    sections.append("```text\n" + txt.strip() + "\n```")

    rc, txt = _run_capture("analysis/gpu_summary.py", ["--mode", args.mode])
    sections.append("## GPU Summary")
    sections.append(f"- exit_code: `{rc}`")
    sections.append("```text\n" + txt.strip() + "\n```")

    rc_strict, txt_strict = _run_capture("analysis/gpu_summary.py", ["--mode", args.mode, "--strict"])
    q_rows, q_bad, q_drop, q_sources = _gpu_data_quality(args.mode)
    sections.append("## Data Quality")
    sections.append(f"- gpu_summary_strict_exit_code: `{rc_strict}`")
    sections.append(f"- rows_total: `{q_rows}`")
    sections.append(f"- rows_bad_shape: `{q_bad}`")
    sections.append(f"- rows_dropped_parse: `{q_drop}`")
    sections.append("- energy_sources:")
    if q_sources:
        for k, v in sorted(q_sources.items(), key=lambda kv: (kv[0], kv[1])):
            sections.append(f"  - `{k}`: `{v}` rows")
    else:
        sections.append("  - `none`")
    sections.append("```text\n" + txt_strict.strip() + "\n```")

    rc, txt = _run_capture("analysis/real_kernels_summary.py", [])
    sections.append("## Real Kernels Summary")
    sections.append(f"- exit_code: `{rc}`")
    sections.append("```text\n" + txt.strip() + "\n```")

    rc, txt = _run_capture("analysis/data_quality.py", ["--strict"])
    sections.append("## Data Quality (Global)")
    sections.append(f"- exit_code: `{rc}`")
    sections.append("```text\n" + txt.strip() + "\n```")

    roof_args = [
        "--target",
        args.roofline_target,
        "--ai",
        str(args.roofline_ai),
        "--bytes",
        str(args.roofline_bytes),
    ]
    if args.roofline_target in ("gpu", "both"):
        roof_args += ["--backend", args.roofline_backend]
    rc, txt = _run_capture("analysis/roofline_model.py", roof_args)
    sections.append("## Roofline")
    sections.append(f"- exit_code: `{rc}`")
    sections.append("```text\n" + txt.strip() + "\n```")

    if args.with_plots:
        rc, txt = _run_capture("analysis/generate_plots.py", [])
        sections.append("## Plots")
        sections.append(f"- exit_code: `{rc}`")
        sections.append("```text\n" + txt.strip() + "\n```")
        sections.append("Generated thesis-core images (if matplotlib available):")
        sections.append("- `analysis/figures/thesis_core/cpu_memcpy_bandwidth_scaling.png`")
        sections.append("- `analysis/figures/thesis_core/cpu_stream_triad_scaling.png`")
        sections.append("- `analysis/figures/thesis_core/cpu_peak_compute_scaling.png`")
        sections.append("- `analysis/figures/thesis_core/cpu_memory_latency_hierarchy.png`")
        sections.append("- `analysis/figures/thesis_core/gpu_microbenchmark_suite.png`")
        sections.append("- `analysis/figures/thesis_core/platform_roofline_measured.png`")
        sections.append("- `analysis/figures/thesis_core/real_kernels_model_validation.png`")
        sections.append("- `analysis/figures/thesis_core/real_kernels_filip_contrast_map.png`")

    out.write_text("\n".join(sections) + "\n", encoding="utf-8")
    print(f"[OK] report: {out}")


if __name__ == "__main__":
    main()
