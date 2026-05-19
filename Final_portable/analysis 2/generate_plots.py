#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
import math
import os
import re
import sys
from collections import defaultdict
from pathlib import Path
from statistics import mean, median
from typing import Any

import numpy as np  # type: ignore

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.cache_latency import add_cache_boundary_lines, aggregated_latency_points
from analysis.publication_style import (
    THESIS_CORE_DIR,
    MANIFEST_DIR,
    ensure_figure_dirs,
    clear_pngs,
    setup_publication_theme,
    apply_axis_style,
    save_figure,
    figure_entry,
    backend_color,
    algorithm_color,
    padded_ylim,
)
from analysis.roofline_model import _cpu_peaks, _gpu_peaks

CPU_DIR = ROOT / "data" / "cpu"
GPU_DIR = ROOT / "data" / "gpu"
REAL_DIR = ROOT / "data" / "real_kernels"
RUNS_DIR = ROOT / "data" / "runs"
THESIS_FULL_DIR = ROOT / "data" / "thesis_full"

FIGURE_SET_ID = "thesis-core-v1"


def _try_import_matplotlib():
    mpl_cfg = ROOT / ".cache" / "matplotlib"
    mpl_cfg.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(mpl_cfg))
    try:
        import matplotlib.pyplot as plt  # type: ignore

        return plt
    except Exception as exc:
        print(f"[WARN] matplotlib unavailable: {exc}")
        return None


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _read_rows(paths: list[Path]) -> list[dict[str, str]]:
    rows: list[dict[str, str]] = []
    for path in paths:
        try:
            with path.open("r", encoding="utf-8", newline="") as handle:
                rows.extend(csv.DictReader(handle))
        except Exception:
            continue
    return rows


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


def _to_int(v: object) -> int | None:
    val = _to_float(v)
    if val is None:
        return None
    try:
        return int(val)
    except Exception:
        return None


def _latest_session_dir() -> Path | None:
    latest = RUNS_DIR / "latest"
    if latest.exists():
        try:
            target = latest.resolve()
            if target.exists():
                return target
        except Exception:
            pass
    latest_txt = RUNS_DIR / "latest.txt"
    if latest_txt.exists():
        try:
            name = latest_txt.read_text(encoding="utf-8").strip()
        except Exception:
            name = ""
        if name:
            candidate = RUNS_DIR / name
            if candidate.exists():
                return candidate
    return None


def _latest_nonempty_session_subdir(kind: str, pattern: str = "*.csv") -> Path | None:
    if not RUNS_DIR.exists():
        return None
    candidates = sorted((p for p in RUNS_DIR.iterdir() if p.is_dir()), key=lambda p: p.stat().st_mtime, reverse=True)
    for session_dir in candidates:
        sub = session_dir / kind
        if sub.exists() and any(sub.glob(pattern)):
            return sub
    return None


def _latest_thesis_full_dir() -> Path | None:
    if not THESIS_FULL_DIR.exists():
        return None
    dirs = sorted((p for p in THESIS_FULL_DIR.iterdir() if p.is_dir()), key=lambda p: p.stat().st_mtime, reverse=True)
    for path in dirs:
        summary_path = path / "summary.json"
        if not summary_path.exists():
            continue
        try:
            summary = _read_json(summary_path)
        except Exception:
            continue
        if bool(summary.get("running", False)):
            continue
        if str(summary.get("workflow", "")).strip() == "full_thesis_pipeline" or str(summary.get("experiment_class", "")).strip() == "thesis_full_campaign":
            return path
    return dirs[0] if dirs else None


def _existing_csv_dir(path: Path | None) -> Path | None:
    if path is None:
        return None
    if path.exists() and any(path.glob("*.csv")):
        return path
    return None


def _full_campaign_sources() -> tuple[Path | None, Path | None, list[Path], str] | None:
    campaign_dir = _latest_thesis_full_dir()
    if campaign_dir is None:
        return None
    summary_path = campaign_dir / "summary.json"
    if not summary_path.exists():
        return None
    try:
        summary = _read_json(summary_path)
    except Exception:
        return None
    steps = summary.get("steps") or []
    if not isinstance(steps, list):
        return None
    by_id = {str(step.get("id", "")): step for step in steps if isinstance(step, dict)}

    def step_subdir(step_id: str, name: str) -> Path | None:
        step = by_id.get(step_id) or {}
        payload = step.get("payload") if isinstance(step.get("payload"), dict) else {}
        result_dir = Path(str(payload.get("session_dir") or step.get("result_dir") or "")).expanduser()
        if not str(result_dir):
            return None
        return result_dir / name

    cpu_dir = _existing_csv_dir(step_subdir("cpu_benchmark", "cpu")) or _existing_csv_dir(step_subdir("cpu_real_kernels", "cpu"))
    gpu_dir = _existing_csv_dir(step_subdir("gpu_benchmark", "gpu")) or _existing_csv_dir(step_subdir("gpu_real_kernels", "gpu"))
    real_dirs: list[Path] = []
    for step_id in ("cpu_real_kernels", "gpu_real_kernels"):
        candidate = _existing_csv_dir(step_subdir(step_id, "real_kernels"))
        if candidate is not None and str(candidate) not in {str(x) for x in real_dirs}:
            real_dirs.append(candidate)
    if cpu_dir is None and gpu_dir is None and not real_dirs:
        return None
    return cpu_dir, gpu_dir, real_dirs, f"thesis_full:{campaign_dir.name}"


def _data_dirs() -> tuple[Path, Path, list[Path], str]:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        session_root = Path(run_root)
        cpu = _existing_csv_dir(session_root / "cpu") or _latest_nonempty_session_subdir("cpu") or CPU_DIR
        gpu = _existing_csv_dir(session_root / "gpu") or _latest_nonempty_session_subdir("gpu") or GPU_DIR
        real = _existing_csv_dir(session_root / "real_kernels") or _latest_nonempty_session_subdir("real_kernels") or REAL_DIR
        return cpu, gpu, [real], f"session:{session_root.name}"

    full_sources = _full_campaign_sources()
    if full_sources is not None:
        cpu, gpu, real_dirs, label = full_sources
        return (
            cpu or _latest_nonempty_session_subdir("cpu") or CPU_DIR,
            gpu or _latest_nonempty_session_subdir("gpu") or GPU_DIR,
            real_dirs or [_latest_nonempty_session_subdir("real_kernels") or REAL_DIR],
            label,
        )

    latest = _latest_session_dir()
    if latest is not None:
        return (
            _existing_csv_dir(latest / "cpu") or _latest_nonempty_session_subdir("cpu") or CPU_DIR,
            _existing_csv_dir(latest / "gpu") or _latest_nonempty_session_subdir("gpu") or GPU_DIR,
            [_existing_csv_dir(latest / "real_kernels") or _latest_nonempty_session_subdir("real_kernels") or REAL_DIR],
            f"fallback:{latest.name}",
        )
    return CPU_DIR, GPU_DIR, [REAL_DIR], "fallback:global"


def _canonical_stem(stem: str) -> str:
    s = re.sub(r"__normalized_\d{8}_\d{6}$", "", stem)
    s = re.sub(r"__user-[^_]+__ts-\d{8}_\d{6}$", "", s)
    return s


def _gpu_transfer_label(label: str) -> str:
    mapping = {
        "device_to_device": "urządzenie -> urządzenie",
        "host_to_device": "host -> urządzenie",
        "device_to_host": "urządzenie -> host",
        "pinned_host_to_device": "host przypięty -> urządzenie",
        "device_to_pinned_host": "urządzenie -> host przypięty",
    }
    key = str(label).strip().lower()
    return mapping.get(key, str(label).replace("_", " "))


def _gpu_compute_label(label: str) -> str:
    mapping = {
        "compute": "obliczenia",
        "fma_peak": "szczyt FMA",
        "peak_compute": "szczyt obliczeniowy",
    }
    key = str(label).strip().lower()
    return mapping.get(key, str(label))


def _pick_latest_unique(paths: list[Path]) -> list[Path]:
    latest: dict[str, Path] = {}
    for path in paths:
        key = _canonical_stem(path.stem)
        prev = latest.get(key)
        if prev is None or path.stat().st_mtime > prev.stat().st_mtime:
            latest[key] = path
    return sorted(latest.values())


def _unique_nonempty(rows: list[dict[str, str]], *fields: str) -> list[str]:
    seen: set[str] = set()
    values: list[str] = []
    for row in rows:
        for field in fields:
            val = str(row.get(field, "")).strip()
            if not val or val in seen:
                continue
            seen.add(val)
            values.append(val)
            break
    return values


def _system_arch_tag(rows: list[dict[str, str]]) -> str:
    systems = _unique_nonempty(rows, "system", "os")
    archs = _unique_nonempty(rows, "arch", "machine")
    parts: list[str] = []
    if systems:
        parts.append("system: " + " / ".join(systems[:2]))
    if archs:
        parts.append("architektura: " + " / ".join(archs[:2]))
    return " | ".join(part for part in parts if part).strip()


def _cpu_platform_label(rows: list[dict[str, str]]) -> str:
    cpu_models = _unique_nonempty(rows, "cpu_model")
    fallback_models = _unique_nonempty(rows, "processor", "machine")
    system_arch = _system_arch_tag(rows)
    parts: list[str] = []
    chosen_cpu = cpu_models[0] if cpu_models else (fallback_models[0] if fallback_models else "")
    if chosen_cpu:
        parts.append(f"CPU: {chosen_cpu}")
    if system_arch:
        parts.append(system_arch)
    return "Platforma testowa: " + " | ".join(parts) if parts else ""


def _gpu_platform_label(rows: list[dict[str, str]]) -> str:
    gpu_models = _unique_nonempty(rows, "gpu_model", "device_name")
    backends = _unique_nonempty(rows, "backend")
    system_arch = _system_arch_tag(rows)
    parts: list[str] = []
    if gpu_models:
        parts.append(f"GPU: {' / '.join(gpu_models[:2])}")
    if backends:
        parts.append(f"backend: {' / '.join(backends[:2])}")
    if system_arch:
        parts.append(system_arch)
    return "Platforma testowa: " + " | ".join(parts) if parts else ""


def _real_platform_label(rows: list[dict[str, str]]) -> str:
    system_arch = _system_arch_tag(rows)
    backends = _unique_nonempty(rows, "backend")
    devices = _unique_nonempty(rows, "device_name")
    parts: list[str] = []
    if system_arch:
        parts.append(system_arch)
    if backends:
        parts.append("backendy: " + ", ".join(backends[:4]))
    if devices:
        parts.append("urządzenia: " + ", ".join(devices[:4]))
    return "Platforma testowa: " + " | ".join(parts) if parts else ""


def _roofline_platform_label(
    cpu_model: str | None,
    gpu_backend: str | None,
    gpu_model: str | None,
    system_arch: str | None = None,
) -> str:
    parts: list[str] = []
    if cpu_model:
        parts.append(f"CPU: {cpu_model}")
    if gpu_model:
        if gpu_backend:
            parts.append(f"GPU: {gpu_model} | backend: {gpu_backend}")
        else:
            parts.append(f"GPU: {gpu_model}")
    if system_arch:
        parts.append(system_arch)
    return "Platforma testowa: " + " | ".join(parts) if parts else ""


def _median_group(rows: list[dict[str, str]], *, key_fields: tuple[str, ...], value_field: str) -> dict[tuple[Any, ...], float]:
    grouped: dict[tuple[Any, ...], list[float]] = defaultdict(list)
    for row in rows:
        value = _to_float(row.get(value_field))
        if value is None:
            continue
        key: list[Any] = []
        skip = False
        for field in key_fields:
            if field == "size_mb":
                raw = row.get("size_mb")
                val = _to_int(raw)
            elif field == "size_bytes":
                val = _to_int(row.get("size_bytes"))
            elif field == "threads":
                val = _to_int(row.get("threads") or row.get("num_threads"))
            else:
                val = row.get(field)
            if val is None or val == "":
                skip = True
                break
            key.append(val)
        if skip:
            continue
        grouped[tuple(key)].append(value)
    return {key: float(median(vals)) for key, vals in grouped.items() if vals}


def _best_by_size_from_threaded(rows: list[dict[str, str]], *, value_field: str, extra_match: dict[str, str] | None = None) -> dict[int, float]:
    filtered = []
    for row in rows:
        if extra_match and any(str(row.get(k, "")).strip() != str(v) for k, v in extra_match.items()):
            continue
        filtered.append(row)
    med = _median_group(filtered, key_fields=("size_mb", "threads"), value_field=value_field)
    best: dict[int, float] = {}
    for (size_mb, _threads), value in med.items():
        size_mb = int(size_mb)
        best[size_mb] = max(best.get(size_mb, float("-inf")), value)
    return best


def _best_by_thread(rows: list[dict[str, str]], *, value_field: str, size_field: str = "size_mb", extra_match: dict[str, str] | None = None) -> dict[int, float]:
    filtered = []
    for row in rows:
        if extra_match and any(str(row.get(k, "")).strip() != str(v) for k, v in extra_match.items()):
            continue
        filtered.append(row)
    med = _median_group(filtered, key_fields=(size_field, "threads"), value_field=value_field)
    best: dict[int, float] = {}
    for (_size, threads), value in med.items():
        threads = int(threads)
        best[threads] = max(best.get(threads, float("-inf")), value)
    return best


def _efficiency_curve(best_by_thread: dict[int, float]) -> dict[int, float]:
    base = best_by_thread.get(1)
    if base is None or base <= 0:
        return {}
    out: dict[int, float] = {}
    for threads, value in sorted(best_by_thread.items()):
        out[threads] = value / (base * max(1, threads))
    return out


def _plot_size_curve(ax: Any, series: list[tuple[str, dict[int, float], str, str]], *, x_label: str, y_label: str, title: str) -> None:
    for label, mapping, color, linestyle in series:
        if not mapping:
            continue
        xs = sorted(mapping.keys())
        ys = [mapping[x] for x in xs]
        ax.plot(xs, ys, marker="o", linewidth=1.9, markersize=4.5, label=label, color=color, linestyle=linestyle)
    ax.set_xscale("log", base=2)
    ax.set_xlabel(x_label)
    ax.set_ylabel(y_label)
    ax.set_title(title)
    apply_axis_style(ax, grid_axis="both")
    if series:
        ax.legend(frameon=False)


def _plot_scaling_with_efficiency(ax: Any, best_by_thread: dict[int, float], *, y_label: str, title: str, color: str) -> None:
    xs = sorted(best_by_thread.keys())
    ys = [best_by_thread[x] for x in xs]
    eff = _efficiency_curve(best_by_thread)
    eff_y = [eff.get(x, float("nan")) * 100.0 for x in xs]
    ax.plot(xs, ys, marker="o", linewidth=1.95, color=color, label="throughput")
    ax.set_xlabel("Liczba wątków")
    ax.set_ylabel(y_label, color=color)
    ax.tick_params(axis="y", colors=color)
    ax.set_title(title)
    apply_axis_style(ax, grid_axis="both")
    ax2 = ax.twinx()
    ax2.plot(xs, eff_y, marker="s", linewidth=1.55, color="#111111", linestyle="--", label="efficiency")
    ax2.set_ylabel("Sprawność skalowania [% 1T/thread]", color="#111111")
    ax2.tick_params(axis="y", colors="#111111")
    handles = []
    labels = []
    for axis in (ax, ax2):
        h, l = axis.get_legend_handles_labels()
        handles.extend(h)
        labels.extend(l)
    ax.legend(handles, labels, loc="best", frameon=False)


def _plot_cpu_memcpy_suite(plt: Any, cpu_dir: Path) -> tuple[Path | None, dict[str, Any]]:
    rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*bandwidth*.csv"))))
    if not rows:
        return None, {}
    st_rows = [row for row in rows if "mt" not in str(row.get("benchmark", "")).lower()]
    mt_rows = [row for row in rows if "mt" in str(row.get("benchmark", "")).lower()]

    st_median = _median_group(st_rows, key_fields=("size_bytes",), value_field="gbps")
    st_size = {int(size_bytes) // (1024 * 1024): val for (size_bytes,), val in st_median.items()}
    mt_best = _best_by_size_from_threaded(mt_rows, value_field="gbps", extra_match=None)
    scaling = _best_by_thread(mt_rows, value_field="gbps", size_field="size_bytes")

    fig, axes = plt.subplots(1, 2, figsize=(12.6, 4.8))
    _plot_size_curve(
        axes[0],
        [
            ("1T median", st_size, backend_color("cpu"), "-"),
            ("MT best-achieved", mt_best, "#111111", "--"),
        ],
        x_label="Rozmiar working setu [MB]",
        y_label="Przepustowość [GB/s]",
        title="CPU memcpy: przepustowość vs rozmiar",
    )
    _plot_scaling_with_efficiency(
        axes[1],
        scaling,
        y_label="Najlepsza przepustowość [GB/s]",
        title="CPU memcpy: skalowanie względem liczby wątków",
        color=backend_color("cpu"),
    )
    fig.suptitle("CPU memcpy: główna figura publikacyjna", fontsize=14)
    out = THESIS_CORE_DIR / "cpu_memcpy_bandwidth_scaling.png"
    save_figure(fig, out, dpi=220, platform_label=_cpu_platform_label(rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="cpu_memcpy_bandwidth_scaling",
        section="benchmarks",
        path=out,
        question="Jak CPU skaluje przepustowość kopiowania pamięci względem rozmiaru i liczby wątków?",
        summary="Łączy przebieg 1T/MT po rozmiarze z wykresem skalowania i sprawności.",
    )
    meta.update({"aggregation": "median per condition, MT envelope = best median over tested thread counts"})
    return out, meta


def _plot_cpu_stream_suite(plt: Any, cpu_dir: Path) -> tuple[Path | None, dict[str, Any]]:
    rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*stream*.csv"))))
    triad_st = [row for row in rows if str(row.get("benchmark", "")) == "stream" and str(row.get("kernel", "")).lower() == "triad"]
    triad_mt = [row for row in rows if str(row.get("benchmark", "")) == "stream_mt" and str(row.get("kernel", "")).lower() == "triad"]
    if not triad_st and not triad_mt:
        return None, {}

    st_median = _median_group(triad_st, key_fields=("size_mb",), value_field="gbps")
    st_size = {int(size_mb): val for (size_mb,), val in st_median.items()}
    mt_best = _best_by_size_from_threaded(triad_mt, value_field="gbps", extra_match={"kernel": "triad"})
    scaling = _best_by_thread(triad_mt, value_field="gbps", extra_match={"kernel": "triad"})

    fig, axes = plt.subplots(1, 2, figsize=(12.6, 4.8))
    _plot_size_curve(
        axes[0],
        [
            ("TRIAD 1T median", st_size, algorithm_color("stream"), "-"),
            ("TRIAD MT best-achieved", mt_best, "#111111", "--"),
        ],
        x_label="Rozmiar tablic [MB]",
        y_label="Przepustowość [GB/s]",
        title="CPU STREAM TRIAD: przebieg względem rozmiaru",
    )
    _plot_scaling_with_efficiency(
        axes[1],
        scaling,
        y_label="Najlepsza przepustowość [GB/s]",
        title="CPU STREAM TRIAD: skalowanie względem liczby wątków",
        color=algorithm_color("stream"),
    )
    fig.suptitle("CPU STREAM: reprezentatywny kernel TRIAD", fontsize=14)
    out = THESIS_CORE_DIR / "cpu_stream_triad_scaling.png"
    save_figure(fig, out, dpi=220, platform_label=_cpu_platform_label(rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="cpu_stream_triad_scaling",
        section="benchmarks",
        path=out,
        question="Jak zachowuje się reprezentatywny kernel STREAM po rozmiarze i liczbie wątków?",
        summary="Ogranicza rodzinę STREAM do TRIAD jako najczytelniejszego przypadku reprezentatywnego.",
    )
    meta.update({"aggregation": "median per condition, MT envelope = best median over tested thread counts"})
    return out, meta


def _plot_cpu_compute_scaling(plt: Any, cpu_dir: Path) -> tuple[Path | None, dict[str, Any]]:
    rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*compute*.csv"))))
    peak_rows = [row for row in rows if str(row.get("benchmark", "")).strip() == "fma_peak_mt"]
    if not peak_rows:
        return None, {}
    scaling = _best_by_thread(peak_rows, value_field="gflops", size_field="n_per_thread")
    if not scaling:
        return None, {}

    xs = sorted(scaling.keys())
    ys = [scaling[x] for x in xs]
    eff = _efficiency_curve(scaling)

    fig, axes = plt.subplots(1, 2, figsize=(12.6, 4.8))
    ax0, ax1 = axes
    ax0.plot(xs, ys, marker="o", linewidth=2.0, color=algorithm_color("compute"))
    ax0.set_xlabel("Liczba wątków")
    ax0.set_ylabel("GFLOP/s")
    ax0.set_title("CPU FMA: przepustowość względem liczby wątków")
    apply_axis_style(ax0, grid_axis="both")

    ax1.plot(xs, [eff[x] * 100.0 for x in xs], marker="s", linewidth=1.9, color="#111111")
    ax1.set_xlabel("Liczba wątków")
    ax1.set_ylabel("Sprawność skalowania [% 1T/thread]")
    ax1.set_title("CPU FMA: sprawność skalowania")
    ax1.set_ylim(0.0, max(110.0, max(eff[x] * 100.0 for x in xs) * 1.08))
    apply_axis_style(ax1, grid_axis="both")

    fig.suptitle("CPU: główna figura sufitu obliczeniowego zamiast wielu wariantów pomocniczych", fontsize=14)
    out = THESIS_CORE_DIR / "cpu_peak_compute_scaling.png"
    save_figure(fig, out, dpi=220, platform_label=_cpu_platform_label(rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="cpu_peak_compute_scaling",
        section="benchmarks",
        path=out,
        question="Jak CPU skaluje osiągany peak compute między 1T i all-core?",
        summary="Zastępuje trzy osobne wykresy jedną figurą throughput + efficiency.",
    )
    meta.update({"aggregation": "best per thread count across repeated peak-FMA runs"})
    return out, meta


def _plot_cpu_memory_latency(plt: Any, cpu_dir: Path) -> tuple[Path | None, dict[str, Any]]:
    pointer_rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*pointer_latency*.csv"))))
    tlb_rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*tlb_latency*.csv"))))
    if not pointer_rows and not tlb_rows:
        return None, {}

    pointer_points = aggregated_latency_points(pointer_rows, x_field="working_set_bytes", label_field="estimated_residency")
    tlb_points = aggregated_latency_points(tlb_rows, x_field="pages_touched")

    fig, axes = plt.subplots(1, 2, figsize=(12.8, 4.8))
    ax_cache, ax_tlb = axes

    if pointer_points:
        xs = [p["x"] for p in pointer_points]
        ys = [p["latency_ns_median"] for p in pointer_points]
        yerr = [p["latency_ns_std"] for p in pointer_points]
        ax_cache.errorbar(xs, ys, yerr=yerr, fmt="-o", linewidth=1.5, color="#111111", capsize=3)
        add_cache_boundary_lines(ax_cache, pointer_rows)
        ax_cache.set_xscale("log", base=2)
        ax_cache.set_xlabel("Rozmiar zbioru roboczego [B]")
        ax_cache.set_ylabel("Mediana opóźnienia [ns/dostęp]")
        ax_cache.set_title("Hierarchia cache CPU")
        apply_axis_style(ax_cache, grid_axis="both")
    else:
        ax_cache.text(0.5, 0.5, "Brak danych cache", transform=ax_cache.transAxes, ha="center", va="center")
        ax_cache.set_axis_off()

    if tlb_points:
        xs = [p["x"] for p in tlb_points]
        ys = [p["latency_ns_median"] for p in tlb_points]
        yerr = [p["latency_ns_std"] for p in tlb_points]
        ax_tlb.errorbar(xs, ys, yerr=yerr, fmt="-o", linewidth=1.5, color="#991b1b", capsize=3)
        ax_tlb.set_xscale("log", base=2)
        ax_tlb.set_xlabel("Liczba dotykanych stron")
        ax_tlb.set_ylabel("Mediana opóźnienia [ns/dostęp]")
        ax_tlb.set_title("TLB i page-walk")
        apply_axis_style(ax_tlb, grid_axis="both")
    else:
        ax_tlb.text(0.5, 0.5, "Brak danych TLB", transform=ax_tlb.transAxes, ha="center", va="center")
        ax_tlb.set_axis_off()

    fig.suptitle("Opóźnienia pamięci CPU: cache i TLB w jednej figurze", fontsize=14)
    out = THESIS_CORE_DIR / "cpu_memory_latency_hierarchy.png"
    save_figure(fig, out, dpi=220, platform_label=_cpu_platform_label(pointer_rows or tlb_rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="cpu_memory_latency_hierarchy",
        section="benchmarks",
        path=out,
        question="Gdzie pojawiają się koszty przejścia między poziomami cache i translacji stron?",
        summary="Scala wcześniejsze publication/debug variants do jednej finalnej figury 1x2.",
    )
    meta.update({"aggregation": "median latency with sigma across repeated pointer-chase runs"})
    return out, meta


def _plot_gpu_microbenchmark_suite(plt: Any, gpu_dir: Path) -> tuple[Path | None, dict[str, Any]]:
    bandwidth_rows = _read_rows(_pick_latest_unique(sorted(gpu_dir.glob("*bandwidth*.csv"))))
    compute_rows = _read_rows(_pick_latest_unique(sorted(gpu_dir.glob("*compute*.csv"))))
    latency_rows = _read_rows(_pick_latest_unique(sorted(gpu_dir.glob("*pointer_latency*.csv"))))
    if not any((bandwidth_rows, compute_rows, latency_rows)):
        return None, {}

    fig, axes = plt.subplots(1, 3, figsize=(15.2, 4.8))
    ax_bw, ax_comp, ax_lat = axes

    if bandwidth_rows:
        med = _median_group(bandwidth_rows, key_fields=("transfer_kind", "size_bytes"), value_field="throughput_gbps")
        series: dict[str, dict[int, float]] = defaultdict(dict)
        for (transfer_kind, size_bytes), value in med.items():
            series[str(transfer_kind)][int(size_bytes) // (1024 * 1024)] = value
        for label, mapping in sorted(series.items()):
            xs = sorted(mapping.keys())
            ys = [mapping[x] for x in xs]
            ax_bw.plot(
                xs,
                ys,
                marker="o",
                linewidth=1.7,
                label=_gpu_transfer_label(label),
                color=backend_color("metal") if "device_to_device" in label else "#334155",
            )
        ax_bw.set_xscale("log", base=2)
        ax_bw.set_xlabel("Rozmiar bufora [MB]")
        ax_bw.set_ylabel("Przepustowość [GB/s]")
        ax_bw.set_title("GPU: przepustowość względem rozmiaru")
        apply_axis_style(ax_bw, grid_axis="both")
        ax_bw.legend(frameon=False, fontsize=7)
    else:
        ax_bw.text(0.5, 0.5, "Brak danych przepustowości", transform=ax_bw.transAxes, ha="center", va="center")
        ax_bw.set_axis_off()

    if compute_rows:
        grouped: dict[str, list[float]] = defaultdict(list)
        for row in compute_rows:
            label = str(row.get("benchmark") or Path(str(row.get("source_file", ""))).stem or "compute")
            value = _to_float(row.get("gflops") or row.get("throughput_gflops") or row.get("gflops_peak"))
            if value is not None:
                grouped[_gpu_compute_label(label)].append(value)
        labels = sorted(grouped.keys())
        vals = [max(grouped[label]) for label in labels]
        ax_comp.bar(range(len(labels)), vals, color=backend_color("metal"), alpha=0.9)
        ax_comp.set_xticks(range(len(labels)), labels, rotation=18, ha="right")
        ax_comp.set_ylabel("GFLOP/s")
        ax_comp.set_title("GPU: mikrobenchmarki obliczeniowe")
        apply_axis_style(ax_comp, grid_axis="y")
    else:
        ax_comp.text(0.5, 0.5, "Brak danych obliczeniowych", transform=ax_comp.transAxes, ha="center", va="center")
        ax_comp.set_axis_off()

    if latency_rows:
        med = _median_group(latency_rows, key_fields=("backend", "size_bytes"), value_field="latency_ns")
        series: dict[str, dict[int, float]] = defaultdict(dict)
        for (backend, size_bytes), value in med.items():
            series[str(backend)][int(size_bytes) // 1024] = value
        for backend, mapping in sorted(series.items()):
            xs = sorted(mapping.keys())
            ys = [mapping[x] for x in xs]
            ax_lat.plot(xs, ys, marker="o", linewidth=1.7, label=backend, color=backend_color(backend))
        ax_lat.set_xscale("log", base=2)
        ax_lat.set_xlabel("Rozmiar zbioru roboczego [KB]")
        ax_lat.set_ylabel("Opóźnienie [ns/dostęp]")
        ax_lat.set_title("GPU: opóźnienie łańcucha wskaźników")
        apply_axis_style(ax_lat, grid_axis="both")
        if len(series) > 1:
            ax_lat.legend(frameon=False)
    else:
        ax_lat.text(0.5, 0.5, "Brak danych opóźnień", transform=ax_lat.transAxes, ha="center", va="center")
        ax_lat.set_axis_off()

    fig.suptitle("GPU: mikrobenchmarki przepustowości, obliczeń i opóźnień", fontsize=14)
    out = THESIS_CORE_DIR / "gpu_microbenchmark_suite.png"
    save_figure(fig, out, dpi=220, platform_label=_gpu_platform_label(bandwidth_rows or compute_rows or latency_rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="gpu_microbenchmark_suite",
        section="benchmarks",
        path=out,
        question="Jak wygląda podstawowy profil GPU: przepustowość, peak compute i latency?",
        summary="Scala dwie figury bandwidth i dwa warianty pointer latency do jednego zestawu 1x3.",
    )
    meta.update({"aggregation": "median for size curves, best-achieved for compute bars"})
    return out, meta


def _measured_peaks(cpu_dir: Path, gpu_dir: Path) -> dict[str, dict[str, Any]]:
    cpu_records = _cpu_peaks(cpu_dir) if cpu_dir.exists() else {}
    gpu_records = _gpu_peaks(gpu_dir) if gpu_dir.exists() else {}
    cpu_best = None
    if cpu_records:
        cpu_best = max(cpu_records.values(), key=lambda rec: (float(rec.peak_gflops), float(rec.peak_bw_gbps)))
    gpu_best = None
    if gpu_records:
        gpu_best = max(gpu_records.values(), key=lambda rec: (float(rec.peak_gflops), float(rec.peak_bw_gbps)))
    return {
        "cpu": {
            "record": cpu_best,
            "peak_bw_gbps": float(cpu_best.peak_bw_gbps) if cpu_best is not None else float("nan"),
            "peak_gflops": float(cpu_best.peak_gflops) if cpu_best is not None else float("nan"),
            "model": str(cpu_best.model) if cpu_best is not None else "",
            "backend": "cpu",
        },
        "gpu": {
            "record": gpu_best,
            "peak_bw_gbps": float(gpu_best.peak_bw_gbps) if gpu_best is not None else float("nan"),
            "peak_gflops": float(gpu_best.peak_gflops) if gpu_best is not None else float("nan"),
            "model": str(gpu_best.model) if gpu_best is not None else "",
            "backend": str(gpu_best.backend) if gpu_best is not None else "",
        },
    }


def _roofline_curve(peak_bw_gbps: float, peak_gflops: float) -> tuple[list[float], list[float]]:
    xs = [2 ** p for p in np.linspace(-6, 6, 49)]
    ys = [min(peak_gflops, peak_bw_gbps * x) for x in xs]
    return xs, ys


def _plot_platform_roofline(plt: Any, cpu_dir: Path, gpu_dir: Path) -> tuple[Path | None, dict[str, Any], dict[str, dict[str, Any]]]:
    peaks = _measured_peaks(cpu_dir, gpu_dir)
    available = [target for target in ("cpu", "gpu") if math.isfinite(peaks[target]["peak_bw_gbps"]) and math.isfinite(peaks[target]["peak_gflops"])]
    if not available:
        return None, {}, peaks
    cpu_rows = _read_rows(_pick_latest_unique(sorted(cpu_dir.glob("*.csv"))))
    gpu_rows = _read_rows(_pick_latest_unique(sorted(gpu_dir.glob("*.csv"))))
    system_arch = _system_arch_tag(cpu_rows or gpu_rows)

    fig, axes = plt.subplots(1, len(available), figsize=(7.0 * len(available), 4.9), squeeze=False)
    for idx, target in enumerate(available):
        ax = axes[0][idx]
        rec = peaks[target]
        xs, ys = _roofline_curve(rec["peak_bw_gbps"], rec["peak_gflops"])
        ax.plot(xs, ys, color=backend_color(rec.get("backend", target)), linewidth=2.0)
        ax.axhline(rec["peak_gflops"], color="#94a3b8", linestyle="--", linewidth=1.0)
        ax.set_xscale("log", base=2)
        ax.set_xlabel("Intensywność arytmetyczna [FLOP/bajt]")
        ax.set_ylabel("Osiągalna wydajność [GFLOP/s]")
        ax.set_title(f"Zmierzony roofline: {target.upper()}")
        ax.text(
            0.03,
            0.06,
            f"Sufit przepustowości: {rec['peak_bw_gbps']:.1f} GB/s\nSufit obliczeniowy: {rec['peak_gflops']:.1f} GFLOP/s",
            transform=ax.transAxes,
            fontsize=8.5,
            color="#334155",
        )
        apply_axis_style(ax, grid_axis="both")
    fig.suptitle("Zmierzony roofline wykorzystywany w dalszej interpretacji", fontsize=14)
    out = THESIS_CORE_DIR / "platform_roofline_measured.png"
    save_figure(
        fig,
        out,
        dpi=220,
        platform_label=_roofline_platform_label(
            peaks["cpu"].get("model") or None,
            peaks["gpu"].get("backend") or None,
            peaks["gpu"].get("model") or None,
            system_arch=system_arch or None,
        ),
    )
    plt.close(fig)
    meta = figure_entry(
        figure_id="platform_roofline_measured",
        section="benchmarks",
        path=out,
        question="Jakie są zmierzone pułapy bandwidth i compute używane dalej jako ceilings interpretacyjne?",
        summary="Usuwa syntetyczny punkt AI=8 i zostawia same zmierzone sufity roofline dla CPU i GPU.",
    )
    meta.update({"aggregation": "measured peak envelopes from microbenchmark CSV files"})
    return out, meta, peaks


def _kernel_ai_estimate(row: dict[str, str]) -> float | None:
    kernel = str(row.get("kernel", "")).strip().lower()
    dtype = str(row.get("dtype", "float32")).strip().lower()
    itemsize = 8.0 if dtype == "float64" else 4.0

    if kernel == "gemm":
        m = _to_int(row.get("m"))
        n = _to_int(row.get("n"))
        k = _to_int(row.get("k"))
        if not (m and n and k):
            return None
        flops = 2.0 * float(m) * float(n) * float(k)
        bytes_total = (float(m * k) + float(k * n) + float(m * n)) * itemsize
        return flops / max(bytes_total, 1.0)

    if kernel == "spmv":
        n = _to_int(row.get("n"))
        nnz_per_row = _to_int(row.get("nnz_per_row"))
        if not (n and nnz_per_row):
            return None
        nnz = float(n * nnz_per_row)
        flops = 2.0 * nnz
        bytes_total = nnz * itemsize + nnz * 4.0 + nnz * itemsize + float(n) * itemsize
        return flops / max(bytes_total, 1.0)

    if kernel == "fem_integration":
        return _to_float(row.get("ai_flop_per_byte"))

    if kernel == "assembly_like":
        ai = _to_float(row.get("ai_flop_per_byte"))
        if ai is not None:
            return ai
        n_elements = _to_int(row.get("n_elements"))
        n_qp = _to_int(row.get("n_qp"))
        n_dofs = _to_int(row.get("n_dofs"))
        if not (n_elements and n_qp and n_dofs):
            return None
        flops = float(n_elements * n_qp * n_dofs * n_dofs * 6)
        bytes_total = float(n_elements * n_qp * (3 * n_dofs + 2 * n_dofs * n_dofs)) * itemsize
        return flops / max(bytes_total, 1.0)

    if kernel == "fem":
        n_elements = _to_int(row.get("n_elements"))
        n_qp = _to_int(row.get("n_qp"))
        if not (n_elements and n_qp):
            return None
        flops = float(n_elements * n_qp * (9 * 2))
        bytes_total = float((n_elements * 9 + n_qp * 9 + n_elements) * itemsize)
        return flops / max(bytes_total, 1.0)

    if kernel == "stencil2d":
        return 0.18
    if kernel == "stencil3d":
        return 0.12
    if kernel == "reduction":
        return 0.06
    return None


def _load_real_rows(real_dirs: list[Path]) -> list[dict[str, str]]:
    csv_paths: list[Path] = []
    seen: set[str] = set()
    for real_dir in real_dirs:
        for path in _pick_latest_unique(sorted(real_dir.glob("*.csv"))):
            key = str(path.resolve())
            if key in seen:
                continue
            csv_paths.append(path)
            seen.add(key)
    return _read_rows(csv_paths)


def _plot_real_kernels_model_validation(plt: Any, real_dirs: list[Path], peaks: dict[str, dict[str, Any]]) -> tuple[Path | None, dict[str, Any], list[dict[str, str]]]:
    rows = [row for row in _load_real_rows(real_dirs) if str(row.get("status", "ok")) == "ok"]
    if not rows:
        return None, {}, []

    grouped_compute: dict[tuple[str, str], list[float]] = defaultdict(list)
    grouped_memory: dict[tuple[str, str], list[float]] = defaultdict(list)
    omitted: list[str] = []
    for row in rows:
        kernel = str(row.get("kernel", "")).strip().lower()
        backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
        target = "cpu" if backend == "cpu" else "gpu"
        peak_compute = _to_float(peaks.get(target, {}).get("peak_gflops"))
        peak_bw = _to_float(peaks.get(target, {}).get("peak_bw_gbps"))
        gflops = _to_float(row.get("gflops"))
        gbps = _to_float(row.get("throughput_gbps") or row.get("gbps"))
        if gflops is not None and peak_compute and peak_compute > 0:
            util = 100.0 * gflops / peak_compute
            if util > 110.0:
                label = f"{kernel}/{backend}"
                if label not in omitted:
                    omitted.append(label)
                continue
            grouped_compute[(kernel, backend)].append(util)
        elif gbps is not None and peak_bw and peak_bw > 0:
            util = 100.0 * gbps / peak_bw
            if util > 110.0:
                label = f"{kernel}/{backend}"
                if label not in omitted:
                    omitted.append(label)
                continue
            grouped_memory[(kernel, backend)].append(util)

    if not grouped_compute and not grouped_memory:
        return None, {}, rows

    fig, axes = plt.subplots(1, 2, figsize=(13.6, 5.0))
    ax0, ax1 = axes

    def plot_group(ax: Any, grouped: dict[tuple[str, str], list[float]], title: str) -> None:
        if not grouped:
            ax.text(0.5, 0.5, "Brak danych", transform=ax.transAxes, ha="center", va="center")
            ax.set_axis_off()
            return
        labels = [f"{kernel}\n{backend}" for kernel, backend in sorted(grouped.keys())]
        vals = [median(grouped[key]) for key in sorted(grouped.keys())]
        colors = [algorithm_color(key[0]) for key in sorted(grouped.keys())]
        ax.bar(range(len(labels)), vals, color=colors, alpha=0.9)
        ax.set_xticks(range(len(labels)), labels, rotation=18, ha="right")
        ax.set_ylabel("Wykorzystanie zmierzonego sufitu [%]")
        ax.set_title(title)
        ax.set_ylim(0.0, max(105.0, max(vals) * 1.15))
        apply_axis_style(ax, grid_axis="y")

    plot_group(ax0, grouped_compute, "Jądra obliczeniowe względem zmierzonego sufitu obliczeniowego")
    plot_group(ax1, grouped_memory, "Jądra pamięciowe względem zmierzonego sufitu przepustowości")

    fig.suptitle("Real kernels: walidacja modelu mikrobenchmarkowego", fontsize=14)
    if omitted:
        fig.text(
            0.02,
            0.02,
            "Pominięto punkty poza zakresem modelu (>110% zmierzonego sufitu), np. vendor/library-specific paths: "
            + ", ".join(sorted(omitted)[:6]),
            fontsize=8.2,
            color="#475569",
        )
    out = THESIS_CORE_DIR / "real_kernels_model_validation.png"
    save_figure(fig, out, dpi=220, platform_label=_real_platform_label(rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="real_kernels_model_validation",
        section="real_kernels",
        path=out,
        question="Na ile real kernels zbliżają się do zmierzonych sufitów compute i bandwidth?",
        summary="Centralna figura walidacyjna łącząca mikrobenchmark ceilings z realistycznymi kernelami.",
    )
    meta.update({"aggregation": "median utilization relative to measured per-target ceiling; out-of-model points omitted above 110%"})
    return out, meta, rows


def _plot_real_kernels_filip_contrast_map(plt: Any, rows: list[dict[str, str]], peaks: dict[str, dict[str, Any]]) -> tuple[Path | None, dict[str, Any]]:
    if not rows:
        return None, {}
    grouped: dict[tuple[str, str], list[tuple[float, float]]] = defaultdict(list)
    omitted: list[str] = []
    for row in rows:
        kernel = str(row.get("kernel", "")).strip().lower()
        backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
        ai = _kernel_ai_estimate(row)
        gflops = _to_float(row.get("gflops"))
        if ai is None or gflops is None or not math.isfinite(ai) or not math.isfinite(gflops) or ai <= 0:
            continue
        target = "cpu" if backend == "cpu" else "gpu"
        peak_compute = _to_float(peaks.get(target, {}).get("peak_gflops"))
        if peak_compute is not None and peak_compute > 0 and gflops > peak_compute * 1.10:
            label = f"{kernel}/{backend}"
            if label not in omitted:
                omitted.append(label)
            continue
        grouped[(kernel, backend)].append((ai, gflops))
    if not grouped:
        return None, {}

    fig, ax = plt.subplots(figsize=(10.4, 5.8))

    # roofline overlays for CPU and GPU
    for target in ("cpu", "gpu"):
        peak_bw = _to_float(peaks.get(target, {}).get("peak_bw_gbps"))
        peak_gflops = _to_float(peaks.get(target, {}).get("peak_gflops"))
        if peak_bw is None or peak_gflops is None or peak_bw <= 0 or peak_gflops <= 0:
            continue
        xs, ys = _roofline_curve(peak_bw, peak_gflops)
        backend = peaks[target].get("backend") or target
        label = f"{target.upper()} roofline zmierzony"
        ax.plot(xs, ys, color=backend_color(str(backend)), linewidth=1.6, alpha=0.65, label=label)

    markers = {"cpu": "o", "metal": "s", "cuda": "^", "hip": "D", "opencl": "P"}
    label_map = {
        "spmv": "SpMV (kontrast ograniczony pamięcią)",
        "fem_integration": "Integracja FEM (podobna do Filipa)",
        "fem": "Uproszczony FEM",
        "gemm": "GEMM (referencja obliczeniowa)",
        "reduction": "Redukcja",
        "stencil2d": "Stencil 2D",
        "stencil3d": "Stencil 3D",
    }
    for (kernel, backend), pts in sorted(grouped.items()):
        xs = [median([p[0] for p in pts])]
        ys = [median([p[1] for p in pts])]
        ax.scatter(
            xs,
            ys,
            s=88,
            color=algorithm_color(kernel),
            marker=markers.get(backend, "o"),
            edgecolors="#111111",
            linewidths=0.6,
            label=f"{label_map.get(kernel, kernel)} | {backend}",
        )
        ax.annotate(backend, (xs[0], ys[0]), textcoords="offset points", xytext=(5, 4), fontsize=8)

    ax.set_xscale("log")
    ax.set_xlabel("Intensywność arytmetyczna [FLOP/bajt]")
    ax.set_ylabel("Osiągana wydajność [GFLOP/s]")
    ax.set_title("Real kernels w przestrzeni roofline: FEM podobny do Filipa vs kontrast ograniczony pamięcią")
    apply_axis_style(ax, grid_axis="both")
    ax.legend(frameon=False, fontsize=7, ncol=2, loc="upper left")
    if omitted:
        fig.text(
            0.02,
            0.02,
            "Pominięto punkty poza zakresem modelu (>110% zmierzonego sufitu): " + ", ".join(sorted(omitted)[:6]),
            fontsize=8.2,
            color="#475569",
        )
    out = THESIS_CORE_DIR / "real_kernels_filip_contrast_map.png"
    save_figure(fig, out, dpi=220, platform_label=_real_platform_label(rows))
    plt.close(fig)
    meta = figure_entry(
        figure_id="real_kernels_filip_contrast_map",
        section="real_kernels",
        path=out,
        question="Gdzie Filip-like FEM leży w przestrzeni roofline względem SpMV i GEMM?",
        summary="Główna figura interpretacyjna łącząca AI, achieved GFLOP/s i measured roofline ceilings.",
    )
    meta.update({"aggregation": "median AI and median achieved GFLOP/s per kernel/backend pair; out-of-model points omitted above 110%"})
    return out, meta


def _write_manifest(entries: list[dict[str, Any]], *, source_label: str, cpu_dir: Path, gpu_dir: Path, real_dirs: list[Path]) -> Path:
    ensure_figure_dirs()
    manifest_path = MANIFEST_DIR / "thesis_core_manifest.json"
    payload = {
        "figure_set": FIGURE_SET_ID,
        "source_label": source_label,
        "cpu_dir": str(cpu_dir),
        "gpu_dir": str(gpu_dir),
        "real_dirs": [str(path) for path in real_dirs],
        "output_dir": str(THESIS_CORE_DIR),
        "figures": entries,
    }
    manifest_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    return manifest_path


def generate_thesis_core_figures(*, clean: bool = True) -> dict[str, Any]:
    ensure_figure_dirs()
    if clean:
        clear_pngs(THESIS_CORE_DIR)

    plt = _try_import_matplotlib()
    if plt is None:
        raise SystemExit("matplotlib unavailable")
    setup_publication_theme(plt)

    cpu_dir, gpu_dir, real_dirs, source_label = _data_dirs()
    print(f"[INFO] Plot source mode: {source_label}")
    print(f"[INFO] Plot source CPU:  {cpu_dir}")
    print(f"[INFO] Plot source GPU:  {gpu_dir}")
    print(f"[INFO] Plot source REAL: {[str(p) for p in real_dirs]}")

    entries: list[dict[str, Any]] = []

    for builder in (
        lambda: _plot_cpu_memcpy_suite(plt, cpu_dir),
        lambda: _plot_cpu_stream_suite(plt, cpu_dir),
        lambda: _plot_cpu_compute_scaling(plt, cpu_dir),
        lambda: _plot_cpu_memory_latency(plt, cpu_dir),
        lambda: _plot_gpu_microbenchmark_suite(plt, gpu_dir),
    ):
        _path, meta = builder()
        if meta:
            entries.append(meta)

    roof_path, roof_meta, peaks = _plot_platform_roofline(plt, cpu_dir, gpu_dir)
    if roof_meta:
        entries.append(roof_meta)

    real_path, real_meta, real_rows = _plot_real_kernels_model_validation(plt, real_dirs, peaks)
    if real_meta:
        entries.append(real_meta)

    contrast_path, contrast_meta = _plot_real_kernels_filip_contrast_map(plt, real_rows, peaks)
    if contrast_meta:
        entries.append(contrast_meta)

    manifest_path = _write_manifest(entries, source_label=source_label, cpu_dir=cpu_dir, gpu_dir=gpu_dir, real_dirs=real_dirs)
    result = {
        "figure_set": FIGURE_SET_ID,
        "output_dir": str(THESIS_CORE_DIR),
        "manifest_path": str(manifest_path),
        "generated_figures": [entry["path"] for entry in entries],
        "figure_count": len(entries),
    }
    print(json.dumps(result, indent=2, ensure_ascii=False))
    return result


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate compact thesis-ready figure set from benchmark and real-kernel data.")
    ap.add_argument("--no-clean", action="store_true", help="Do not remove previous thesis-core PNG files before generation.")
    args = ap.parse_args()
    generate_thesis_core_figures(clean=not args.no_clean)


if __name__ == "__main__":
    main()
