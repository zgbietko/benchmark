from __future__ import annotations

import json
import math
import os
from pathlib import Path
from typing import Any, Dict, List


ROOT = Path(__file__).resolve().parents[1]
_mpl_cfg = ROOT / ".cache" / "matplotlib"
_mpl_cfg.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(_mpl_cfg))


def _try_import_matplotlib():
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt  # type: ignore

        return plt
    except Exception:
        return None


def _read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _read_jsonl(path: Path) -> List[Dict[str, Any]]:
    if not path.exists():
        return []
    rows: List[Dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            text = line.strip()
            if not text:
                continue
            try:
                obj = json.loads(text)
            except Exception:
                continue
            if isinstance(obj, dict):
                rows.append(obj)
    return rows


def _safe_float(value: Any) -> float:
    try:
        out = float(value)
    except Exception:
        return float("nan")
    return out if math.isfinite(out) else float("nan")


def _metric_candidates(eval_rows: List[Dict[str, Any]]) -> List[str]:
    preferred = [
        "metric_gflops_mean",
        "metric_gbps_mean",
        "metric_ns_per_unit",
        "metric_internal_ns_per_elem",
        "metric_kernel_ms",
        "metric_j_per_gflop",
        "metric_j_per_gb",
        "metric_edp",
    ]
    available = {key for row in eval_rows for key in row.keys() if key.startswith("metric_")}
    ordered = [name for name in preferred if name in available]
    for name in sorted(available):
        if name not in ordered:
            ordered.append(name)
    return ordered


def generate_optimization_diagnostics(out_dir: Path) -> Dict[str, str]:
    plt = _try_import_matplotlib()
    if plt is None:
        return {}

    summary_path = out_dir / "summary.json"
    eval_path = out_dir / "evaluations.jsonl"
    iter_path = out_dir / "iterations.jsonl"
    if not eval_path.exists():
        return {}

    summary = _read_json(summary_path) if summary_path.exists() else {}
    eval_rows = _read_jsonl(eval_path)
    iter_rows = _read_jsonl(iter_path)
    if not eval_rows and not iter_rows:
        return {}

    generated: Dict[str, str] = {}
    method = str(summary.get("method", "")).strip().lower()
    iter_label = "Proba" if method == "random_search" else "Iteracja"

    if iter_rows:
        xs = [int(row.get("iteration", row.get("trial", 0))) for row in iter_rows]
        ys = [_safe_float(row.get("best_brightness", row.get("best_score", float("nan")))) for row in iter_rows]
        finite = [(x, y) for x, y in zip(xs, ys) if math.isfinite(y)]
        if finite:
            fx, fy = zip(*finite)
            fig, ax = plt.subplots(figsize=(9.2, 4.8))
            ax.plot(fx, fy, marker="o", linewidth=1.5, color="#1d4ed8")
            ax.set_title("Przebieg najlepszego wyniku optymalizacji")
            ax.set_xlabel(iter_label)
            ax.set_ylabel("Najlepszy score / brightness")
            ax.grid(True, alpha=0.25)
            fig.tight_layout()
            out = out_dir / "optimization_convergence.png"
            fig.savefig(out, dpi=180)
            plt.close(fig)
            generated["convergence_plot"] = str(out)

    metric_candidates = _metric_candidates(eval_rows)
    primary_metric = metric_candidates[0] if metric_candidates else ""
    secondary_metric = metric_candidates[1] if len(metric_candidates) > 1 else ""

    if primary_metric:
        best_by_it: Dict[int, float] = {}
        for row in eval_rows:
            status = str(row.get("status", ""))
            try:
                ok = int(row.get("constraints_ok", 0)) == 1
            except Exception:
                ok = False
            if status != "ok" or not ok:
                continue
            it = int(row.get("iteration", row.get("trial", 0)))
            value = _safe_float(row.get(primary_metric, float("nan")))
            if not math.isfinite(value):
                continue
            prev = best_by_it.get(it)
            if prev is None:
                best_by_it[it] = value
            else:
                if "ns_per_unit" in primary_metric or "kernel_ms" in primary_metric or "j_per_" in primary_metric or primary_metric.endswith("_edp"):
                    if value < prev:
                        best_by_it[it] = value
                elif value > prev:
                    best_by_it[it] = value
        if best_by_it:
            xs = sorted(best_by_it.keys())
            ys = [best_by_it[i] for i in xs]
            fig, ax = plt.subplots(figsize=(9.2, 4.8))
            ax.plot(xs, ys, marker="o", linewidth=1.5, color="#0f766e")
            ax.set_title(f"Najlepsza wartosc metryki {primary_metric.replace('metric_', '')}")
            ax.set_xlabel(iter_label)
            ax.set_ylabel(primary_metric.replace("metric_", ""))
            ax.grid(True, alpha=0.25)
            fig.tight_layout()
            out = out_dir / "optimization_primary_metric.png"
            fig.savefig(out, dpi=180)
            plt.close(fig)
            generated["primary_metric_plot"] = str(out)

    if primary_metric and secondary_metric:
        x_ok: List[float] = []
        y_ok: List[float] = []
        c_ok: List[float] = []
        x_bad: List[float] = []
        y_bad: List[float] = []
        for row in eval_rows:
            x = _safe_float(row.get(primary_metric, float("nan")))
            y = _safe_float(row.get(secondary_metric, float("nan")))
            if not math.isfinite(x) or not math.isfinite(y):
                continue
            brightness = _safe_float(row.get("brightness", row.get("score", float("nan"))))
            status = str(row.get("status", ""))
            try:
                ok = int(row.get("constraints_ok", 0)) == 1
            except Exception:
                ok = False
            if status == "ok" and ok:
                x_ok.append(x)
                y_ok.append(y)
                c_ok.append(brightness if math.isfinite(brightness) else 0.0)
            else:
                x_bad.append(x)
                y_bad.append(y)
        if x_ok or x_bad:
            fig, ax = plt.subplots(figsize=(8.8, 5.6))
            if x_ok:
                scatter = ax.scatter(x_ok, y_ok, c=c_ok, cmap="viridis", alpha=0.85, label="feasible")
                fig.colorbar(scatter, ax=ax, label="brightness / score")
            if x_bad:
                ax.scatter(x_bad, y_bad, marker="x", color="#b91c1c", alpha=0.8, label="infeasible")
            ax.set_title("Rozrzut ocenionych konfiguracji")
            ax.set_xlabel(primary_metric.replace("metric_", ""))
            ax.set_ylabel(secondary_metric.replace("metric_", ""))
            ax.grid(True, alpha=0.25)
            ax.legend()
            fig.tight_layout()
            out = out_dir / "optimization_scatter.png"
            fig.savefig(out, dpi=180)
            plt.close(fig)
            generated["scatter_plot"] = str(out)

    if generated:
        summary["plots"] = {**(summary.get("plots") or {}), **generated}
        summary_path.write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")

    return generated
