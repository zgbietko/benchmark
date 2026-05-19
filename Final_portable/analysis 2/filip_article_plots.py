#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import math
import os
from pathlib import Path
import platform
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
from analysis.publication_style import operator_style, padded_ylim
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
OPTION_FIELD_LABELS = {
    "coal_read": "skojarz. odczyt",
    "coal_write": "skojarz. zapis",
    "compute_all_shape_fun_der": "wszystkie pochodne kszt.",
    "use_workspace_for_pde_coeff": "buf. PDE",
    "use_workspace_for_geo_data": "buf. geo",
    "use_workspace_for_shape_fun": "buf. kszt.",
    "use_workspace_for_stiff_mat": "buf. macierz",
    "padding": "padding",
}
FILIP_OPTION_LABEL = "Maska bitowa opcji autotuningu"
VARIANT_ORDER = ["qss", "sqs", "ssq"]
THESIS_CORE_PLOT_NAMES = [
    "filip_variant_qss.png",
    "filip_variant_sqs.png",
    "filip_variant_ssq.png",
    "filip_autotuning_trace.png",
    "filip_best_summary.png",
    "filip_memory_compute_breakdown.png",
]
APPENDIX_PLOT_NAMES = [
    "filip_best_configuration_card.png",
]
PREFERRED_PLOT_ORDER = [*THESIS_CORE_PLOT_NAMES, *APPENDIX_PLOT_NAMES]
PAPER_OPERATOR_PREFERENCE = [
    "laplace",
    "test",
    "diffusion",
    "diffusion_convection_mass",
    "diffusion_mass",
    "convection",
    "mass",
]
_CURRENT_PLATFORM_LABEL = ""


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _summary_method(summary: dict[str, Any]) -> str:
    method = str(summary.get("method", "")).strip()
    if method:
        return method
    workflow = str(summary.get("workflow", "")).strip().lower()
    filip_mode = str(summary.get("filip_mode", "")).strip().lower()
    if workflow == "filip_original" or filip_mode == "exact_reference":
        return "filip_original"
    if workflow == "filip_autotune":
        return "random_search"
    if workflow == "filip_firefly":
        return "firefly"
    return "unknown"


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
        "test": "benchmark TEST",
        "diffusion": "Dyfuzja (typu Poissona)",
        "mass": "Masa",
        "convection": "Konwekcja",
        "diffusion_mass": "Dyfuzja + masa",
        "diffusion_convection_mass": "Konwekcja-dyfuzja-masa",
    }
    return mapping.get(name, name.replace("_", " ").title())


def _variant_title(name: str) -> str:
    return str(name).upper()


def _slugify(text: str) -> str:
    raw = "".join(ch.lower() if ch.isalnum() else "_" for ch in str(text))
    while "__" in raw:
        raw = raw.replace("__", "_")
    return raw.strip("_") or "unknown"


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
    raw_phase = raw.get("raw_phase")
    if not isinstance(raw_phase, dict):
        raw_phase = {}

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
        "internal_ns_per_elem": _safe_float(raw_phase.get("internal_ns_per_elem")),
        "kernel_ns_per_unit": _safe_float(raw_phase.get("kernel_ns_per_unit")),
        "kernel_time_s": _safe_float(raw_phase.get("kernel_time_s")),
        "internal_time_s": _safe_float(raw_phase.get("internal_time_s")),
        "input_time_s": _safe_float(raw_phase.get("input_time_s")),
        "output_time_s": _safe_float(raw_phase.get("output_time_s")),
        "gflops_mean": gflops,
        "gbps_mean": gbps,
        "iteration": _safe_int(raw.get("iteration"), -1),
        "trial": _safe_int(raw.get("trial"), -1),
        "firefly_id": _safe_int(raw.get("firefly_id"), -1),
        "brightness": _safe_float(raw.get("brightness")),
        "score": _safe_float(raw.get("score")),
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
        "method": _summary_method(summary),
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


def _all_plot_operators(rows: Iterable[dict[str, Any]]) -> list[str]:
    source_rows = list(rows)
    available = {str(row.get("operator", "")).strip() for row in source_rows if str(row.get("operator", "")).strip()}
    if not available:
        return []
    preferred: list[str] = []
    for operator in PAPER_OPERATOR_PREFERENCE:
        if operator in available and operator not in preferred:
            preferred.append(operator)
    ranked = _select_plot_operators(source_rows, max_operators=max(1, len(available)))
    for operator in ranked:
        if operator in available and operator not in preferred:
            preferred.append(operator)
    tail = sorted(available - set(preferred))
    return preferred + tail


def _status_ok_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    return [row for row in rows if str(row.get("status", "")) == "ok" and math.isfinite(_safe_float(row.get("ns_per_unit")))]


def _preferred_rows(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    ok_rows = _status_ok_rows(rows)
    feasible = [row for row in ok_rows if bool(row.get("constraints_ok"))]
    return feasible if feasible else ok_rows


def _variant_landscape_rows(rows: Iterable[dict[str, Any]], *, include_infeasible: bool) -> list[dict[str, Any]]:
    ok_rows = _status_ok_rows(rows)
    if include_infeasible:
        return ok_rows
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


def _row_time_ns(row: dict[str, Any]) -> float:
    for key in ("internal_ns_per_elem", "kernel_ns_per_unit", "ns_per_unit"):
        value = _safe_float(row.get(key))
        if math.isfinite(value) and value > 0.0:
            return value
    metrics = row.get("metrics", {})
    if isinstance(metrics, dict):
        for key in ("internal_ns_per_elem", "kernel_ns_per_unit", "ns_per_unit"):
            value = _safe_float(metrics.get(key))
            if math.isfinite(value) and value > 0.0:
                return value
    return float("nan")


def _best_time_by_combo(rows: Iterable[dict[str, Any]], operator: str, variant: str) -> tuple[list[str], list[float], list[dict[str, Any]]]:
    best: dict[str, dict[str, Any]] = {}
    order_map: dict[str, int] = {}
    for row in rows:
        if row.get("operator") != operator or row.get("variant") != variant:
            continue
        combo = str(row.get("combo_bits", ""))
        time_ns = _row_time_ns(row)
        if not math.isfinite(time_ns):
            continue
        option_index = _safe_int(row.get("option_index"), -1)
        if option_index >= 0:
            prev_idx = order_map.get(combo)
            if prev_idx is None or option_index < prev_idx:
                order_map[combo] = option_index
        prev = best.get(combo)
        if prev is None or time_ns < _row_time_ns(prev):
            best[combo] = row
    combos = sorted(
        best.keys(),
        key=lambda combo: (
            0 if combo in order_map else 1,
            order_map.get(combo, 10**9),
            _combo_sort_key(combo),
        ),
    )
    rows_out = [best[c] for c in combos]
    return combos, [_row_time_ns(row) for row in rows_out], rows_out


def _option_settings_text(cfg: dict[str, Any]) -> str:
    enabled = [OPTION_FIELD_LABELS.get(key, key) for key in OPTION_FIELDS if _safe_int(cfg.get(key), 0)]
    if not enabled:
        return "no option flags enabled"
    return ", ".join(enabled)


def _row_settings_label(row: dict[str, Any]) -> str:
    cfg = row.get("config", {})
    if not isinstance(cfg, dict):
        cfg = {}
    variant = str(row.get("variant") or cfg.get("algorithm_variant") or "").upper()
    operator = str(row.get("operator") or cfg.get("operator") or "")
    bits = str(row.get("combo_bits") or _combo_bits(cfg))
    return f"{variant} | {operator} | {bits}"


def _unique_rows_by_config(rows: Iterable[dict[str, Any]]) -> list[dict[str, Any]]:
    out: list[dict[str, Any]] = []
    seen: set[str] = set()
    for row in rows:
        cfg = row.get("config", {})
        key = json.dumps(cfg, sort_keys=True, ensure_ascii=True, default=str)
        if key in seen:
            continue
        out.append(row)
        seen.add(key)
    return out


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
    preferred = optimization_dir / "figures" / "thesis_core"
    preferred.mkdir(parents=True, exist_ok=True)
    return preferred


def _ensure_appendix_dir(optimization_dir: Path) -> Path:
    appendix = optimization_dir / "figures" / "appendix"
    appendix.mkdir(parents=True, exist_ok=True)
    return appendix


def _ensure_manifest_dir(optimization_dir: Path) -> Path:
    manifest_dir = optimization_dir / "figures" / "manifests"
    manifest_dir.mkdir(parents=True, exist_ok=True)
    return manifest_dir


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
        run_method = _summary_method(summary)
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
    _save_annotated_figure(fig, out_path, dpi=180)
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


def _platform_label_for_run(run: dict[str, Any]) -> str:
    summary = run.get("summary", {}) or {}
    backend = str(summary.get("resolved_backend", "") or summary.get("backend", "")).strip()
    device = str(summary.get("device", "")).strip()
    case_label = str(summary.get("benchmark_case_label", "")).strip() or str(summary.get("benchmark_case", "")).strip()
    provenance = summary.get("provenance", {}) if isinstance(summary.get("provenance"), dict) else {}
    platform_info = provenance.get("platform", {}) if isinstance(provenance.get("platform"), dict) else {}
    system_name = str(platform_info.get("system", "")).strip() or platform.system().strip()
    machine = str(platform_info.get("machine", "")).strip() or platform.machine().strip()
    parts: list[str] = []
    if device:
        parts.append(device)
    if backend:
        parts.append(f"backend: {backend}")
    if system_name:
        parts.append(f"system: {system_name}")
    if machine:
        parts.append(f"architektura: {machine}")
    if case_label:
        parts.append(f"przypadek: {case_label}")
    return f"Platforma testowa: {' | '.join(parts)}" if parts else ""


def _annotate_platform(fig, platform_label: str | None = None) -> None:
    label = str(platform_label or _CURRENT_PLATFORM_LABEL or "").strip()
    if not label:
        return
    fig.text(
        0.995,
        0.995,
        label,
        ha="right",
        va="top",
        fontsize=8.4,
        color="#475569",
        bbox={
            "boxstyle": "round,pad=0.24",
            "facecolor": "white",
            "edgecolor": "#cbd5e1",
            "alpha": 0.92,
        },
    )


def _save_annotated_figure(fig, out_path: Path, *, dpi: int, platform_label: str | None = None) -> None:
    _annotate_platform(fig, platform_label=platform_label)
    fig.savefig(out_path, dpi=dpi)


def _paper_line_styles(n: int) -> list[dict[str, Any]]:
    if n <= 1:
        return [{"color": "#111111", "linestyle": "-", "linewidth": 2.1}]
    if n == 2:
        return [
            {"color": "#111111", "linestyle": (0, (1.2, 1.2)), "linewidth": 2.2},
            {"color": "#b8bcc2", "linestyle": "-", "linewidth": 2.2},
        ]
    palette = ["#111111", "#6b7280", "#b8bcc2", "#0f766e", "#b45309", "#7c3aed"]
    styles: list[dict[str, Any]] = []
    for idx in range(n):
        styles.append(
            {
                "color": palette[idx % len(palette)],
                "linestyle": "-" if idx % 2 else (0, (1.2, 1.2)),
                "linewidth": 1.8,
            }
        )
    return styles


def _article_axis_style(ax: Any) -> None:
    ax.set_facecolor("#ffffff")
    ax.grid(True, axis="y", color="#d4d4d4", linewidth=0.5, alpha=0.9)
    ax.grid(False, axis="x")
    for spine in ax.spines.values():
        spine.set_color("#8f8f8f")
        spine.set_linewidth(0.8)
    ax.tick_params(axis="y", labelsize=8, colors="#222222")
    ax.tick_params(axis="x", length=0, pad=3, colors="#333333")


def _set_combo_ticklabels(ax: Any, combos: list[str]) -> None:
    xs = list(range(len(combos)))
    ax.set_xticks(xs, [_stacked_combo_label(combo) for combo in combos], fontsize=_combo_fontsize(combos))
    for lbl in ax.get_xticklabels():
        lbl.set_fontfamily("monospace")
        lbl.set_color("#333333")


def _article_ylim(values: Iterable[float]) -> tuple[float, float] | None:
    finite_vals = sorted(float(v) for v in values if math.isfinite(float(v)))
    if not finite_vals:
        return None
    lo = finite_vals[0]
    hi = finite_vals[-1]
    if len(finite_vals) >= 8:
        robust_hi = float(np.percentile(np.array(finite_vals, dtype=float), 98.0))
        if hi > robust_hi * 1.35:
            hi = robust_hi
    span = max(hi - lo, max(abs(lo), abs(hi), 1.0) * 0.08, 1e-6)
    pad = span * 0.08
    lower = lo - pad
    upper = hi + pad
    if lo >= 0.0:
        lower = max(0.0, lower)
    if lower >= upper:
        upper = lower + 1.0
    return lower, upper


def _combo_union_for_operators(rows: Iterable[dict[str, Any]], operators: list[str], variant: str) -> list[str]:
    order_map: dict[str, int] = {}
    combos: set[str] = set()
    for row in rows:
        if row.get("variant") != variant or row.get("operator") not in operators:
            continue
        combo = str(row.get("combo_bits", "")).strip()
        if not combo:
            continue
        combos.add(combo)
        option_index = _safe_int(row.get("option_index"), -1)
        if option_index >= 0:
            prev = order_map.get(combo)
            if prev is None or option_index < prev:
                order_map[combo] = option_index
    return sorted(
        combos,
        key=lambda combo: (
            0 if combo in order_map else 1,
            order_map.get(combo, 10**9),
            _combo_sort_key(combo),
        ),
    )


def _aligned_operator_times(
    rows: Iterable[dict[str, Any]],
    *,
    operators: list[str],
    variant: str,
    combos: list[str],
) -> list[tuple[str, list[float]]]:
    payload: list[tuple[str, list[float]]] = []
    for operator in operators:
        cur_combos, cur_times, _ = _best_time_by_combo(rows, operator, variant)
        if not cur_combos:
            continue
        mapping = {combo: value for combo, value in zip(cur_combos, cur_times)}
        payload.append((operator, [mapping.get(combo, float("nan")) for combo in combos]))
    return payload


def _plot_compact_variant_figure(
    rows: list[dict[str, Any]],
    *,
    operators: list[str],
    variant: str,
    out_path: Path,
    include_infeasible: bool = False,
) -> bool:
    source_rows = _variant_landscape_rows(rows, include_infeasible=include_infeasible)
    combos = _combo_union_for_operators(source_rows, operators, variant)
    series = _aligned_operator_times(source_rows, operators=operators, variant=variant, combos=combos)
    if not combos or not series:
        return False

    width = max(15.5, min(30.0, 0.34 * len(combos)))
    height = 6.3 if len(combos) >= 48 else 5.8
    fig, ax = plt.subplots(figsize=(width, height))
    xs = list(range(len(combos)))
    all_vals: list[float] = []

    for operator, ys in series:
        style = operator_style(operator)
        clean = [float(y) if math.isfinite(float(y)) else np.nan for y in ys]
        ax.plot(xs, clean, label=_operator_title(operator), marker="o", markersize=2.7, **style)
        finite_pts = [(idx, float(val)) for idx, val in enumerate(ys) if math.isfinite(float(val))]
        all_vals.extend(val for _, val in finite_pts)
        if finite_pts:
            best_idx, best_val = min(finite_pts, key=lambda item: item[1])
            ax.scatter([best_idx], [best_val], s=32, color=style.get("color", "#111111"), zorder=4)

    _article_axis_style(ax)
    _set_combo_ticklabels(ax, combos)
    ax.set_xlim(-0.5, len(combos) - 0.5)
    ax.set_xlabel(FILIP_OPTION_LABEL, fontsize=10, labelpad=12)
    ax.set_ylabel("Czas [ns]", fontsize=10)
    if include_infeasible:
        ax.set_title(
            f"Wariant {_variant_title(variant)}: pełny przegląd {len(combos)} kombinacji czasu",
            fontsize=12,
            pad=6,
        )
    else:
        ax.set_title(f"Wariant {_variant_title(variant)}: wszystkie kombinacje autotuningu", fontsize=12, pad=6)
    bounds = padded_ylim(all_vals, lower_floor_zero=True, pad_fraction=0.08)
    if bounds is not None:
        ax.set_ylim(*bounds)
    ax.legend(frameon=False, ncol=min(2, max(1, len(series))), loc="upper left")
    if include_infeasible:
        fig.text(
            0.012,
            0.015,
            "Figura obejmuje wszystkie zakończone pomiary czasu (status=ok), a nie tylko kombinacje z constraints_ok.",
            fontsize=8.3,
            color="#475569",
        )
    fig.tight_layout(rect=(0.0, 0.03 if include_infeasible else 0.0, 1.0, 1.0))
    _save_annotated_figure(fig, out_path, dpi=220)
    plt.close(fig)
    return True


def _plot_compat_variant_aliases(rows: list[dict[str, Any]], operators: list[str], variant: str, out_path: Path) -> bool:
    return _plot_compact_variant_figure(rows, operators=operators, variant=variant, out_path=out_path)


def _write_article_manifest(
    optimization_dir: Path,
    *,
    plots_dir: Path,
    appendix_dir: Path,
    generated_core: list[str],
    generated_appendix: list[str],
    operators: list[str],
    mode: str,
    selected_rows_count: int,
) -> dict[str, Any]:
    manifest_dir = _ensure_manifest_dir(optimization_dir)
    payload = {
        "figure_set": "filip-thesis-core-v1",
        "mode": mode,
        "optimization_dir": str(optimization_dir),
        "thesis_core_dir": str(plots_dir),
        "appendix_dir": str(appendix_dir),
        "selected_operators": operators,
        "selected_rows_count": selected_rows_count,
        "figure_paths": generated_core,
        "appendix_figure_paths": generated_appendix,
    }
    manifest_path = manifest_dir / "filip_figures_manifest.json"
    manifest_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    compat_manifest = manifest_dir / "article_plots_summary.json"
    compat_manifest.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    compat_path = plots_dir / "article_plots_summary.json"
    compat_path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    core_manifest = plots_dir / "filip_figures_manifest.json"
    core_manifest.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    payload["plots_dir"] = str(plots_dir)
    payload["generated_plots"] = list(generated_core)
    payload["appendix_plots"] = list(generated_appendix)
    payload["manifest_path"] = str(manifest_path)
    return payload


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
    ylim: tuple[float, float] | None = None,
) -> bool:
    if not combos or not series_payload:
        ax.text(0.5, 0.5, "Brak danych", transform=ax.transAxes, ha="center", va="center", fontsize=10)
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

    _article_axis_style(ax)
    ax.set_title(title, fontsize=11, pad=5)
    ax.set_ylabel("Czas [ns / jednostkę]", fontsize=10)
    _set_combo_ticklabels(ax, combos)
    if show_xlabel:
        ax.set_xlabel(FILIP_OPTION_LABEL, fontsize=10, labelpad=12)
    ax.set_xlim(-0.5, len(combos) - 0.5)
    y_bounds = ylim or _article_ylim(all_vals)
    if y_bounds is not None:
        ax.set_ylim(*y_bounds)
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
    ylim: tuple[float, float] | None = None,
) -> bool:
    if not combos or not ys:
        ax.text(0.5, 0.5, "Brak danych", transform=ax.transAxes, ha="center", va="center", fontsize=10)
        ax.set_axis_off()
        return False

    xs = list(range(len(combos)))
    ax.plot(xs, ys, linewidth=1.35, color="#2b2b2b", alpha=0.97)

    _article_axis_style(ax)
    _set_combo_ticklabels(ax, combos)
    ax.set_title(f"{_variant_title(variant)} | {_operator_title(operator)}", fontsize=10, pad=3)
    ax.set_ylabel("Czas [ns / (element * qp)]", fontsize=9)
    if show_xlabel:
        ax.set_xlabel(FILIP_OPTION_LABEL, fontsize=9, labelpad=10)
    ax.set_xlim(-0.5, len(combos) - 0.5)
    y_bounds = ylim or _article_ylim(ys)
    if y_bounds is not None:
        ax.set_ylim(*y_bounds)
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

    shared_ylim = _article_ylim([val for _, _, ys in payloads for val in ys])
    fig, axes = plt.subplots(len(payloads), 1, figsize=(24, 4.9 * len(payloads) + 1.0), squeeze=False, sharex=False)
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
            ylim=shared_ylim,
        ) or plotted
    fig.subplots_adjust(left=0.05, right=0.995, top=0.96, bottom=0.06, hspace=0.44)
    _save_annotated_figure(fig, out_path, dpi=220)
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

    shared_ylim = _article_ylim(
        [val for _, _, aligned in payloads for _, ys in aligned for val in ys]
    )
    fig, axes = plt.subplots(len(payloads), 1, figsize=(24, 4.9 * len(payloads) + 1.1), squeeze=False, sharex=False)
    handles: list[Any] = []
    labels: list[str] = []
    for idx, (ax, payload) in enumerate(zip(axes[:, 0], payloads)):
        operator, combos, aligned = payload
        ok = _plot_paper_like_series(
            ax,
            combos=combos,
            series_payload=aligned,
            title=f"{_variant_title(variant)} | {_operator_title(operator)}",
            show_xlabel=True,
            ylim=shared_ylim,
        )
        if ok and not handles:
            handles, labels = ax.get_legend_handles_labels()
            if ax.legend_ is not None:
                ax.legend_.remove()
    if handles and len(labels) > 1:
        fig.legend(handles, labels, loc="upper center", ncol=max(1, min(4, len(labels))), frameon=False, bbox_to_anchor=(0.5, 0.995))
    fig.subplots_adjust(left=0.05, right=0.995, top=0.95, bottom=0.06, hspace=0.44)
    _save_annotated_figure(fig, out_path, dpi=220)
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

    shared_ylim = _article_ylim(
        [val for _, _, _, aligned in payloads for _, ys in aligned for val in ys]
    )
    fig, axes = plt.subplots(len(payloads), 1, figsize=(24, 4.5 * len(payloads) + 1.4), squeeze=False, sharex=False)
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
            ylim=shared_ylim,
        )
        if ok and not handles:
            handles, labels = ax.get_legend_handles_labels()
            if ax.legend_ is not None:
                ax.legend_.remove()
    if handles and len(labels) > 1:
        fig.legend(handles, labels, loc="upper center", ncol=max(1, min(4, len(labels))), frameon=False, bbox_to_anchor=(0.5, 0.995))
    fig.subplots_adjust(left=0.05, right=0.995, top=0.97, bottom=0.04, hspace=0.42)
    _save_annotated_figure(fig, out_path, dpi=220)
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

    shared_ylim = _article_ylim([val for _, ys in payloads.values() for val in ys])
    fig, axes = plt.subplots(
        len(operators),
        len(VARIANT_ORDER),
        figsize=(8.0 * len(VARIANT_ORDER), 4.9 * len(operators)),
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
                ylim=shared_ylim,
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
    fig.subplots_adjust(left=0.045, right=0.995, top=0.93, bottom=0.06, hspace=0.44, wspace=0.14)
    _save_annotated_figure(fig, out_path, dpi=220)
    plt.close(fig)
    return True


def _plot_operator_all_variants(rows: list[dict[str, Any]], operator: str, out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    payloads: list[tuple[str, list[str], list[float]]] = []
    for variant in VARIANT_ORDER:
        combos, ys = _best_by_combo(source_rows, operator, variant)
        if combos:
            payloads.append((variant, combos, ys))
    if not payloads:
        return False

    shared_ylim = _article_ylim([val for _variant, _combos, ys in payloads for val in ys])
    fig, axes = plt.subplots(len(payloads), 1, figsize=(24, 4.8 * len(payloads) + 1.0), squeeze=False, sharex=False)
    plotted = False
    for ax, payload in zip(axes[:, 0], payloads):
        variant, combos, ys = payload
        plotted = _plot_variant_series(
            ax,
            combos=combos,
            ys=ys,
            variant=variant,
            operator=operator,
            show_xlabel=True,
            ylim=shared_ylim,
        ) or plotted
    fig.suptitle(f"{_operator_title(operator)} | wszystkie warianty", fontsize=14, y=0.995, color="#0f172a")
    fig.subplots_adjust(left=0.05, right=0.995, top=0.94, bottom=0.06, hspace=0.44)
    _save_annotated_figure(fig, out_path, dpi=220)
    plt.close(fig)
    return plotted


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
    ax.set_ylabel("Najlepszy czas [ns / (element * qp)]")
    ax.set_title("Najlepszy czas wykonania po operatorze i wariancie")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.25)
    fig.tight_layout()
    _save_annotated_figure(fig, out_path, dpi=180)
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
    ax.bar(x, read_vals, label="Odczyt / transfer", color="#93c5fd")
    ax.bar(x, compute_vals, bottom=read_vals, label="Obliczenia", color="#86efac")
    ax.bar(x, write_vals, bottom=np.array(read_vals) + np.array(compute_vals), label="Zapis", color="#fca5a5")
    ax.set_xticks(x, labels)
    ax.set_ylabel("Szacowany udział [%]")
    ax.set_title("Szacowany udział obliczeń i operacji pamięciowych")
    ax.legend()
    ax.grid(True, axis="y", alpha=0.25)
    fig.tight_layout()
    _save_annotated_figure(fig, out_path, dpi=180)
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
        ax.set_ylabel("Najlepszy czas [ns / (element * qp)]")
        ax.set_xlabel("Backend / urządzenie")
        ax.set_title(f"Porównanie backendów po autotuningu: {_operator_title(operator)}")
        ax.legend()
        ax.grid(True, axis="y", alpha=0.25)

    fig.tight_layout()
    _save_annotated_figure(fig, out_path, dpi=180)
    plt.close(fig)
    return True


def _plot_filip_execution_time_by_option(rows: list[dict[str, Any]], operators: list[str], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    if not source_rows:
        return False

    payloads: list[tuple[str, str, list[str], list[float], list[dict[str, Any]]]] = []
    for operator in operators:
        for variant in VARIANT_ORDER:
            combos, ys, best_rows = _best_time_by_combo(source_rows, operator, variant)
            if combos:
                payloads.append((operator, variant, combos, ys, best_rows))
    if not payloads:
        return False

    shared_ylim = _article_ylim([val for _, _, _, ys, _ in payloads for val in ys])
    fig, axes = plt.subplots(len(payloads), 1, figsize=(24, 4.7 * len(payloads) + 1.2), squeeze=False)
    plotted = False
    color_map = {"qss": "#1d4ed8", "sqs": "#0f766e", "ssq": "#b45309"}
    for ax, (operator, variant, combos, ys, best_rows) in zip(axes[:, 0], payloads):
        xs = list(range(len(combos)))
        ax.plot(xs, ys, marker="o", markersize=3.0, linewidth=1.35, color=color_map.get(variant, "#111111"))
        if ys:
            best_idx = min(range(len(ys)), key=lambda idx: ys[idx])
            best_row = best_rows[best_idx]
            best_cfg = best_row.get("config", {}) if isinstance(best_row.get("config"), dict) else {}
            ax.scatter([best_idx], [ys[best_idx]], s=58, color="#dc2626", zorder=5, label="best")
            ax.annotate(
                f"best: {ys[best_idx]:.3g} ns\n{_option_settings_text(best_cfg)}",
                xy=(best_idx, ys[best_idx]),
                xytext=(8, 12),
                textcoords="offset points",
                fontsize=7,
                color="#111827",
                bbox={"boxstyle": "round,pad=0.25", "fc": "#ffffff", "ec": "#d1d5db", "alpha": 0.92},
            )
        _article_axis_style(ax)
        _set_combo_ticklabels(ax, combos)
        ax.set_xlim(-0.5, len(combos) - 0.5)
        if shared_ylim is not None:
            ax.set_ylim(*shared_ylim)
        ax.set_title(f"Czas wykonania kodu Filipa: {_variant_title(variant)} | {_operator_title(operator)}", fontsize=10, pad=3)
        ax.set_ylabel("Czas [ns / element]")
        ax.set_xlabel("Maska bitowa opcji autotuningu")
        plotted = True

    fig.subplots_adjust(left=0.05, right=0.995, top=0.965, bottom=0.045, hspace=0.48)
    _save_annotated_figure(fig, out_path, dpi=220)
    plt.close(fig)
    return plotted


def _plot_autotuning_trace_with_settings(rows: list[dict[str, Any]], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    source_rows = [row for row in source_rows if math.isfinite(_row_time_ns(row))]
    if not source_rows:
        return False

    def order_key(item: tuple[int, dict[str, Any]]) -> tuple[int, int, int, int]:
        idx, row = item
        iteration = _safe_int(row.get("iteration"), -1)
        trial = _safe_int(row.get("trial"), -1)
        firefly_id = _safe_int(row.get("firefly_id"), -1)
        option_index = _safe_int(row.get("option_index"), -1)
        primary = iteration if iteration >= 0 else trial if trial >= 0 else option_index if option_index >= 0 else idx
        return primary, firefly_id if firefly_id >= 0 else 0, option_index if option_index >= 0 else idx, idx

    ordered = [row for _, row in sorted(enumerate(source_rows), key=order_key)]
    xs = list(range(len(ordered)))
    ys = [_row_time_ns(row) for row in ordered]
    best_so_far: list[float] = []
    cur = float("inf")
    for val in ys:
        if math.isfinite(val):
            cur = min(cur, val)
        best_so_far.append(cur if math.isfinite(cur) else float("nan"))

    best_idx = min(range(len(ys)), key=lambda idx: ys[idx])
    best_row = ordered[best_idx]
    best_cfg = best_row.get("config", {}) if isinstance(best_row.get("config"), dict) else {}

    fig, ax = plt.subplots(figsize=(15, 6.4))
    colors = {"qss": "#1d4ed8", "sqs": "#0f766e", "ssq": "#b45309"}
    for variant in VARIANT_ORDER:
        vx = [x for x, row in zip(xs, ordered) if str(row.get("variant", "")).lower() == variant]
        vy = [_row_time_ns(row) for row in ordered if str(row.get("variant", "")).lower() == variant]
        if vx:
            ax.scatter(vx, vy, s=32, color=colors.get(variant), alpha=0.78, label=_variant_title(variant))
    ax.plot(xs, best_so_far, color="#111827", linewidth=1.8, label="najlepszy do tej pory")
    ax.scatter([best_idx], [ys[best_idx]], s=90, color="#dc2626", zorder=5)
    ax.annotate(
        f"best {ys[best_idx]:.3g} ns\n{_row_settings_label(best_row)}",
        xy=(best_idx, ys[best_idx]),
        xytext=(14, 16),
        textcoords="offset points",
        fontsize=9,
        bbox={"boxstyle": "round,pad=0.35", "fc": "#ffffff", "ec": "#d1d5db", "alpha": 0.95},
    )
    _article_axis_style(ax)
    ax.set_title("Przebieg autotuningu: czas wykonania i najlepszy wynik w czasie", fontsize=12)
    ax.set_xlabel("Kolejnosc ewaluacji")
    ax.set_ylabel("Czas [ns / element]")
    ax.legend(loc="best")
    bounds = _article_ylim(ys)
    if bounds is not None:
        ax.set_ylim(*bounds)
    fig.subplots_adjust(left=0.08, right=0.96, top=0.94, bottom=0.07, wspace=0.12)
    _save_annotated_figure(fig, out_path, dpi=200)
    plt.close(fig)
    return True


def _plot_autotuning_settings_heatmap(rows: list[dict[str, Any]], out_path: Path, *, top_n: int = 28) -> bool:
    source_rows = _preferred_rows(rows)
    ranked = sorted(
        [row for row in _unique_rows_by_config(source_rows) if math.isfinite(_row_time_ns(row))],
        key=_row_time_ns,
    )[: max(1, top_n)]
    if not ranked:
        return False

    matrix = np.array(
        [
            [_safe_int((row.get("config") or {}).get(key), 0) if isinstance(row.get("config"), dict) else 0 for key in OPTION_FIELDS]
            for row in ranked
        ],
        dtype=float,
    )
    labels = [f"{idx+1:02d}  {_row_settings_label(row)}" for idx, row in enumerate(ranked)]
    times = np.array([_row_time_ns(row) for row in ranked], dtype=float)

    fig, (ax_hm, ax_bar) = plt.subplots(
        1,
        2,
        figsize=(17, max(6.5, 0.36 * len(ranked) + 2.0)),
        gridspec_kw={"width_ratios": [3.7, 1.5], "wspace": 0.08},
    )
    im = ax_hm.imshow(matrix, aspect="auto", interpolation="nearest", cmap="Blues", vmin=0.0, vmax=1.0)
    ax_hm.set_title("Najlepsze konfiguracje autotuningu i ich ustawienia", fontsize=12)
    ax_hm.set_yticks(np.arange(len(labels)), labels, fontsize=7)
    ax_hm.set_xticks(
        np.arange(len(OPTION_FIELDS)),
        [OPTION_FIELD_LABELS.get(key, key) for key in OPTION_FIELDS],
        rotation=35,
        ha="right",
        fontsize=8,
    )
    for y in range(matrix.shape[0]):
        for x in range(matrix.shape[1]):
            ax_hm.text(x, y, str(int(matrix[y, x])), ha="center", va="center", fontsize=7, color="#111827")
    ax_hm.grid(False)
    for spine in ax_hm.spines.values():
        spine.set_visible(False)
    cbar = fig.colorbar(im, ax=ax_hm, fraction=0.018, pad=0.012)
    cbar.set_ticks([0, 1])
    cbar.set_ticklabels(["wył.", "wł."])

    y_pos = np.arange(len(ranked))
    ax_bar.barh(y_pos, times, color="#2563eb", alpha=0.88)
    ax_bar.invert_yaxis()
    ax_bar.set_yticks([])
    ax_bar.set_xlabel("Czas [ns / element]")
    ax_bar.set_title("Czas wykonania", fontsize=11)
    ax_bar.grid(True, axis="x", alpha=0.25)
    for y, val in zip(y_pos, times):
        ax_bar.text(val, y, f" {val:.3g}", va="center", fontsize=7)

    fig.subplots_adjust(left=0.08, right=0.96, top=0.94, bottom=0.07, wspace=0.12)
    _save_annotated_figure(fig, out_path, dpi=200)
    plt.close(fig)
    return True


def _plot_best_configuration_card(rows: list[dict[str, Any]], out_path: Path) -> bool:
    source_rows = _preferred_rows(rows)
    ranked = sorted([row for row in source_rows if math.isfinite(_row_time_ns(row))], key=_row_time_ns)
    if not ranked:
        return False
    best = ranked[0]
    cfg = best.get("config", {}) if isinstance(best.get("config"), dict) else {}
    metrics = best.get("metrics", {}) if isinstance(best.get("metrics"), dict) else {}
    time_ns = _row_time_ns(best)

    left_rows = [
        ("Backend", str(best.get("backend", ""))),
        ("Urządzenie", str(best.get("device", ""))),
        ("Operator", str(best.get("operator", cfg.get("operator", "")))),
        ("Wariant", str(best.get("variant", cfg.get("algorithm_variant", ""))).upper()),
        ("Maska bitowa opcji", str(best.get("combo_bits") or _combo_bits(cfg))),
        ("Indeks opcji", str(best.get("option_index", ""))),
        ("Liczba elementów", str(cfg.get("n_elements", metrics.get("n_elements", "")))),
        ("Liczba punktów całkowania", str(cfg.get("n_qp", metrics.get("n_qp_effective", "")))),
        ("Rozmiar grupy roboczej", str(cfg.get("workgroup_size", ""))),
    ]
    right_rows = [
        ("Czas [ns / element]", f"{time_ns:.6g}"),
        ("Czas całkowity [s]", f"{_safe_float(best.get('elapsed_s_mean')):.6g}"),
        ("GFLOP/s", f"{_safe_float(best.get('gflops_mean')):.6g}"),
        ("GB/s", f"{_safe_float(best.get('gbps_mean')):.6g}"),
        ("Wartość CV GFLOP/s", f"{_safe_float(metrics.get('cv_gflops')):.6g}"),
        ("Ocena mapowania", f"{_safe_float(metrics.get('mapping_score')):.6g}"),
    ]
    flag_rows = [(OPTION_FIELD_LABELS.get(key, key), "TAK" if _safe_int(cfg.get(key), 0) else "NIE") for key in OPTION_FIELDS]

    fig = plt.figure(figsize=(14, 7.8))
    fig.patch.set_facecolor("#f8fafc")
    ax = fig.add_subplot(111)
    ax.axis("off")
    ax.text(0.03, 0.94, "Najlepsza konfiguracja kodu Filipa / kampanii FEM", fontsize=18, fontweight="bold", color="#0f172a")
    ax.text(
        0.03,
        0.89,
        "Ten panel pokazuje nie tylko czas wykonania, ale też ustawienia autotuningu, które doprowadziły do najlepszego wyniku.",
        fontsize=10,
        color="#475569",
    )

    def draw_table(title: str, rows_payload: list[tuple[str, str]], bbox: list[float]) -> None:
        ax.text(bbox[0], bbox[1] + bbox[3] + 0.025, title, transform=ax.transAxes, fontsize=12, fontweight="bold", color="#1e293b")
        table = ax.table(
            cellText=[[k, v] for k, v in rows_payload],
            colLabels=["Pole", "Wartość"],
            colLoc="left",
            cellLoc="left",
            bbox=bbox,
        )
        table.auto_set_font_size(False)
        table.set_fontsize(8.5)
        for (row_idx, _col_idx), cell in table.get_celld().items():
            cell.set_edgecolor("#cbd5e1")
            cell.set_linewidth(0.5)
            cell.set_facecolor("#e2e8f0" if row_idx == 0 else "#ffffff")

    draw_table("Identyfikacja przypadku", left_rows, [0.03, 0.38, 0.42, 0.45])
    draw_table("Metryki wykonania", right_rows, [0.52, 0.50, 0.42, 0.33])
    draw_table("Flagi autotuningu", flag_rows, [0.52, 0.08, 0.42, 0.33])
    ax.text(0.03, 0.25, "Aktywne ustawienia:", fontsize=12, fontweight="bold", color="#1e293b")
    ax.text(0.03, 0.20, _option_settings_text(cfg), fontsize=10, color="#111827", wrap=True)
    ax.text(0.03, 0.12, "Uwaga interpretacyjna:", fontsize=12, fontweight="bold", color="#1e293b")
    ax.text(
        0.03,
        0.07,
        "Dla exact reference czas oznacza wewnętrzny czas oryginalnego kodu. Dla kampanii natywnych oznacza znormalizowany czas pomiaru FEM.",
        fontsize=9,
        color="#475569",
        wrap=True,
    )
    _save_annotated_figure(fig, out_path, dpi=200)
    plt.close(fig)
    return True


def generate_article_plots(optimization_dir: Path, *, max_operators: int = 2) -> dict[str, Any]:
    global _CURRENT_PLATFORM_LABEL
    _setup_plot_style()

    run = _load_run(optimization_dir)
    _CURRENT_PLATFORM_LABEL = _platform_label_for_run(run)
    plots_dir = _ensure_plots_dir(optimization_dir)
    appendix_dir = _ensure_appendix_dir(optimization_dir)
    operators = _paper_operator_selection(run["rows"], max_operators=max_operators)
    if not operators:
        operators = _select_plot_operators(_status_ok_rows(run["rows"]), max_operators=max(1, max_operators))
    autotuning_rows = run["rows"]
    generated_core: list[str] = []
    generated_appendix: list[str] = []

    method = str(run.get("method", "")).strip().lower()
    if not method and str(run.get("summary", {}).get("workflow", "")).strip() == "filip_original":
        method = "filip_original"
    if method == "filip_original":
        paper_refs: list[dict[str, Any]] = []
        sweep_rows: list[dict[str, Any]] = []
        rows = run["rows"]
        variant_rows = _status_ok_rows(run["rows"])
    else:
        paper_refs = _paper_reference_configs(run, operators)
        sweep_rows = _paper_sweep_rows(run, paper_refs, plots_dir) if paper_refs else []
        rows = sweep_rows or run["rows"]
        variant_rows = rows or run["rows"]
    selected_rows = rows or run["rows"]

    for variant in VARIANT_ORDER:
        variant_path = plots_dir / f"filip_variant_{variant}.png"
        if _plot_compact_variant_figure(
            variant_rows,
            operators=operators,
            variant=variant,
            out_path=variant_path,
            include_infeasible=(method == "filip_original"),
        ):
            generated_core.append(str(variant_path))

    trace_path = plots_dir / "filip_autotuning_trace.png"
    if _plot_autotuning_trace_with_settings(autotuning_rows, trace_path):
        generated_core.append(str(trace_path))

    best_path = plots_dir / "filip_best_summary.png"
    if _plot_best_summary(selected_rows, operators, best_path):
        generated_core.append(str(best_path))

    breakdown_path = plots_dir / "filip_memory_compute_breakdown.png"
    if _plot_memory_compute_breakdown(selected_rows, operators, breakdown_path):
        generated_core.append(str(breakdown_path))

    card_path = appendix_dir / "filip_best_configuration_card.png"
    if _plot_best_configuration_card(autotuning_rows, card_path):
        generated_appendix.append(str(card_path))

    summary = _write_article_manifest(
        optimization_dir,
        plots_dir=plots_dir,
        appendix_dir=appendix_dir,
        generated_core=generated_core,
        generated_appendix=generated_appendix,
        operators=operators,
        mode=method or "unknown",
        selected_rows_count=len(selected_rows),
    )
    summary.update(
        {
            "operators": operators,
            "paper_references": paper_refs,
            "paper_sweep_rows": len(rows),
            "generated_plots": generated_core,
            "appendix_plots": generated_appendix,
            "preferred_plot_order": list(PREFERRED_PLOT_ORDER),
        }
    )
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
