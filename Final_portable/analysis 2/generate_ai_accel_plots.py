#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import os
from collections import defaultdict
from pathlib import Path
from statistics import median
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in __import__("sys").path:
    __import__("sys").path.insert(0, str(ROOT))

from analysis.publication_style import (
    THESIS_CORE_DIR,
    apply_axis_style,
    backend_color,
    ensure_figure_dirs,
    save_figure,
    setup_publication_theme,
)

RUNS_DIR = ROOT / "data" / "runs"
GLOBAL_DIR = ROOT / "data" / "ai_accel"


def _to_float(v: object) -> float | None:
    try:
        if v is None:
            return None
        s = str(v).strip()
        if not s or s.lower() == "nan":
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


def _source_dir(scope: str, session: str) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        p = Path(run_root) / "ai_accel"
        if p.exists():
            return p
    if scope == "session":
        if session and session != "latest":
            return RUNS_DIR / session / "ai_accel"
        latest = _latest_session_dir()
        if latest is not None:
            return latest / "ai_accel"
    return GLOBAL_DIR


def _read_rows(src: Path) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for path in sorted(src.glob("*.csv")):
        with path.open("r", encoding="utf-8", newline="") as handle:
            rows.extend(csv.DictReader(handle))
    return rows


def _platform_label(rows: list[dict[str, str]]) -> str:
    models = sorted({str(r.get("device_name", "")).strip() for r in rows if str(r.get("device_name", "")).strip()})
    systems = sorted({str(r.get("system", "")).strip() for r in rows if str(r.get("system", "")).strip()})
    arches = sorted({str(r.get("arch", "")).strip() for r in rows if str(r.get("arch", "")).strip()})
    parts: list[str] = []
    if systems:
        parts.append("/".join(systems))
    if arches:
        parts.append("/".join(arches))
    if models:
        parts.append("; ".join(models[:3]))
    return " | ".join(parts)


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate AI acceleration overview plot.")
    ap.add_argument("--scope", choices=["global", "session"], default="session")
    ap.add_argument("--session", default="latest")
    args = ap.parse_args()

    src = _source_dir(args.scope, args.session)
    print(f"[INFO] AI plot source: {src}")
    if not src.exists():
        print("[WARN] source does not exist")
        return

    rows = [r for r in _read_rows(src) if str(r.get("status", "ok")).strip().lower() == "ok"]
    if not rows:
        print("[WARN] no successful rows")
        return

    try:
        import matplotlib.pyplot as plt  # type: ignore
    except Exception as exc:
        print(f"[WARN] matplotlib unavailable: {exc}")
        return

    ensure_figure_dirs()
    setup_publication_theme(plt)

    matmul = defaultdict(list)
    coreml = defaultdict(list)
    matmul_elapsed = defaultdict(list)
    matmul_ops = {}

    for row in rows:
        kernel = str(row.get("kernel", "")).strip().lower()
        backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
        g = _to_float(row.get("gflops"))
        e = _to_float(row.get("elapsed_s"))
        ops = _to_float(row.get("ops_count"))
        if g is None:
            continue
        if kernel == "coreml_mlp_predict":
            units = str(row.get("compute_units", "")).strip().lower() or "all"
            coreml[(backend, units)].append(g)
        else:
            precision = str(row.get("precision", "")).strip().lower() or "unknown"
            matmul[(backend, precision)].append(g)
            m = str(row.get("m", "")).strip()
            n = str(row.get("n", "")).strip()
            k = str(row.get("k", "")).strip()
            shape_key = f"{m}x{n}x{k}"
            if e is not None:
                matmul_elapsed[(backend, shape_key)].append(e)
            if ops is not None:
                matmul_ops[(backend, shape_key)] = float(ops)

    fig, axes = plt.subplots(1, 2, figsize=(13.2, 5.0))
    ax0, ax1 = axes

    if matmul:
        keys = sorted(matmul.keys())
        vals = [median(matmul[key]) for key in keys]
        labels = [f"{k[0]}\n{k[1]}" for k in keys]
        colors = [backend_color(k[0]) for k in keys]
        ax0.bar(range(len(keys)), vals, color=colors, alpha=0.9)
        ax0.set_xticks(range(len(keys)), labels, rotation=18, ha="right")
        ax0.set_ylabel("GFLOP/s (mediana)")
        ax0.set_title("AI matmul: backend x precyzja")
        apply_axis_style(ax0, grid_axis="y")
    else:
        ax0.text(0.5, 0.5, "Brak danych matmul", transform=ax0.transAxes, ha="center", va="center")
        ax0.set_axis_off()

    if coreml:
        keys = sorted(coreml.keys())
        vals = [median(coreml[key]) for key in keys]
        labels = [f"{k[0]}\n{k[1]}" for k in keys]
        colors = [backend_color("metal") for _ in keys]
        ax1.bar(range(len(keys)), vals, color=colors, alpha=0.9)
        ax1.set_xticks(range(len(keys)), labels, rotation=18, ha="right")
        ax1.set_ylabel("GFLOP/s (mediana)")
        ax1.set_title("Apple Core ML: jednostki obliczeniowe")
        apply_axis_style(ax1, grid_axis="y")
    else:
        ax1.text(0.5, 0.5, "Brak probe Core ML/NE", transform=ax1.transAxes, ha="center", va="center")
        ax1.set_axis_off()

    fig.suptitle("Przeglad akceleracji AI", fontsize=14)
    out = THESIS_CORE_DIR / "ai_accel_overview.png"
    save_figure(fig, out, dpi=220, platform_label=_platform_label(rows))
    plt.close(fig)
    print(f"[OK] Plot: {out}")

    if matmul_elapsed:
        fig2, ax = plt.subplots(1, 1, figsize=(8.8, 4.8))
        backends = sorted({key[0] for key in matmul_elapsed.keys()})
        for backend in backends:
            points: list[tuple[float, float]] = []
            for (b, shape_key), vals in matmul_elapsed.items():
                if b != backend:
                    continue
                ops = matmul_ops.get((b, shape_key))
                if ops is None:
                    continue
                points.append((float(ops), float(median(vals))))
            points.sort(key=lambda x: x[0])
            if not points:
                continue
            xs = [p[0] for p in points]
            ys = [p[1] for p in points]
            ax.plot(xs, ys, marker="o", linewidth=1.9, label=backend, color=backend_color(backend))
        ax.set_xscale("log")
        ax.set_xlabel("Liczba FLOP (log)")
        ax.set_ylabel("Czas [s] (mediana)")
        ax.set_title("AI matmul break-even: CPU vs akceleratory")
        apply_axis_style(ax, grid_axis="both")
        ax.legend(frameon=True, loc="best")
        out2 = THESIS_CORE_DIR / "ai_accel_break_even.png"
        save_figure(fig2, out2, dpi=220, platform_label=_platform_label(rows))
        plt.close(fig2)
        print(f"[OK] Plot: {out2}")

    if matmul:
        fig3, ax = plt.subplots(1, 1, figsize=(9.4, 4.8))
        prec_order = ["float64", "float32", "float16", "int8"]
        available_precs = [p for p in prec_order if any(k[1] == p for k in matmul.keys())]
        if not available_precs:
            available_precs = sorted({k[1] for k in matmul.keys()})
        backends = sorted({k[0] for k in matmul.keys()})
        for backend in backends:
            ys: list[float] = []
            for prec in available_precs:
                vals = matmul.get((backend, prec), [])
                ys.append(float(median(vals)) if vals else float("nan"))
            ax.plot(range(len(available_precs)), ys, marker="o", linewidth=1.9, label=backend, color=backend_color(backend))
        ax.set_xticks(range(len(available_precs)), available_precs)
        ax.set_ylabel("GFLOP/s (mediana)")
        ax.set_xlabel("Precyzja")
        ax.set_title("AI matmul: skalowanie po precyzjach")
        apply_axis_style(ax, grid_axis="y")
        ax.legend(frameon=True, loc="best")
        out3 = THESIS_CORE_DIR / "ai_precision_scaling.png"
        save_figure(fig3, out3, dpi=220, platform_label=_platform_label(rows))
        plt.close(fig3)
        print(f"[OK] Plot: {out3}")


if __name__ == "__main__":
    main()
