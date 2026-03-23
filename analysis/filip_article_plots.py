#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import os
from pathlib import Path
import sys
from typing import Any, Iterable


ROOT = Path(__file__).resolve().parents[1]
OPT_DIR = ROOT / "data" / "optimization"
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

_mpl_cfg = ROOT / ".cache" / "matplotlib"
_mpl_cfg.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(_mpl_cfg))

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # type: ignore
import numpy as np  # type: ignore

from analysis.roofline_model import _cpu_peaks, _gpu_peaks, _pick_record
from fem_catalog import nshape as fem_nshape
from optimization.problems import FemParametricProblem, FemParametricProblemConfig


OPTION_FIELDS = [
    "coal_read",
    "coal_write",
    "compute_all_shape_fun_der",
    "use_workspace_for_pde_coeff",
    "use_workspace_for_geo_data",
    "use_workspace_for_shape_fun",
    "use_workspace_for_stiff_mat",
    "padding",
]
FILIP_OPTION_LABEL = "Options (9 bits, Filip order)"
VARIANT_ORDER = ["qss", "sqs", "ssq"]
PREFERRED_PLOT_ORDER = [
    "article_paper_option_times.png",
    "article_variant_option_times.png",
    "article_best_summary.png",
    "article_autotuning_overview.png",
    "article_memory_compute_breakdown.png",
    "article_backend_comparison.png",
]
PAPER_OPERATOR_PREFERENCE = [
    "laplace",
    "test",
    "diffusion",
    "diffusion_convection_mass",
    "diffusion_mass",
    "convection",
    "mass",
]


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _read_jsonl(path: Path) -> list[dict[str, Any]]:
    rows: list[dict[str, Any]] = []
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                rows.append(json.loads(line))
            except Exception:
                continue
    return rows


def _safe_float(value: Any) -> float:
    try:
        if value is None:
            return float("nan")
        x = float(value)
        return x
    except Exception:
        return float("nan")


def _safe_int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except Exception:
        return int(default)


def _is_finite_positive(value: float) -> bool:
    return math.isfinite(value) and value > 0.0


def _parse_backend_from_dir(path: Path) -> str:
    parts = path.name.split("__backend-")
    if len(parts) == 2:
        return parts[1]
    return "unknown"


def _operator_title(name: str) -> str:
    mapping = {
        "laplace": "Laplace",
        "test": "TEST benchmark",
        "diffusion": "Diffusion (Poisson-like)",
        "mass": "Mass",
        "convection": "Convection",
        "diffusion_mass": "Diffusion + Mass",
        "diffusion_convection_mass": "Convection-Diffusion-Mass",
    }
    return mapping.get(name, name.replace("_", " ").title())


def _variant_title(name: str) -> str:
    return str(name).upper()


def _combo_bits(cfg: dict[str, Any], option_row: Any | None = None) -> str:
    if isinstance(option_row, (list, tuple)) and len(option_row) >= 9:
        return "".join("1" if _safe_int(v, 0) != 0 else "0" for v in option_row[:9])

    bits = []
    for key in OPTION_FIELDS[:-1]:
        bits.append("1" if _safe_int(cfg.get(key), 0) != 0 else "0")

    padding = 1 if _safe_int(cfg.get("padding"), 0) != 0 else 0
    bits.append("1" if padding == 0 else "0")
    bits.append("1" if padding == 1 else "0")
    return "".join(bits)


def _combo_sort_key(bits: str) -> tuple[int, str]:
    try:
        return int(bits, 2), bits
    except Exception:
        return 0, bits


def _ns_per_unit(metrics: dict[str, Any]) -> float:
    elapsed = _safe_float(metrics.get("elapsed_s_mean"))
    n_elem = max(1.0, _safe_float(metrics.get("n_elements")))
    n_qp = max(1.0, _safe_float(metrics.get("n_qp_effective")) or _safe_float(metrics.get("n_qp_requested")) or 1.0)
    if not _is_finite_positive(elapsed):
        return float("nan")
    return elapsed * 1e9 / max(1.0, n_elem * n_qp)


def _row_label(row: dict[str, Any]) -> str:
    backend = str(row.get("backend", "unknown"))
    device = str(row.get("device", "")).strip()
    if device and device != "unknown":
        return f"{backend} | {device}"
    return backend


def _normalized_eval_row(raw: dict[str, Any], *, summary: dict[str, Any], out_dir: Path) -> dict[str, Any] | None:
    cfg = raw.get("config")
    if not isinstance(cfg, dict):
        cfg = {k[4:]: v for k, v in raw.items() if k.startswith("cfg_")}
    metrics = raw.get("metrics")
    if not isinstance(metrics, dict):
        metrics = {k[7:]: v for k, v in raw.items() if k.startswith("metric_")}
    artifacts = {k[9:]: v for k, v in raw.items() if k.startswith("artifact_")}

    if not cfg or not metrics:
        return None

    operator = str(cfg.get("operator", "")).strip().lower()
    variant = str(cfg.get("algorithm_variant", "")).strip().lower()
    gflops = _safe_float(metrics.get("gflops_mean"))
    gbps = _safe_float(metrics.get("gbps_mean"))
    elapsed = _safe_float(metrics.get("elapsed_s_mean"))
    ns_unit = _ns_per_unit(metrics)
    if not math.isfinite(ns_unit):
        return None

    backend = (
        str(artifacts.get("resolved_backend", "")).strip().lower()
        or str(summary.get("resolved_backend", "")).strip().lower()
        or str(summary.get("backend", "")).strip().lower()
        or _parse_backend_from_dir(out_dir)
    )
    device = (
        str(artifacts.get("device", "")).strip()
        or str(summary.get("device", "")).strip()
        or "unknown"
    )

    return {
        "status": str(raw.get("status", "")).strip().lower(),
        "constraints_ok": _safe_int(raw.get("constraints_ok", 0)) == 1,
        "operator": operator,
        "variant": variant,
        "combo_bits": _combo_bits(cfg, raw.get("option_row")),
        "option_index": _safe_int(raw.get("option_index"), -1),
        "option_row": list(raw.get("option_row", [])) if isinstance(raw.get("option_row"), list) else [],
        "backend": backend,
        "device": device,
        "label": _row_label({"backend": backend, "device": device}),
        "elapsed_s_mean": elapsed,
        "ns_per_unit": ns_unit,
        "gflops_mean": gflops,
        "gbps_mean": gbps,
        "config": dict(cfg),
        "metrics": dict(metrics),
    }


def _load_run(out_dir: Path) -> dict[str, Any]:
    summary = _read_json(out_dir / "summary.json")
    rows_raw = _read_jsonl(out_dir / "evaluations.jsonl")
    rows: list[dict[str, Any]] = []
    for raw in rows_raw:
        row = _normalized_eval_row(raw, summary=summary, out_dir=out_dir)
        if row is not None:
            rows.append(row)
    return {
        "out_dir": out_dir,
        "summary": summary,
        "rows": rows,
        "method": str(summary.get("method", "firefly")),
    }


def _best_ns_row(rows: Iterable[dict[str, Any]], *, operator: str) -> dict[str, Any] | None:
    best: dict[str, Any] | None = None
    for row in rows:
        if row.get("operator") != operator:
            continue
        ns_val = _safe_float(row.get("ns_per_unit"))
        if not math.isfinite(ns_val):
            continue
        if best is None or ns_val < _safe_float(best.get("ns_per_unit")):
            best = row
    return best


def _paper_operator_selection(rows: list[dict[str, Any]], max_operators: int) -> list[str]:
    source_rows = _preferred_rows(rows)
    available = {str(row.get("operator", "")) for row in source_rows if str(row.get("operator", ""))}
    selected: list[str] = []
    for operator in PAPER_OPERATOR_PREFERENCE:
        if operator in available and operator not in selected:
            selected.append(operator)
        if len(selected) >= max_operators:
            return selected

    ranked = _select_plot_operators(source_rows, max_operators=max(max_operators, len(available)))
    for operator in ranked:
        if operator in available and operator not in selected:
            selected.append(operator)
        if len(selected) >= max_operators:
            break
    return selected


def _paper_sweep_n_elements(base_cfg: dict[str, Any], backend: str) -> int:
    try:
        current = max(1, int(base_cfg.get("n_elements", 1)))
    except Exception:
        current = 1
    element_type = str(base_cfg.get("element_type", "tet4"))
    if backend == "cpu":
        if element_type == "hex8":
            cap = 16_000
        elif element_type == "prism6":
            cap = 20_000
        else:
            cap = 32_000
        floor = 8_000
    else:
        if element_type == "hex8":
            cap = 24_000
        elif element_type == "prism6":
            cap = 48_000
        else:
            cap = 64_000
        floor = 12_000
    return max(floor, min(current, cap))


def _paper_reference_configs(run: dict[str, Any], operators: list[str]) -> list[dict[str, Any]]:
    rows = _preferred_rows(run["rows"])
    summary = run["summary"]
    backend = (
        str(summary.get("resolved_backend", "")).strip().lower()
        or str(summary.get("backend", "")).strip().lower()
        or _parse_backend_from_dir(run["out_dir"])
    )
    refs: list[dict[str, Any]] = []
    for operator in operators:
        best = _best_ns_row(rows, operator=operator)
        if best is None:
            continue
        cfg = dict(best.get("config", {}))
        ref = {
            "n_elements": _paper_sweep_n_elements(cfg, backend),
            "n_qp": max(1, _safe_int(cfg.get("n_qp"), 1)),
            "element_type": str(cfg.get("element_type", "tet4")),
            "operator": operator,
            "dtype": str(cfg.get("dtype", "float32")),
            "workgroup_size": max(1, _safe_int(cfg.get("workgroup_size"), 1 if backend == "cpu" else 64)),
        }
        refs.append(ref)
    return refs


def _sweep_cache_paths(plots_dir: Path) -> tuple[Path, Path]:
    return plots_dir / "paper_sweep.jsonl", plots_dir / "paper_sweep_summary.json"


def _ref_signature(summary: dict[str, Any], refs: list[dict[str, Any]]) -> dict[str, Any]:
    return {
        "backend": str(summary.get("resolved_backend", "")).strip().lower() or str(summary.get("backend", "")).strip().lower(),
        "device": str(summary.get("device", "")).strip(),
        "references": refs,
    }


def _load_cached_paper_sweep(plots_dir: Path, *, summary: dict[str, Any], refs: list[dict[str, Any]]) -> list[dict[str, Any]] | None:
    rows_path, summary_path = _sweep_cache_paths(plots_dir)
    if not rows_path.exists() or not summary_path.exists():
        return None
    try:
        cached_summary = _read_json(summary_path)
        if cached_summary.get("signature") != _ref_signature(summary, refs):
            return None
        return _read_jsonl(rows_path)
    except Exception:
        return None


def _flags_from_bits(bits: int) -> dict[str, int]:
    out: dict[str, int] = {}
    for idx, key in enumerate(OPTION_FIELDS):
        out[key] = 1 if ((bits >> idx) & 1) else 0
    return out


def _paper_sweep_rows(run: dict[str, Any], refs: list[dict[str, Any]], plots_dir: Path) -> list[dict[str, Any]]:
    cached = _load_cached_paper_sweep(plots_dir, summary=run["summary"], refs=refs)
    if cached is not None:
        rows: list[dict[str, Any]] = []
        for raw in cached:
            row = _normalized_eval_row(raw, summary=run["summary"], out_dir=run["out_dir"])
            if row is not None:
                rows.append(row)
        if rows:
            return rows

    summary = run["summary"]
    backend = (
        str(summary.get("resolved_backend", "")).strip().lower()
        or str(summary.get("backend", "")).strip().lower()
        or _parse_backend_from_dir(run["out_dir"])
    )
    requested_backend = backend or "cpu"

    try:
        ref_element_types = sorted({str(ref.get("element_type", "tet4")) for ref in refs}) or ["tet4"]
        ref_operators = sorted({str(ref.get("operator", "diffusion")) for ref in refs}) or ["diffusion"]
        problem = FemParametricProblem(
            FemParametricProblemConfig(
                backend=requested_backend,
                device_index=0,
                repeats=1,
                execution_policy="native_only",
                n_elements_min=1,
                n_elements_max=1,
                n_qp_min=1,
                n_qp_max=1,
                element_types=ref_element_types,
                operators=ref_operators,
                dtypes=["float32"],
                algorithm_variants=list(VARIANT_ORDER),
                workgroup_sizes=[1, 32, 64, 128, 256, 512],
                use_workspace_for_pde_coeff_choices=[0, 1],
                use_workspace_for_geo_data_choices=[0, 1],
                use_workspace_for_shape_fun_choices=[0, 1],
                use_workspace_for_stiff_mat_choices=[0, 1],
                padding_choices=[0, 1],
                compute_all_shape_fun_der_choices=[0, 1],
                coal_read_choices=[0, 1],
                coal_write_choices=[0, 1],
                memory_budget_fraction=0.25,
                eval_cache_size=4096,
                screening_repeats=1,
                screening_prune_factor=0.0,
                record_raw_artifacts=False,
            )
        )
    except Exception:
        return []

    rows_out: list[dict[str, Any]] = []
    raw_rows: list[dict[str, Any]] = []
    try:
        for ref in refs:
            base = dict(ref)
            for variant in VARIANT_ORDER:
                for bits in range(1 << len(OPTION_FIELDS)):
                    cfg = dict(base)
                    cfg["algorithm_variant"] = variant
                    cfg.update(_flags_from_bits(bits))
                    res = problem.evaluate(cfg)
                    row = {
                        "status": res.status,
                        "constraints_ok": int(res.constraints_ok),
                        "config": cfg,
                        "metrics": res.metrics,
                        "artifact_resolved_backend": problem.mode.resolved_backend,
                        "artifact_execution_mode": problem.mode.execution_mode,
                        "artifact_device": problem.mode.device_name,
                    }
                    raw_rows.append(row)
                    normalized = _normalized_eval_row(row, summary=summary, out_dir=run["out_dir"])
                    if normalized is not None:
                        rows_out.append(normalized)
    finally:
        problem.close()

    rows_path, summary_path = _sweep_cache_paths(plots_dir)
    payload = {
        "signature": _ref_signature(summary, refs),
        "row_count": len(raw_rows),
    }
    with rows_path.open("w", encoding="utf-8") as f:
        for row in raw_rows:
            f.write(json.dumps(row, ensure_ascii=True) + "\n")
    summary_path.write_text(json.dumps(payload, indent=2, ensure_ascii=True), encoding="utf-8")
    return rows_out


def _select_plot_operators(rows: Iterable[dict[str, Any]], max_operators: int) -> list[str]:
    best_by_operator: dict[str, float] = {}
    count_by_operator: dict[str, int] = {}
    for row in rows:
        op = str(row.get("operator", ""))
        if not op:
            continue
        count_by_operator[op] = count_by_operator.get(op, 0) + 1
        g = _safe_float(row.get("gflops_mean"))
        if op not in best_by_operator or g > best_by_operator[op]:
            best_by_operator[op] = g
    ranked = sorted(
        count_by_operator.keys(),
        key=lambda op: (best_by_operator.get(op, float("-inf")), count_by_operator.get(op, 0), op),
        reverse=True,
    )
    return ranked[: max(1, max_operators)]


def _status_ok_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    return [row for row in rows if str(row.get("status", "")) == "ok" and math.isfinite(_safe_float(row.get("ns_per_unit")))]


def _preferred_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    ok_rows = _status_ok_rows(rows)
    feasible = [row for row in ok_rows if bool(row.get("constraints_ok"))]
    return feasible if feasible else ok_rows


def _best_by_combo(rows: Iterable[dict[str, Any]], operator: str, variant: str) -> tuple[list[str], list[float]]:
    best: dict[str, float] = {}
    order_map: dict[str, int] = {}
    for row in rows:
        if row.get("operator") != operator or row.get("variant") != variant:
            continue
        combo = str(row.get("combo_bits", ""))
        ns_val = _safe_float(row.get("ns_per_unit"))
        if not math.isfinite(ns_val):
            continue
        option_index = _safe_int(row.get("option_index"), -1)
        if option_index >= 0:
            prev_idx = order_map.get(combo)
            if prev_idx is None or option_index < prev_idx:
                order_map[combo] = option_index
        prev = best.get(combo)
        if prev is None or ns_val < prev:
            best[combo] = ns_val
    combos = sorted(
        best.keys(),
        key=lambda combo: (
            0 if combo in order_map else 1,
            order_map.get(combo, 10**9),
            _combo_sort_key(combo),
        ),
    )
    return combos, [best[c] for c in combos]


def _best_row(rows: Iterable[dict[str, Any]], *, operator: str, variant: str) -> dict[str, Any] | None:
    best: dict[str, Any] | None = None
    for row in rows:
        if row.get("operator") != operator or row.get("variant") != variant:
            continue
        g = _safe_float(row.get("gflops_mean"))
        if not math.isfinite(g):
            continue
        if best is None or g > _safe_float(best.get("gflops_mean")):
            best = row
    return best


def _estimate_rw_bytes(row: dict[str, Any]) -> tuple[float, float]:
    cfg = row.get("config", {})
    if not isinstance(cfg, dict):
        return 0.0, 0.0
    n_elem = max(1, _safe_int(cfg.get("n_elements"), 1))
    n_qp = max(1, _safe_int(cfg.get("n_qp"), 1))
    etype = str(cfg.get("element_type", "tet4"))
    dtype = str(cfg.get("dtype", "float32"))
    itemsize = 4 if dtype == "float32" else 8
    nshape = fem_nshape(etype)

    geo = float(n_elem * nshape * 3 * itemsize)
    stiff = float(n_elem * nshape * nshape * itemsize)
    shape = float(n_elem * n_qp * nshape * itemsize)
    coeff = float(n_elem * n_qp * itemsize)

    read_bytes = geo + shape + coeff
    write_bytes = stiff

    if _safe_int(cfg.get("use_workspace_for_pde_coeff"), 0):
        read_bytes += coeff
    if _safe_int(cfg.get("use_workspace_for_geo_data"), 0):
        read_bytes += geo
    if _safe_int(cfg.get("use_workspace_for_shape_fun"), 0):
        read_bytes += shape
    if _safe_int(cfg.get("use_workspace_for_stiff_mat"), 0):
        write_bytes += stiff
    if _safe_int(cfg.get("padding"), 0):
        read_bytes *= 1.05
        write_bytes *= 1.05
    return max(0.0, read_bytes), max(0.0, write_bytes)


def _roofline_peaks_for_backend(backend: str, rows: list[dict[str, Any]]) -> tuple[float, float]:
    try:
        cpu_records = _cpu_peaks(ROOT / "data" / "cpu")
        gpu_records = _gpu_peaks(ROOT / "data" / "gpu")
        if backend == "cpu":
            rec = _pick_record("cpu", cpu_records, gpu_records, backend="", model_contains="")
        else:
            rec = _pick_record("gpu", cpu_records, gpu_records, backend=backend, model_contains="")
        if rec is not None and _is_finite_positive(rec.peak_gflops) and _is_finite_positive(rec.peak_bw_gbps):
            return float(rec.peak_gflops), float(rec.peak_bw_gbps)
    except Exception:
        pass

    peak_gflops = max((_safe_float(row.get("gflops_mean")) for row in rows), default=float("nan"))
    peak_bw = max((_safe_float(row.get("gbps_mean")) for row in rows), default=float("nan"))
    if not _is_finite_positive(peak_gflops):
        peak_gflops = 1.0
    if not _is_finite_positive(peak_bw):
        peak_bw = 1.0
    return float(peak_gflops), float(peak_bw)


def _estimate_share(row: dict[str, Any], peak_gflops: float, peak_bw_gbps: float) -> tuple[float, float, float]:
    metrics = row.get("metrics", {})
    if not isinstance(metrics, dict):
        return 0.0, 0.0, 0.0
    elapsed = _safe_float(metrics.get("elapsed_s_mean"))
    gflops = _safe_float(metrics.get("gflops_mean"))
    gbps = _safe_float(metrics.get("gbps_mean"))
    if not (_is_finite_positive(elapsed) and _is_finite_positive(gflops) and _is_finite_positive(gbps)):
        return 0.0, 0.0, 0.0

    total_flops = gflops * 1e9 * elapsed
    read_bytes, write_bytes = _estimate_rw_bytes(row)
    total_bytes = max(1.0, gbps * 1e9 * elapsed)
    if read_bytes + write_bytes <= 0.0:
        read_bytes = total_bytes * 0.6
        write_bytes = total_bytes * 0.4
    else:
        scale = total_bytes / max(read_bytes + write_bytes, 1.0)
        read_bytes *= scale
        write_bytes *= scale

    t_compute = total_flops / max(peak_gflops * 1e9, 1e-12)
    t_read = read_bytes / max(peak_bw_gbps * 1e9, 1e-12)
    t_write = write_bytes / max(peak_bw_gbps * 1e9, 1e-12)
    total = t_compute + t_read + t_write
    if total <= 0.0:
        return 0.0, 0.0, 0.0
    return 100.0 * t_read / total, 100.0 * t_compute / total, 100.0 * t_write / total


def _latest_optimization_dir() -> Path | None:
    if not OPT_DIR.exists():
        return None
    runs = [p for p in OPT_DIR.iterdir() if p.is_dir() and (p / "summary.json").exists() and (p / "evaluations.jsonl").exists()]
    if not runs:
        return None
    return max(runs, key=lambda p: p.stat().st_mtime)


def _ensure_plots_dir(optimization_dir: Path) -> Path:
    preferred = optimization_dir / "plots"
    try:
        preferred.mkdir(parents=True, exist_ok=True)
    except Exception:
        pass

    if preferred.exists() and os.access(preferred, os.W_OK):
        blocked = False
        for name in [*PREFERRED_PLOT_ORDER, "article_plots_summary.json"]:
            candidate = preferred / name
            if candidate.exists() and not os.access(candidate, os.W_OK):
                blocked = True
                break
        if not blocked:
            probe = preferred / ".codex_write_probe"
            try:
                probe.write_text("ok", encoding="utf-8")
                probe.unlink(missing_ok=True)
                return preferred
            except Exception:
                pass

    fallback = ROOT / "analysis" / "plots" / f"{optimization_dir.name}__filip_article"
    fallback.mkdir(parents=True, exist_ok=True)
    return fallback


def _collect_sibling_runs(current: Path, method: str) -> list[dict[str, Any]]:
    if not OPT_DIR.exists():
        return []
    latest_by_label: dict[str, Path] = {}
    for p in OPT_DIR.iterdir():
        if not p.is_dir():
            continue
        summary_path = p / "summary.json"
        eval_path = p / "evaluations.jsonl"
        if not summary_path.exists() or not eval_path.exists():
            continue
        try:
            summary = _read_json(summary_path)
        except Exception:
            continue
        problem_name = str(summary.get("problem", ""))
        run_method = str(summary.get("method", "firefly"))
        if not problem_name.startswith("fem_parametric") or run_method != method:
            continue
        backend = str(summary.get("resolved_backend", "") or summary.get("backend", "") or _parse_backend_from_dir(p)).strip()
        device = str(summary.get("device", "")).strip()
        label = f"{backend} | {device}" if device else backend
        prev = latest_by_label.get(label)
        if prev is None or p.stat().st_mtime > prev.stat().st_mtime:
            latest_by_label[label] = p
    if current.name not in {p.name for p in latest_by_label.values()}:
        try:
            current_summary = _read_json(current / "summary.json")
            backend = str(current_summary.get("resolved_backend", "") or current_summary.get("backend", "") or _parse_backend_from_dir(current)).strip()
            device = str(current_summary.get("device", "")).strip()
            label = f"{backend} | {device}" if device else backend
        except Exception:
            label = _parse_backend_from_dir(current)
        latest_by_label[label] = current
    runs = []
    for p in sorted(latest_by_label.values(), key=lambda x: x.name):
        try:
            runs.append(_load_run(p))
        except Exception:
            continue
    return runs


def _setup_plot_style() -> None:
    plt.style.use("default")
    plt.rcParams.update(
        {
            "figure.facecolor": "white",
            "axes.facecolor": "white",
            "axes.grid": True,
            "grid.alpha": 0.25,
            "font.family": "serif",
            "axes.titlesize": 11,
            "axes.labelsize": 10,
            "legend.fontsize": 8,
        }
    )


def _plot_autotuning_overview(rows: list[dict[str, Any]], operators: list[str], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    if not source_rows:
        return False

    fig, axes = plt.subplots(len(operators), 1, figsize=(12, 3.5 * len(operators)), squeeze=False)
    color_map = {"qss": "#1d4ed8", "sqs": "#0f766e", "ssq": "#b45309"}
    plotted = False

    for ax, operator in zip(axes[:, 0], operators):
        subplot_vals: list[float] = []
        for variant in VARIANT_ORDER:
            combos, ys = _best_by_combo(source_rows, operator, variant)
            if not combos:
                continue
            xs = list(range(len(combos)))
            ax.plot(
                xs,
                ys,
                marker="o",
                linewidth=1.1,
                markersize=2.5,
                label=_variant_title(variant),
                color=color_map.get(variant),
            )
            best_idx = min(range(len(ys)), key=lambda idx: ys[idx])
            ax.scatter([best_idx], [ys[best_idx]], s=34, color=color_map.get(variant), zorder=4)
            ax.annotate(
                combos[best_idx],
                xy=(best_idx, ys[best_idx]),
                xytext=(4, -10),
                textcoords="offset points",
                fontsize=7,
                color=color_map.get(variant),
            )
            subplot_vals.extend(float(y) for y in ys if math.isfinite(float(y)))
            plotted = True
        ax.set_title(f"Automatic tuning results: {_operator_title(operator)}")
        ax.set_ylabel("ns / (element * qp)")
        ax.set_xlabel("Autotuning option combination index (sorted binary mask)")
        n_points = max((len(_best_by_combo(source_rows, operator, variant)[0]) for variant in VARIANT_ORDER), default=0)
        if n_points > 0:
            stride = max(1, n_points // 8)
            ax.set_xticks(list(range(0, n_points, stride)))
        if subplot_vals:
            robust_hi = float(np.percentile(np.array(subplot_vals, dtype=float), 97.0))
            max_hi = max(subplot_vals)
            if robust_hi > 0.0 and max_hi > robust_hi * 1.35:
                ax.set_ylim(0.0, robust_hi * 1.10)
                ax.text(
                    0.99,
                    0.96,
                    "high outliers clipped",
                    transform=ax.transAxes,
                    ha="right",
                    va="top",
                    fontsize=8,
                    color="#475569",
                )
        ax.legend()
        ax.grid(True, alpha=0.25)

    fig.tight_layout()
    fig.savefig(out_path, dpi=180)
    plt.close(fig)
    return plotted


def _apply_robust_ylim(ax: Any, values: Iterable[float]) -> None:
    finite_vals = [float(v) for v in values if math.isfinite(float(v))]
    if not finite_vals:
        return
    robust_hi = float(np.percentile(np.array(finite_vals, dtype=float), 97.0))
    max_hi = max(finite_vals)
    if robust_hi > 0.0 and max_hi > robust_hi * 1.35:
        ax.set_ylim(0.0, robust_hi * 1.10)
        ax.text(
            0.99,
            0.96,
            "high outliers clipped",
            transform=ax.transAxes,
            ha="right",
            va="top",
            fontsize=8,
            color="#475569",
        )


def _stacked_combo_label(bits: str) -> str:
    return "\n".join(list(str(bits)))


def _combo_fontsize(combos: list[str]) -> float:
    n_points = len(combos)
    if n_points >= 72:
        return 4.0
    if n_points >= 48:
        return 4.4
    if n_points >= 24:
        return 4.8
    return 5.4


def _run_series_label(run: dict[str, Any]) -> str:
    rows = _preferred_rows(run.get("rows", []))
    if rows:
        label = str(rows[0].get("label", "")).strip()
        if label:
            return label
    summary = run.get("summary", {})
    backend = str(summary.get("resolved_backend", "") or summary.get("backend", "")).strip()
    device = str(summary.get("device", "")).strip()
    if backend and device:
        return f"{backend} | {device}"
    return backend or str(run.get("out_dir", "run"))


def _paper_line_styles(n: int) -> list[dict[str, Any]]:
    if n <= 1:
        return [{"color": "#111111", "linestyle": "-", "linewidth": 1.6}]
    if n == 2:
        return [
            {"color": "#111111", "linestyle": (0, (1.2, 1.2)), "linewidth": 1.8},
            {"color": "#b8bcc2", "linestyle": "-", "linewidth": 1.8},
        ]
    palette = ["#111111", "#6b7280", "#b8bcc2", "#0f766e", "#b45309", "#7c3aed"]
    styles: list[dict[str, Any]] = []
    for idx in range(n):
        styles.append(
            {
                "color": palette[idx % len(palette)],
                "linestyle": "-" if idx % 2 else (0, (1.2, 1.2)),
                "linewidth": 1.5,
            }
        )
    return styles


def _combo_union_for_series(series_runs: list[dict[str, Any]], operator: str, variant: str) -> list[str]:
    combos: set[str] = set()
    order_map: dict[str, int] = {}
    for run in series_runs:
        run_rows = _status_ok_rows(run.get("rows", []))
        cur_combos, _ = _best_by_combo(run_rows, operator, variant)
        combos.update(cur_combos)
        for row in run_rows:
            if row.get("operator") != operator or row.get("variant") != variant:
                continue
            combo = str(row.get("combo_bits", ""))
            option_index = _safe_int(row.get("option_index"), -1)
            if option_index >= 0:
                prev_idx = order_map.get(combo)
                if prev_idx is None or option_index < prev_idx:
                    order_map[combo] = option_index
    return sorted(
        combos,
        key=lambda combo: (
            0 if combo in order_map else 1,
            order_map.get(combo, 10**9),
            _combo_sort_key(combo),
        ),
    )


def _aligned_series(
    series_runs: list[dict[str, Any]],
    operator: str,
    variant: str,
    combos: list[str],
) -> list[tuple[str, list[float]]]:
    payload: list[tuple[str, list[float]]] = []
    for run in series_runs:
        run_rows = _status_ok_rows(run.get("rows", []))
        cur_combos, ys = _best_by_combo(run_rows, operator, variant)
        if not cur_combos:
            continue
        mapping = {combo: val for combo, val in zip(cur_combos, ys)}
        payload.append((_run_series_label(run), [mapping.get(combo, float("nan")) for combo in combos]))
    return payload


def _plot_paper_like_series(
    ax: Any,
    *,
    combos: list[str],
    series_payload: list[tuple[str, list[float]]],
    title: str,
    show_xlabel: bool,
) -> bool:
    if not combos or not series_payload:
        ax.text(0.5, 0.5, "No data", transform=ax.transAxes, ha="center", va="center", fontsize=10)
        ax.set_axis_off()
        return False

    xs = list(range(len(combos)))
    styles = _paper_line_styles(len(series_payload))
    all_vals: list[float] = []
    for idx, (label, ys) in enumerate(series_payload):
        style = styles[min(idx, len(styles) - 1)]
        cleaned = [float(y) if math.isfinite(float(y)) else np.nan for y in ys]
        ax.plot(xs, cleaned, label=label, **style)
        all_vals.extend(float(y) for y in ys if math.isfinite(float(y)))

    ax.set_title(title)
    ax.set_ylabel("Time [ns / (element * qp)]")
    ax.set_xticks(xs, [_stacked_combo_label(combo) for combo in combos], fontsize=_combo_fontsize(combos))
    ax.tick_params(axis="x", length=0, pad=3)
    if show_xlabel:
        ax.set_xlabel(FILIP_OPTION_LABEL)
    ax.grid(True, axis="y", alpha=0.28)
    ax.set_xlim(-0.5, len(combos) - 0.5)
    _apply_robust_ylim(ax, all_vals)
    return True


def _paper_comparison_runs(current_run: dict[str, Any]) -> list[dict[str, Any]]:
    method = str(current_run.get("method", "")).strip() or str(current_run.get("summary", {}).get("method", "")).strip()
    current_out = Path(str(current_run.get("out_dir", "")))
    candidates = _collect_sibling_runs(current_out, method) if current_out.exists() else []
    if not candidates:
        return [current_run]
    current_backend = str(current_run.get("summary", {}).get("resolved_backend", "") or current_run.get("summary", {}).get("backend", "")).strip().lower()
    by_label: dict[str, dict[str, Any]] = {}
    for run in candidates:
        run_backend = str(run.get("summary", {}).get("resolved_backend", "") or run.get("summary", {}).get("backend", "")).strip().lower()
        if current_backend == "cpu":
            if run_backend != "cpu":
                continue
        elif run_backend == "cpu":
            continue
        label = _run_series_label(run)
        by_label[label] = run
    current_label = _run_series_label(current_run)
    by_label[current_label] = current_run
    return [by_label[key] for key in sorted(by_label.keys())]


def _plot_variant_series(
    ax: Any,
    *,
    combos: list[str],
    ys: list[float],
    variant: str,
    operator: str,
    show_xlabel: bool,
) -> bool:
    if not combos or not ys:
        ax.text(0.5, 0.5, "No data", transform=ax.transAxes, ha="center", va="center", fontsize=10)
        ax.set_axis_off()
        return False

    xs = list(range(len(combos)))
    color_map = {"qss": "#1d4ed8", "sqs": "#0f766e", "ssq": "#b45309"}
    color = color_map.get(variant, "#334155")
    ax.plot(xs, ys, linewidth=1.15, color=color, alpha=0.95)
    ax.scatter(xs, ys, s=16, color=color, alpha=0.9, zorder=3)

    best_idx = min(range(len(ys)), key=lambda idx: ys[idx])
    ax.scatter([best_idx], [ys[best_idx]], s=44, color="#dc2626", zorder=4)
    ax.annotate(
        f"best: {combos[best_idx]} ({ys[best_idx]:.2f} ns)",
        xy=(best_idx, ys[best_idx]),
        xytext=(8, -10),
        textcoords="offset points",
        fontsize=8,
        color="#991b1b",
    )

    ax.set_xticks(xs, [_stacked_combo_label(combo) for combo in combos], fontsize=_combo_fontsize(combos))
    ax.tick_params(axis="x", length=0, pad=3)
    ax.set_title(f"{_variant_title(variant)} option sweep: {_operator_title(operator)}")
    ax.set_ylabel("ns / (element * qp)")
    if show_xlabel:
        ax.set_xlabel(FILIP_OPTION_LABEL)
    ax.grid(True, alpha=0.25)
    ax.set_xlim(-0.5, len(combos) - 0.5)
    _apply_robust_ylim(ax, ys)
    return True


def _plot_variant_option_times(rows: list[dict[str, Any]], operators: list[str], variant: str, out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    payloads: list[tuple[str, list[str], list[float]]] = []
    for operator in operators:
        combos, ys = _best_by_combo(source_rows, operator, variant)
        if combos:
            payloads.append((operator, combos, ys))
    if not payloads:
        return False

    fig, axes = plt.subplots(len(payloads), 1, figsize=(20, 5.6 * len(payloads) + 1.2), squeeze=False)
    plotted = False
    for idx, (ax, payload) in enumerate(zip(axes[:, 0], payloads)):
        operator, combos, ys = payload
        plotted = _plot_variant_series(
            ax,
            combos=combos,
            ys=ys,
            variant=variant,
            operator=operator,
            show_xlabel=True,
        ) or plotted
    fig.subplots_adjust(left=0.055, right=0.995, top=0.96, bottom=0.07, hspace=0.52)
    fig.savefig(out_path, dpi=220)
    plt.close(fig)
    return plotted


def _plot_paper_variant_option_times(
    current_run: dict[str, Any],
    operators: list[str],
    variant: str,
    out_path: Path,
) -> bool:
    series_runs = _paper_comparison_runs(current_run)
    payloads: list[tuple[str, list[str], list[tuple[str, list[float]]]]] = []
    for operator in operators:
        combos = _combo_union_for_series(series_runs, operator, variant)
        aligned = _aligned_series(series_runs, operator, variant, combos)
        if combos and aligned:
            payloads.append((operator, combos, aligned))
    if not payloads:
        return False

    fig, axes = plt.subplots(len(payloads), 1, figsize=(22, 5.8 * len(payloads) + 1.8), squeeze=False, sharex=False)
    handles: list[Any] = []
    labels: list[str] = []
    for idx, (ax, payload) in enumerate(zip(axes[:, 0], payloads)):
        operator, combos, aligned = payload
        ok = _plot_paper_like_series(
            ax,
            combos=combos,
            series_payload=aligned,
            title=f"Automatic tuning results: {_operator_title(operator)} | {_variant_title(variant)}",
            show_xlabel=True,
        )
        if ok and not handles:
            handles, labels = ax.get_legend_handles_labels()
            if ax.legend_ is not None:
                ax.legend_.remove()
    if handles:
        fig.legend(handles, labels, loc="upper center", ncol=max(1, min(4, len(labels))), frameon=False, bbox_to_anchor=(0.5, 0.995))
    fig.subplots_adjust(left=0.05, right=0.995, top=0.95, bottom=0.06, hspace=0.56)
    fig.savefig(out_path, dpi=220)
    plt.close(fig)
    return True


def _plot_paper_option_times_overview(current_run: dict[str, Any], operators: list[str], out_path: Path) -> bool:
    series_runs = _paper_comparison_runs(current_run)
    payloads: list[tuple[str, str, list[str], list[tuple[str, list[float]]]]] = []
    for operator in operators:
        for variant in VARIANT_ORDER:
            combos = _combo_union_for_series(series_runs, operator, variant)
            aligned = _aligned_series(series_runs, operator, variant, combos)
            if combos and aligned:
                payloads.append((operator, variant, combos, aligned))
    if not payloads:
        return False

    fig, axes = plt.subplots(len(payloads), 1, figsize=(22, 4.9 * len(payloads) + 2.4), squeeze=False, sharex=False)
    handles: list[Any] = []
    labels: list[str] = []
    for idx, (ax, payload) in enumerate(zip(axes[:, 0], payloads)):
        operator, variant, combos, aligned = payload
        ok = _plot_paper_like_series(
            ax,
            combos=combos,
            series_payload=aligned,
            title=f"{_variant_title(variant)} | {_operator_title(operator)}",
            show_xlabel=True,
        )
        if ok and not handles:
            handles, labels = ax.get_legend_handles_labels()
            if ax.legend_ is not None:
                ax.legend_.remove()
    if handles:
        fig.legend(handles, labels, loc="upper center", ncol=max(1, min(4, len(labels))), frameon=False, bbox_to_anchor=(0.5, 0.995))
    fig.subplots_adjust(left=0.05, right=0.995, top=0.97, bottom=0.04, hspace=0.58)
    fig.savefig(out_path, dpi=220)
    plt.close(fig)
    return True


def _plot_variant_option_times_grid(rows: list[dict[str, Any]], operators: list[str], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    payloads: dict[tuple[str, str], tuple[list[str], list[float]]] = {}
    plotted = False
    for operator in operators:
        for variant in VARIANT_ORDER:
            combos, ys = _best_by_combo(source_rows, operator, variant)
            payloads[(operator, variant)] = (combos, ys)
            plotted = plotted or bool(combos)
    if not plotted:
        return False

    fig, axes = plt.subplots(
        len(operators),
        len(VARIANT_ORDER),
        figsize=(7.0 * len(VARIANT_ORDER), 5.8 * len(operators)),
        squeeze=False,
    )
    for row_idx, operator in enumerate(operators):
        for col_idx, variant in enumerate(VARIANT_ORDER):
            ax = axes[row_idx][col_idx]
            combos, ys = payloads.get((operator, variant), ([], []))
            ok = _plot_variant_series(
                ax,
                combos=combos,
                ys=ys,
                variant=variant,
                operator=operator,
                show_xlabel=True,
            )
            if ok and row_idx == 0:
                ax.text(
                    0.01,
                    1.08,
                    _variant_title(variant),
                    transform=ax.transAxes,
                    ha="left",
                    va="bottom",
                    fontsize=10,
                    fontweight="bold",
                    color="#0f172a",
                )
    fig.subplots_adjust(left=0.045, right=0.995, top=0.93, bottom=0.07, hspace=0.62, wspace=0.18)
    fig.savefig(out_path, dpi=220)
    plt.close(fig)
    return True


def _plot_best_summary(rows: list[dict[str, Any]], operators: list[str], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    if not source_rows:
        return False

    fig, ax = plt.subplots(figsize=(12, 5.5))
    x = np.arange(len(operators))
    width = 0.24
    plotted = False

    for idx, variant in enumerate(VARIANT_ORDER):
        vals = []
        for operator in operators:
            best = _best_row(source_rows, operator=operator, variant=variant)
            vals.append(_safe_float(best.get("ns_per_unit")) if best is not None else float("nan"))
        offs = x + (idx - (len(VARIANT_ORDER) - 1) / 2.0) * width
        bars = ax.bar(offs, vals, width=width, label=_variant_title(variant))
        for bar, val in zip(bars, vals):
            if math.isfinite(val):
                ax.text(
                    bar.get_x() + bar.get_width() / 2.0,
                    val,
                    f"{val:.2f}",
                    ha="center",
                    va="bottom",
                    fontsize=8,
                    rotation=90,
                )
        plotted = plotted or any(math.isfinite(v) and v > 0.0 for v in vals)

    ax.set_xticks(x, [_operator_title(op) for op in operators], rotation=12, ha="right")
    ax.set_ylabel("Best ns / (element * qp)")
    ax.set_title("Best auto-tuned execution time by operator and variant")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(out_path, dpi=180)
    plt.close(fig)
    return plotted


def _plot_memory_compute_breakdown(rows: list[dict[str, Any]], operators: list[str], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    if not source_rows:
        return False

    backend = str(source_rows[0].get("backend", "unknown"))
    peak_gflops, peak_bw = _roofline_peaks_for_backend(backend, source_rows)

    labels: list[str] = []
    read_vals: list[float] = []
    compute_vals: list[float] = []
    write_vals: list[float] = []

    for operator in operators:
        for variant in VARIANT_ORDER:
            best = _best_row(source_rows, operator=operator, variant=variant)
            if best is None:
                continue
            read_share, compute_share, write_share = _estimate_share(best, peak_gflops, peak_bw)
            labels.append(f"{_operator_title(operator)}\n{variant.upper()}")
            read_vals.append(read_share)
            compute_vals.append(compute_share)
            write_vals.append(write_share)

    if not labels:
        return False

    x = np.arange(len(labels))
    fig, ax = plt.subplots(figsize=(12, 5.8))
    ax.bar(x, read_vals, label="Read / transfer", color="#93c5fd")
    ax.bar(x, compute_vals, bottom=read_vals, label="Compute", color="#86efac")
    ax.bar(x, write_vals, bottom=np.array(read_vals) + np.array(compute_vals), label="Write", color="#fca5a5")
    ax.set_xticks(x, labels)
    ax.set_ylabel("Estimated share [%]")
    ax.set_title("Estimated computation vs memory-operation share")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.25)
    fig.tight_layout()
    fig.savefig(out_path, dpi=180)
    plt.close(fig)
    return True


def _plot_backend_comparison(current_run: dict[str, Any], sibling_runs: list[dict[str, Any]], operators: list[str], out_path: Path) -> bool:
    usable_runs = []
    for run in sibling_runs:
        rows = _preferred_rows(run.get("rows", []))
        if not rows:
            continue
        label = str(rows[0].get("label", "unknown")).strip() or "unknown"
        usable_runs.append({"label": label, "rows": rows})

    distinct_labels = sorted({str(run["label"]) for run in usable_runs})
    if len(distinct_labels) < 2:
        return False

    operator_payloads: list[tuple[str, list[str], dict[str, list[float]]]] = []
    for operator in operators:
        backend_labels: list[str] = []
        variant_values: dict[str, list[float]] = {variant: [] for variant in VARIANT_ORDER}
        present_backends = 0
        for run in usable_runs:
            label = str(run["label"])
            best_any = False
            values_for_backend: dict[str, float] = {}
            for variant in VARIANT_ORDER:
                best = _best_row(run["rows"], operator=operator, variant=variant)
                ns_val = _safe_float(best.get("ns_per_unit")) if best is not None else float("nan")
                if math.isfinite(ns_val):
                    best_any = True
                values_for_backend[variant] = ns_val
            if not best_any:
                continue
            present_backends += 1
            backend_labels.append(label)
            for variant in VARIANT_ORDER:
                variant_values[variant].append(values_for_backend[variant])
        if present_backends >= 2:
            operator_payloads.append((operator, backend_labels, variant_values))

    if not operator_payloads:
        return False

    fig, axes = plt.subplots(len(operator_payloads), 1, figsize=(12, 4.0 * len(operator_payloads)), squeeze=False)
    width = 0.24
    color_map = {"qss": "#1d4ed8", "sqs": "#0f766e", "ssq": "#b45309"}

    for ax, (operator, backend_labels, variant_values) in zip(axes[:, 0], operator_payloads):
        x = np.arange(len(backend_labels))
        for idx, variant in enumerate(VARIANT_ORDER):
            vals = variant_values[variant]
            offs = x + (idx - (len(VARIANT_ORDER) - 1) / 2.0) * width
            cleaned = [v if math.isfinite(v) else np.nan for v in vals]
            ax.bar(
                offs,
                cleaned,
                width=width,
                label=_variant_title(variant),
                color=color_map.get(variant),
                alpha=0.9,
            )
        ax.set_xticks(x, backend_labels, rotation=12, ha="right")
        ax.set_ylabel("Best ns / (element * qp)")
        ax.set_xlabel("Backend / device")
        ax.set_title(f"Best auto-tuned backend comparison: {_operator_title(operator)}")
        ax.legend()
        ax.grid(True, axis="y", alpha=0.25)

    fig.tight_layout()
    fig.savefig(out_path, dpi=180)
    plt.close(fig)
    return True


def generate_article_plots(optimization_dir: Path, *, max_operators: int = 2) -> dict[str, Any]:
    _setup_plot_style()

    run = _load_run(optimization_dir)
    plots_dir = _ensure_plots_dir(optimization_dir)
    operators = _paper_operator_selection(run["rows"], max_operators=max_operators)
    if not operators:
        operators = _select_plot_operators(_status_ok_rows(run["rows"]), max_operators=max_operators)
    method = str(run.get("method", "")).strip().lower()
    if method == "filip_original":
        paper_refs: list[dict[str, Any]] = []
        sweep_rows: list[dict[str, Any]] = []
        rows = run["rows"]
    else:
        paper_refs = _paper_reference_configs(run, operators)
        sweep_rows = _paper_sweep_rows(run, paper_refs, plots_dir) if paper_refs else []
        rows = sweep_rows or run["rows"]

    generated: list[str] = []
    paper_overview_path = plots_dir / "article_paper_option_times.png"
    if _plot_paper_option_times_overview(run, operators, paper_overview_path):
        generated.append(str(paper_overview_path))

    variant_grid_path = plots_dir / "article_variant_option_times.png"
    if _plot_variant_option_times_grid(rows, operators, variant_grid_path):
        generated.append(str(variant_grid_path))

    for variant in VARIANT_ORDER:
        variant_path = plots_dir / f"article_option_times_{variant}.png"
        if _plot_paper_variant_option_times(run, operators, variant, variant_path):
            generated.append(str(variant_path))

    best_path = plots_dir / "article_best_summary.png"
    if _plot_best_summary(rows, operators, best_path):
        generated.append(str(best_path))

    overview_path = plots_dir / "article_autotuning_overview.png"
    if _plot_autotuning_overview(rows, operators, overview_path):
        generated.append(str(overview_path))

    breakdown_path = plots_dir / "article_memory_compute_breakdown.png"
    if _plot_memory_compute_breakdown(rows, operators, breakdown_path):
        generated.append(str(breakdown_path))

    sibling_runs: list[dict[str, Any]] = []

    summary = {
        "optimization_dir": str(optimization_dir),
        "plots_dir": str(plots_dir),
        "operators": operators,
        "paper_references": paper_refs,
        "paper_sweep_rows": len(rows),
        "generated_plots": generated,
        "comparison_runs": [str(item["out_dir"]) for item in sibling_runs],
    }
    (plots_dir / "article_plots_summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
    return summary


def main() -> None:
    ap = argparse.ArgumentParser(description="Generate Filip article-style plots from fem_parametric optimization results.")
    ap.add_argument("--optimization-dir", default="", help="Path to a specific optimization run directory.")
    ap.add_argument("--mode", choices=["latest"], default="latest")
    ap.add_argument("--max-operators", type=int, default=2)
    args = ap.parse_args()

    out_dir = Path(args.optimization_dir).expanduser() if args.optimization_dir else None
    if out_dir is None:
        out_dir = _latest_optimization_dir()
    if out_dir is None or not out_dir.exists():
        raise SystemExit("No optimization directory found.")

    summary = generate_article_plots(out_dir, max_operators=max(1, int(args.max_operators)))
    print(json.dumps(summary, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
