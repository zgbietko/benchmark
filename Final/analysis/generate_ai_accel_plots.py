#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import math
import os
from collections import defaultdict
from pathlib import Path
from statistics import median
from typing import Any

import numpy as np  # type: ignore

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in __import__("sys").path:
    __import__("sys").path.insert(0, str(ROOT))

from analysis.publication_style import (
    THESIS_CORE_DIR,
    add_figure_note,
    apply_axis_style,
    backend_color,
    ensure_figure_dirs,
    save_figure,
    setup_publication_theme,
)

RUNS_DIR = ROOT / "data" / "runs"
GLOBAL_DIR = ROOT / "data" / "ai_accel"

STATUS_ORDER = ["native", "proxy", "fallback", "mixed", "unsupported", "failed", "not_measured"]
STATUS_COLOR = {
    "native": "#16a34a",
    "proxy": "#2563eb",
    "fallback": "#b45309",
    "mixed": "#7c3aed",
    "unsupported": "#9ca3af",
    "failed": "#dc2626",
    "not_measured": "#f3f4f6",
}
STATUS_SHORT = {
    "native": "N",
    "proxy": "P",
    "fallback": "F",
    "mixed": "M",
    "unsupported": "U",
    "failed": "X",
    "not_measured": "NA",
}


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


def _q1_q3(values: list[float]) -> tuple[float, float]:
    if not values:
        return float("nan"), float("nan")
    q1, q3 = np.percentile(values, [25.0, 75.0])
    return float(q1), float(q3)


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


def _latest_session_with_ai_dir() -> Path | None:
    latest_candidate: Path | None = None
    latest_mtime = -1.0
    if not RUNS_DIR.exists():
        return None
    for entry in RUNS_DIR.iterdir():
        if not entry.is_dir():
            continue
        ai_dir = entry / "ai_accel"
        if not ai_dir.exists() or not ai_dir.is_dir():
            continue
        if not any(ai_dir.glob("*.csv")):
            continue
        try:
            mtime = ai_dir.stat().st_mtime
        except Exception:
            mtime = 0.0
        if mtime > latest_mtime:
            latest_mtime = mtime
            latest_candidate = ai_dir
    return latest_candidate


def _source_dir(scope: str, session: str) -> Path:
    run_root = os.environ.get("BENCH_RUN_DIR", "").strip()
    if run_root:
        p = Path(run_root) / "ai_accel"
        if p.exists():
            return p
    if scope == "session":
        if session and session != "latest":
            requested = RUNS_DIR / session / "ai_accel"
            if requested.exists():
                return requested
        latest = _latest_session_dir()
        if latest is not None:
            latest_ai = latest / "ai_accel"
            if latest_ai.exists():
                return latest_ai
        fallback = _latest_session_with_ai_dir()
        if fallback is not None:
            return fallback
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
    return "Platforma testowa: " + " | ".join(parts) if parts else ""


def _row_status(row: dict[str, str]) -> str:
    status = str(row.get("status", "")).strip().lower()
    validation_status = str(row.get("validation_status", "")).strip().lower()
    impl = str(row.get("implementation_level", "")).strip().lower()
    if status in {"unsupported", "skipped"} or validation_status == "unsupported":
        return "unsupported"
    if status and status != "ok":
        return "failed"
    if "fallback" in impl:
        return "fallback"
    if "proxy" in impl:
        return "proxy"
    if impl.startswith("native"):
        return "native"
    return "mixed" if impl else "not_measured"


def _merge_status(values: set[str]) -> str:
    if not values:
        return "not_measured"
    if len(values) == 1:
        only = next(iter(values))
        return only if only in STATUS_ORDER else "mixed"
    # Any combination of available states is explicitly marked as mixed.
    if values <= {"unsupported", "failed"}:
        return "unsupported" if "unsupported" in values else "failed"
    return "mixed"


def _measurement_note(rows: list[dict[str, str]], *, extra: str = "") -> str:
    reps = {str(r.get("run_idx") or r.get("run_id") or "").strip() for r in rows if str(r.get("run_idx") or r.get("run_id") or "").strip()}
    shapes = {
        f"{str(r.get('m', '')).strip()}x{str(r.get('n', '')).strip()}x{str(r.get('k', '')).strip()}"
        for r in rows
        if str(r.get("m", "")).strip() and str(r.get("n", "")).strip() and str(r.get("k", "")).strip()
    }
    parts = [
        "metryka: mediana",
        "errorbar: IQR",
        f"próbek: {len(rows)}",
    ]
    if reps:
        parts.append(f"powtórzenia (run_idx/run_id): {len(reps)}")
    if shapes:
        parts.append(f"kształty MxNxK: {len(shapes)}")
    if extra:
        parts.append(extra)
    return " | ".join(parts)


def _matmul_rows(rows: list[dict[str, str]]) -> list[dict[str, str]]:
    return [row for row in rows if str(row.get("kernel", "")).strip().lower() == "matmul"]


def _status_matrix(matmul_rows: list[dict[str, str]]) -> tuple[list[str], list[str], dict[tuple[str, str], str]]:
    backends = sorted({str(r.get("backend", "unknown")).strip().lower() or "unknown" for r in matmul_rows})
    prec_order = ["float64", "float32", "float16", "int8"]
    present_prec = {str(r.get("precision", "")).strip().lower() or "unknown" for r in matmul_rows}
    precisions = [p for p in prec_order if p in present_prec]
    precisions += sorted([p for p in present_prec if p not in precisions])
    raw: dict[tuple[str, str], set[str]] = defaultdict(set)
    for row in matmul_rows:
        backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
        precision = str(row.get("precision", "")).strip().lower() or "unknown"
        raw[(backend, precision)].add(_row_status(row))
    combined = {key: _merge_status(values) for key, values in raw.items()}
    return backends, precisions, combined


def _draw_backend_precision_status(ax: Any, backends: list[str], precisions: list[str], status_map: dict[tuple[str, str], str]) -> None:
    if not backends or not precisions:
        ax.text(0.5, 0.5, "Brak danych statusów", transform=ax.transAxes, ha="center", va="center")
        ax.set_axis_off()
        return
    color_order = [STATUS_COLOR[key] for key in STATUS_ORDER]
    cmap = __import__("matplotlib").colors.ListedColormap(color_order)
    code_map = {label: idx for idx, label in enumerate(STATUS_ORDER)}
    matrix = np.full((len(backends), len(precisions)), code_map["not_measured"], dtype=float)
    for bi, backend in enumerate(backends):
        for pi, precision in enumerate(precisions):
            matrix[bi, pi] = float(code_map[status_map.get((backend, precision), "not_measured")])
    ax.imshow(matrix, aspect="auto", cmap=cmap, vmin=0, vmax=len(STATUS_ORDER) - 1)
    ax.set_xticks(range(len(precisions)), precisions, rotation=18, ha="right")
    ax.set_yticks(range(len(backends)), backends)
    ax.set_title("Status ścieżki backend × precyzja")
    for bi, backend in enumerate(backends):
        for pi, precision in enumerate(precisions):
            label = STATUS_SHORT.get(status_map.get((backend, precision), "not_measured"), "NA")
            ax.text(pi, bi, label, ha="center", va="center", fontsize=8, color="#0f172a")
    apply_axis_style(ax, grid_axis="none")


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate AI acceleration publication plots.")
    ap.add_argument("--scope", choices=["global", "session"], default="session")
    ap.add_argument("--session", default="latest")
    args = ap.parse_args()

    src = _source_dir(args.scope, args.session)
    print(f"[INFO] AI plot source: {src}")
    if not src.exists():
        print("[WARN] source does not exist")
        return

    all_rows = _read_rows(src)
    if not all_rows:
        print("[WARN] no rows")
        return
    ok_rows = [r for r in all_rows if str(r.get("status", "ok")).strip().lower() == "ok"]
    matmul_all = _matmul_rows(all_rows)
    matmul_ok = _matmul_rows(ok_rows)

    try:
        import matplotlib.pyplot as plt  # type: ignore
    except Exception as exc:
        print(f"[WARN] matplotlib unavailable: {exc}")
        return

    ensure_figure_dirs()
    setup_publication_theme(plt)

    # Build shared containers.
    perf_by_combo: dict[tuple[str, str], list[float]] = defaultdict(list)
    elapsed_by_backend_ops: dict[tuple[str, float], list[float]] = defaultdict(list)
    status_backends, status_precisions, status_map = _status_matrix(matmul_all)
    coreml_by_units: dict[tuple[str, str], list[float]] = defaultdict(list)

    for row in matmul_ok:
        backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
        precision = str(row.get("precision", "")).strip().lower() or "unknown"
        gflops = _to_float(row.get("gflops"))
        if gflops is not None and math.isfinite(gflops):
            perf_by_combo[(backend, precision)].append(float(gflops))
        elapsed = _to_float(row.get("elapsed_s"))
        ops = _to_float(row.get("ops_count"))
        if elapsed is not None and ops is not None and elapsed > 0 and ops > 0:
            elapsed_by_backend_ops[(backend, float(ops))].append(float(elapsed))

    for row in ok_rows:
        kernel = str(row.get("kernel", "")).strip().lower()
        if kernel != "coreml_mlp_predict":
            continue
        backend = str(row.get("backend", "unknown")).strip().lower() or "unknown"
        units = str(row.get("compute_units", "")).strip().lower() or "all"
        gflops = _to_float(row.get("gflops"))
        if gflops is not None and math.isfinite(gflops):
            coreml_by_units[(backend, units)].append(float(gflops))

    # 1) ai_accel_overview.png
    fig, axes = plt.subplots(1, 2, figsize=(13.6, 5.2))
    ax0, ax1 = axes

    combo_keys = sorted(perf_by_combo.keys())
    if combo_keys:
        labels = [f"{backend}\n{precision}" for backend, precision in combo_keys]
        med_vals = [float(median(perf_by_combo[key])) for key in combo_keys]
        lows: list[float] = []
        highs: list[float] = []
        colors: list[str] = []
        for key in combo_keys:
            values = perf_by_combo[key]
            q1, q3 = _q1_q3(values)
            m = float(median(values))
            lows.append(max(0.0, m - q1) if math.isfinite(q1) else 0.0)
            highs.append(max(0.0, q3 - m) if math.isfinite(q3) else 0.0)
            colors.append(backend_color(key[0]))
        ax0.bar(range(len(combo_keys)), med_vals, color=colors, alpha=0.92)
        ax0.errorbar(
            range(len(combo_keys)),
            med_vals,
            yerr=[lows, highs],
            fmt="none",
            ecolor="#0f172a",
            alpha=0.4,
            capsize=2.5,
            linewidth=0.9,
        )
        ax0.set_xticks(range(len(combo_keys)), labels, rotation=20, ha="right")
        ax0.set_ylabel("GFLOP/s (mediana)")
        ax0.set_title("AI matmul: backend × precyzja")
        apply_axis_style(ax0, grid_axis="y")
    else:
        ax0.text(0.5, 0.5, "Brak poprawnych pomiarów matmul", transform=ax0.transAxes, ha="center", va="center")
        ax0.set_axis_off()

    _draw_backend_precision_status(ax1, status_backends, status_precisions, status_map)
    fig.suptitle("Przegląd akceleracji AI: wydajność i status ścieżek", fontsize=13.5)
    add_figure_note(
        fig,
        _measurement_note(
            all_rows,
            extra="Kody statusów: N-native, P-proxy, F-fallback, U-unsupported, M-mixed, X-failed, NA-not measured.",
        ),
    )
    out = THESIS_CORE_DIR / "ai_accel_overview.png"
    save_figure(fig, out, dpi=220, platform_label=_platform_label(all_rows))
    plt.close(fig)
    print(f"[OK] Plot: {out}")

    # 2) ai_accel_break_even.png
    fig2, axes2 = plt.subplots(1, 2, figsize=(14.0, 5.0))
    ax_time, ax_speed = axes2
    backend_ops_points: dict[str, list[tuple[float, float, float, float]]] = defaultdict(list)
    for (backend, ops), values in elapsed_by_backend_ops.items():
        med = float(median(values))
        q1, q3 = _q1_q3(values)
        low = max(0.0, med - q1) if math.isfinite(q1) else 0.0
        high = max(0.0, q3 - med) if math.isfinite(q3) else 0.0
        backend_ops_points[backend].append((ops, med, low, high))

    for backend, pts in sorted(backend_ops_points.items()):
        pts.sort(key=lambda x: x[0])
        xs = [p[0] for p in pts]
        ys = [p[1] for p in pts]
        low = [p[2] for p in pts]
        high = [p[3] for p in pts]
        ax_time.errorbar(
            xs,
            ys,
            yerr=[low, high],
            marker="o",
            linewidth=1.8,
            capsize=2.5,
            label=backend,
            color=backend_color(backend),
        )
    ax_time.set_xscale("log")
    ax_time.set_yscale("log")
    ax_time.set_xlabel("Liczba FLOP (log)")
    ax_time.set_ylabel("Czas [s] (mediana)")
    ax_time.set_title("Czas wykonania")
    apply_axis_style(ax_time, grid_axis="both")
    if backend_ops_points:
        ax_time.legend(frameon=True, loc="best")

    cpu_ref = {ops: med for ops, med, _low, _high in backend_ops_points.get("cpu", [])}
    speed_any = False
    for backend, pts in sorted(backend_ops_points.items()):
        if backend == "cpu":
            continue
        pairs: list[tuple[float, float]] = []
        for ops, med, _low, _high in pts:
            cpu_med = cpu_ref.get(ops)
            if cpu_med is None or med <= 0:
                continue
            pairs.append((ops, cpu_med / med))
        if not pairs:
            continue
        speed_any = True
        pairs.sort(key=lambda x: x[0])
        ax_speed.plot(
            [p[0] for p in pairs],
            [p[1] for p in pairs],
            marker="o",
            linewidth=1.8,
            label=backend,
            color=backend_color(backend),
        )
    if speed_any:
        ax_speed.axhline(1.0, color="#94a3b8", linestyle="--", linewidth=1.0)
        ax_speed.set_xscale("log")
        ax_speed.set_xlabel("Liczba FLOP (log)")
        ax_speed.set_ylabel("Speedup względem CPU [x]")
        ax_speed.set_title("Punkt przełamania (break-even)")
        apply_axis_style(ax_speed, grid_axis="both")
        ax_speed.legend(frameon=True, loc="best")
    else:
        ax_speed.text(0.5, 0.5, "Brak danych do speedup vs CPU", transform=ax_speed.transAxes, ha="center", va="center")
        ax_speed.set_axis_off()

    fig2.suptitle("AI matmul: break-even CPU vs akceleratory", fontsize=13.5)
    add_figure_note(fig2, _measurement_note(all_rows, extra="Panel lewy: czas; panel prawy: speedup względem CPU dla wspólnych rozmiarów."))
    out2 = THESIS_CORE_DIR / "ai_accel_break_even.png"
    save_figure(fig2, out2, dpi=220, platform_label=_platform_label(all_rows))
    plt.close(fig2)
    print(f"[OK] Plot: {out2}")

    # 3) ai_precision_scaling.png
    fig3, ax3 = plt.subplots(1, 1, figsize=(10.0, 5.0))
    if status_backends and status_precisions:
        positive_values = [float(median(vs)) for vs in perf_by_combo.values() if vs]
        min_positive = min(positive_values) if positive_values else 1e-3
        marker_map = {
            "native": "o",
            "proxy": "s",
            "fallback": "D",
            "mixed": "^",
            "unsupported": "x",
            "failed": "X",
            "not_measured": "P",
        }
        status_code = {"unsupported": "U", "failed": "X", "not_measured": "NA"}
        for backend in status_backends:
            xs_measured: list[int] = []
            ys_measured: list[float] = []
            ylow_measured: list[float] = []
            yhigh_measured: list[float] = []
            status_only: list[tuple[int, str]] = []
            for pi, precision in enumerate(status_precisions):
                key = (backend, precision)
                values = perf_by_combo.get(key, [])
                status = status_map.get(key, "not_measured")
                if values:
                    med = float(median(values))
                    q1, q3 = _q1_q3(values)
                    xs_measured.append(pi)
                    ys_measured.append(med)
                    ylow_measured.append(max(0.0, med - q1) if math.isfinite(q1) else 0.0)
                    yhigh_measured.append(max(0.0, q3 - med) if math.isfinite(q3) else 0.0)
                elif status in {"unsupported", "failed", "not_measured"}:
                    status_only.append((pi, status))
            if not xs_measured and not status_only:
                continue
            if xs_measured:
                line_status = _merge_status({status_map.get((backend, p), "not_measured") for p in status_precisions})
                ax3.errorbar(
                    xs_measured,
                    ys_measured,
                    yerr=[ylow_measured, yhigh_measured],
                    marker=marker_map.get(line_status, "o"),
                    linewidth=1.8,
                    capsize=2.5,
                    label=f"{backend} ({line_status})",
                    color=backend_color(backend),
                )
            y_status = min_positive * 0.35
            for x, status in status_only:
                marker = marker_map.get(status, "x")
                ax3.scatter(
                    [x],
                    [y_status],
                    marker=marker,
                    s=36,
                    linewidths=1.1,
                    color="#7f1d1d",
                )
                ax3.annotate(
                    status_code.get(status, status),
                    (x, y_status),
                    textcoords="offset points",
                    xytext=(2, 4),
                    fontsize=7.1,
                    color="#7f1d1d",
                )
        ax3.set_yscale("log")
        ax3.set_xticks(range(len(status_precisions)), status_precisions)
        ax3.set_ylabel("GFLOP/s (mediana, skala log)")
        ax3.set_xlabel("Precyzja")
        ax3.set_title("AI matmul: skalowanie po precyzjach i statusach ścieżek")
        apply_axis_style(ax3, grid_axis="y")
        ax3.legend(frameon=True, loc="best", fontsize=7.2)
    else:
        ax3.text(0.5, 0.5, "Brak danych precyzji", transform=ax3.transAxes, ha="center", va="center")
        ax3.set_axis_off()

    fig3.suptitle("AI matmul: precyzja vs ścieżka wykonania", fontsize=13.5)
    add_figure_note(fig3, _measurement_note(all_rows, extra="Wartości unsupported/failed/not measured są oznaczane etykietą zamiast interpretacji jako zero."))
    out3 = THESIS_CORE_DIR / "ai_precision_scaling.png"
    save_figure(fig3, out3, dpi=220, platform_label=_platform_label(all_rows))
    plt.close(fig3)
    print(f"[OK] Plot: {out3}")


if __name__ == "__main__":
    main()
