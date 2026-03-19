#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from datetime import datetime
import json
import math
from pathlib import Path
import sys
from typing import Any, Dict, Iterable, List


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from device_resolution import resolve_device_index
from analysis.filip_article_plots import generate_article_plots, _estimate_rw_bytes, _estimate_share, _roofline_peaks_for_backend
from optimization.problems import FemParametricProblem, FemParametricProblemConfig


VARIANT_ORDER = ["qss", "sqs", "ssq"]
OPTION_HEADERS = [
    " -D COAL_READ",
    " -D COAL_WRITE",
    " -D COMPUTE_ALL_SHAPE_FUN_DER",
    " -D USE_WORKSPACE_FOR_PDE_COEFF",
    " -D USE_WORKSPACE_FOR_GEO_DATA",
    " -D USE_WORKSPACE_FOR_SHAPE_FUN",
    " -D USE_WORKSPACE_FOR_STIFF_MAT",
    " -D WORKSPACE_PADDING=0",
    " -D WORKSPACE_PADDING=1",
]
ORIGINAL_FOOTER_HEADERS = [
    "el_data_in [MB]",
    "el_data_out [MB]",
    "nr_elems_per_work_group",
    "nr_elems",
    "nr_elems_per_thread",
    "nr_work_groups",
    "work_group_size",
    "nr_threads",
    "sending el_data_in to GPU memory",
    "input_bandwidth_[GB/s]",
    "executing kernel",
    "internal",
    "copying output buffer",
    "output_bandwidth_[GB/s]",
    "status",
    "constraints_ok",
    "ns_per_(element*qp)",
    "gflops_mean",
    "gbps_mean",
    "energy_j_mean",
    "j_per_gflop",
]
PROFILE_SCALE = {"quick": 0.5, "paper": 1.0, "full": 1.5}
BASE_ELEMENTS = {
    "cpu": {"tet4": 12000, "hex8": 6000},
    "native_gpu": {"tet4": 64000, "hex8": 24000},
    "mapped_gpu": {"tet4": 32000, "hex8": 12000},
}
DEFAULT_OPERATORS = ["diffusion", "diffusion_convection_mass"]
DEFAULT_ELEMENT_TYPES = ["tet4"]


def _parse_csv(raw: str) -> list[str]:
    return [item.strip().lower() for item in str(raw).split(",") if item.strip()]


def _safe_float(value: Any) -> float:
    try:
        if value is None:
            return float("nan")
        return float(value)
    except Exception:
        return float("nan")


def _safe_int(value: Any, default: int = 0) -> int:
    try:
        return int(value)
    except Exception:
        return int(default)


def _is_finite_positive(value: float) -> bool:
    return math.isfinite(value) and value > 0.0


def _make_out_dir(backend: str) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = ROOT / "data" / "optimization" / f"{ts}__filip_original__backend-{backend}"
    out.mkdir(parents=True, exist_ok=True)
    return out


def _qp_cap(element_type: str) -> int:
    if element_type == "tet4":
        return 4
    if element_type == "hex8":
        return 8
    return 1


def _indeks_binary_reflected(m: int) -> int:
    i = 0
    while m % 2 == 0:
        i += 1
        m //= 2
    return i


def _filip_option_rows() -> list[list[int]]:
    k = 9
    w = [0] * (k + 1)
    step = [1] * (k + 1)
    rows: list[list[int]] = []
    m = 0
    while True:
        rows.append([w[idx] for idx in range(1, k + 1)])
        m += 1
        idx = _indeks_binary_reflected(m) + 1
        if idx > k:
            break
        w[idx] += step[idx]
        if w[idx] == 0:
            step[idx] = 1
        if w[idx] == 1:
            step[idx] = -1

    filtered: list[list[int]] = []
    for row in rows:
        if (
            not (row[3] and row[4])
            and not (row[3] and row[5])
            and not (row[3] and row[6])
            and not (row[4] and row[5])
            and not (row[4] and row[6])
            and not (row[5] and row[6])
            and not (row[7] and row[8])
            and not ((row[7] == 0) and (row[8] == 0))
        ):
            filtered.append(row)
    return filtered


def _option_row_to_config_flags(row: list[int]) -> dict[str, int]:
    return {
        "coal_read": int(row[0]),
        "coal_write": int(row[1]),
        "compute_all_shape_fun_der": int(row[2]),
        "use_workspace_for_pde_coeff": int(row[3]),
        "use_workspace_for_geo_data": int(row[4]),
        "use_workspace_for_shape_fun": int(row[5]),
        "use_workspace_for_stiff_mat": int(row[6]),
        "padding": 1 if int(row[8]) == 1 else 0,
    }


def _flatten_mapping(prefix: str, data: dict[str, Any]) -> dict[str, Any]:
    out: dict[str, Any] = {}
    for key, value in sorted(data.items()):
        out[f"{prefix}{key}"] = value
    return out


def _ns_per_unit(metrics: dict[str, Any]) -> float:
    elapsed = _safe_float(metrics.get("elapsed_s_mean"))
    n_elem = max(1.0, _safe_float(metrics.get("n_elements")))
    n_qp = max(1.0, _safe_float(metrics.get("n_qp_effective")) or _safe_float(metrics.get("n_qp_requested")) or 1.0)
    if not _is_finite_positive(elapsed):
        return float("nan")
    return elapsed * 1e9 / max(1.0, n_elem * n_qp)


def _mode_bucket(problem: FemParametricProblem) -> str:
    if problem.mode.resolved_backend == "cpu":
        return "cpu"
    if problem.mode.execution_mode == "native":
        return "native_gpu"
    return "mapped_gpu"


def _suggest_workgroup(problem: FemParametricProblem, requested: int) -> int:
    supported = list(problem.mode.profile.supported_workgroup_sizes)
    if problem.mode.resolved_backend == "cpu":
        return 1
    if requested > 0:
        if requested in supported:
            return requested
        if supported:
            return min(supported, key=lambda value: abs(value - requested))
        return requested
    for preferred in (64, 32, 128, 256):
        if preferred in supported:
            return preferred
    if supported:
        return supported[0]
    return 64


def _memory_probe_config(
    *,
    n_elements: int,
    n_qp: int,
    element_type: str,
    operator: str,
    dtype: str,
    workgroup_size: int,
) -> dict[str, Any]:
    return {
        "n_elements": int(n_elements),
        "n_qp": int(n_qp),
        "element_type": str(element_type),
        "operator": str(operator),
        "dtype": str(dtype),
        "algorithm_variant": "ssq",
        "workgroup_size": int(workgroup_size),
        "use_workspace_for_pde_coeff": 1,
        "use_workspace_for_geo_data": 1,
        "use_workspace_for_shape_fun": 1,
        "use_workspace_for_stiff_mat": 1,
        "padding": 1,
        "compute_all_shape_fun_der": 1,
        "coal_read": 1,
        "coal_write": 1,
    }


def _auto_n_elements(
    *,
    problem: FemParametricProblem,
    profile: str,
    element_type: str,
    operator: str,
    dtype: str,
    n_qp: int,
    workgroup_size: int,
) -> int:
    bucket = _mode_bucket(problem)
    scale = PROFILE_SCALE.get(profile, 1.0)
    desired = int(BASE_ELEMENTS[bucket][element_type] * scale)
    if dtype == "float64":
        desired = max(1, desired // 2)

    probe_budget = int(problem.mode.profile.memory_budget_bytes * (0.72 if bucket == "cpu" else 0.58))
    probe_budget = max(64 * 1024 * 1024, probe_budget)

    def estimate(n_elements: int) -> int:
        cfg = _memory_probe_config(
            n_elements=n_elements,
            n_qp=n_qp,
            element_type=element_type,
            operator=operator,
            dtype=dtype,
            workgroup_size=workgroup_size,
        )
        return int(problem._estimate_candidate_memory_bytes(cfg))

    low = 1
    high = max(1, desired)
    if estimate(high) <= probe_budget:
        return high

    best = low
    while low <= high:
        mid = (low + high) // 2
        if estimate(mid) <= probe_budget:
            best = mid
            low = mid + 1
        else:
            high = mid - 1
    floor = 2000 if bucket == "cpu" else 8000
    return max(floor, best)


def _case_specs(
    *,
    problem: FemParametricProblem,
    profile: str,
    element_types: list[str],
    operators: list[str],
    dtype: str,
    requested_n_qp: int,
    requested_n_elements: int,
    workgroup_size: int,
) -> list[dict[str, Any]]:
    cases: list[dict[str, Any]] = []
    for element_type in element_types:
        qp = _qp_cap(element_type) if requested_n_qp <= 0 else max(1, min(requested_n_qp, _qp_cap(element_type)))
        for operator in operators:
            n_elements = requested_n_elements
            if n_elements <= 0:
                n_elements = _auto_n_elements(
                    problem=problem,
                    profile=profile,
                    element_type=element_type,
                    operator=operator,
                    dtype=dtype,
                    n_qp=qp,
                    workgroup_size=workgroup_size,
                )
            cases.append(
                {
                    "element_type": element_type,
                    "operator": operator,
                    "dtype": dtype,
                    "n_qp": qp,
                    "n_elements": int(n_elements),
                    "workgroup_size": int(workgroup_size),
                }
            )
    return cases


def _serialize_case(case: dict[str, Any]) -> str:
    return (
        f"{case['element_type']} | {case['operator']} | n_elem={case['n_elements']} | "
        f"n_qp={case['n_qp']} | wg={case['workgroup_size']} | dtype={case['dtype']}"
    )


def _best_payload(record: dict[str, Any]) -> dict[str, Any]:
    return {
        "case_key": record["case_key"],
        "option_index": int(record["option_index"]),
        "option_row": list(record["option_row"]),
        "variant": str(record["variant"]),
        "config": dict(record["config_effective"]),
        "metrics": dict(record["metrics"]),
        "status": str(record["status"]),
        "constraints_ok": bool(record["constraints_ok"]),
        "ns_per_unit": _safe_float(record["ns_per_unit"]),
    }


def _plot_row(record: dict[str, Any], backend: str, device: str) -> dict[str, Any]:
    return {
        "backend": backend,
        "device": device,
        "config": dict(record["config_effective"]),
        "metrics": dict(record["metrics"]),
        "status": str(record["status"]),
        "constraints_ok": bool(record["constraints_ok"]),
        "operator": str(record["config_effective"].get("operator", "")),
        "variant": str(record["config_effective"].get("algorithm_variant", "")),
    }


def _phase_metrics(record: dict[str, Any], peak_gflops: float, peak_bw: float) -> dict[str, float]:
    elapsed = _safe_float(record["metrics"].get("elapsed_s_mean"))
    row = _plot_row(record, backend=str(record["backend"]), device=str(record["device"]))
    read_share, compute_share, write_share = _estimate_share(row, peak_gflops, peak_bw)
    read_t = elapsed * max(0.0, read_share) / 100.0
    compute_t = elapsed * max(0.0, compute_share) / 100.0
    write_t = elapsed * max(0.0, write_share) / 100.0
    read_bytes, write_bytes = _estimate_rw_bytes(row)
    read_bw = read_bytes / max(read_t, 1e-12) / 1e9 if read_t > 0.0 else float("inf")
    write_bw = write_bytes / max(write_t, 1e-12) / 1e9 if write_t > 0.0 else float("inf")
    return {
        "el_data_in_mb": read_bytes / 1e6,
        "el_data_out_mb": write_bytes / 1e6,
        "input_time_s": read_t,
        "input_bw_gbps": read_bw,
        "kernel_time_s": elapsed,
        "internal_time_s": compute_t,
        "output_time_s": write_t,
        "output_bw_gbps": write_bw,
    }


def _write_options_txt(out_dir: Path, option_rows: list[list[int]]) -> Path:
    path = out_dir / "options.txt"
    with path.open("w", encoding="utf-8") as f:
        for row in option_rows:
            f.write("\t".join(str(int(v)) for v in row) + "\n")
    return path


def _write_case_csvs(out_dir: Path, records: list[dict[str, Any]], option_rows: list[list[int]], backend: str, device: str) -> tuple[Path, list[str]]:
    csv_dir = out_dir / "csv"
    csv_dir.mkdir(parents=True, exist_ok=True)
    plot_rows = [_plot_row(record, backend=backend, device=device) for record in records if record["status"] == "ok"]
    peak_gflops, peak_bw = _roofline_peaks_for_backend(backend, plot_rows)

    generated: list[str] = []
    grouped: dict[tuple[str, str], list[dict[str, Any]]] = {}
    for record in records:
        key = (str(record["case_key"]), str(record["variant"]))
        grouped.setdefault(key, []).append(record)

    for (case_key, variant), group in grouped.items():
        ordered = sorted(group, key=lambda row: int(row["option_index"]))
        first_cfg = ordered[0]["config_effective"]
        path = csv_dir / f"result__{case_key}__{variant.upper()}__{backend}.csv"
        with path.open("w", encoding="utf-8", newline="") as f:
            writer = csv.writer(f)
            writer.writerow([*OPTION_HEADERS, *ORIGINAL_FOOTER_HEADERS])
            for record in ordered:
                cfg = record["config_effective"]
                metrics = record["metrics"]
                phase = _phase_metrics(record, peak_gflops, peak_bw)
                wg = max(1, _safe_int(cfg.get("workgroup_size"), 1))
                n_elements = max(1, _safe_int(cfg.get("n_elements"), 1))
                n_threads = int(math.ceil(n_elements / max(wg, 1))) * max(wg, 1)
                n_work_groups = max(1, int(math.ceil(n_elements / max(wg, 1))))
                writer.writerow(
                    [
                        *record["option_row"],
                        f"{phase['el_data_in_mb']:.6f}",
                        f"{phase['el_data_out_mb']:.6f}",
                        wg,
                        n_elements,
                        1,
                        n_work_groups,
                        wg,
                        n_threads,
                        f"{phase['input_time_s']:.9f}",
                        f"{phase['input_bw_gbps']:.6f}",
                        f"{_safe_float(metrics.get('elapsed_s_mean')):.9f}",
                        f"{phase['internal_time_s']:.9f}",
                        f"{phase['output_time_s']:.9f}",
                        f"{phase['output_bw_gbps']:.6f}",
                        record["status"],
                        int(record["constraints_ok"]),
                        f"{_safe_float(record['ns_per_unit']):.6f}",
                        f"{_safe_float(metrics.get('gflops_mean')):.6f}",
                        f"{_safe_float(metrics.get('gbps_mean')):.6f}",
                        f"{_safe_float(metrics.get('energy_j_mean')):.6f}",
                        f"{_safe_float(metrics.get('j_per_gflop')):.6f}",
                    ]
                )
        generated.append(str(path))

    combined_path = csv_dir / f"result_filip_original__{backend}.csv"
    with combined_path.open("w", encoding="utf-8", newline="") as f:
        writer = csv.writer(f)
        writer.writerow(
            [
                "backend",
                "device",
                "case_key",
                "variant",
                "option_index",
                *OPTION_HEADERS,
                *ORIGINAL_FOOTER_HEADERS,
            ]
        )
        for record in sorted(records, key=lambda row: (str(row["case_key"]), str(row["variant"]), int(row["option_index"]))):
            metrics = record["metrics"]
            phase = _phase_metrics(record, peak_gflops, peak_bw)
            cfg = record["config_effective"]
            wg = max(1, _safe_int(cfg.get("workgroup_size"), 1))
            n_elements = max(1, _safe_int(cfg.get("n_elements"), 1))
            n_threads = int(math.ceil(n_elements / max(wg, 1))) * max(wg, 1)
            n_work_groups = max(1, int(math.ceil(n_elements / max(wg, 1))))
            writer.writerow(
                [
                    backend,
                    device,
                    record["case_key"],
                    str(record["variant"]).upper(),
                    int(record["option_index"]),
                    *record["option_row"],
                    f"{phase['el_data_in_mb']:.6f}",
                    f"{phase['el_data_out_mb']:.6f}",
                    wg,
                    n_elements,
                    1,
                    n_work_groups,
                    wg,
                    n_threads,
                    f"{phase['input_time_s']:.9f}",
                    f"{phase['input_bw_gbps']:.6f}",
                    f"{_safe_float(metrics.get('elapsed_s_mean')):.9f}",
                    f"{phase['internal_time_s']:.9f}",
                    f"{phase['output_time_s']:.9f}",
                    f"{phase['output_bw_gbps']:.6f}",
                    record["status"],
                    int(record["constraints_ok"]),
                    f"{_safe_float(record['ns_per_unit']):.6f}",
                    f"{_safe_float(metrics.get('gflops_mean')):.6f}",
                    f"{_safe_float(metrics.get('gbps_mean')):.6f}",
                    f"{_safe_float(metrics.get('energy_j_mean')):.6f}",
                    f"{_safe_float(metrics.get('j_per_gflop')):.6f}",
                ]
            )
    return combined_path, generated


def main() -> None:
    ap = argparse.ArgumentParser(description="Strict Filip-style exhaustive FEM benchmark without firefly.")
    ap.add_argument("--backend", choices=["cpu", "cuda", "hip", "metal", "opencl", "amd", "intel"], required=True)
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="paper")
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--execution-policy", choices=["native_only", "allow_fallback"], default="native_only")
    ap.add_argument("--memory-budget-mb", type=int, default=0)
    ap.add_argument("--memory-budget-fraction", type=float, default=0.35)
    ap.add_argument("--eval-cache-size", type=int, default=1024)
    ap.add_argument("--screening-repeats", type=int, default=1)
    ap.add_argument("--screening-prune-factor", type=float, default=0.0)
    ap.add_argument("--operators", default=",".join(DEFAULT_OPERATORS))
    ap.add_argument("--element-types", default=",".join(DEFAULT_ELEMENT_TYPES))
    ap.add_argument("--variants", default=",".join(VARIANT_ORDER))
    ap.add_argument("--dtype", choices=["float32", "float64"], default="float32")
    ap.add_argument("--n-qp", type=int, default=0, help="0 = automatic (element cap, e.g. 4 for tet4)")
    ap.add_argument("--n-elements", type=int, default=0, help="0 = automatic, chosen conservatively from memory budget")
    ap.add_argument("--workgroup-size", type=int, default=0, help="0 = automatic, fixed workgroup close to Filip's workflow")
    ap.add_argument("--no-article-plots", action="store_true")
    ap.add_argument("--limit-option-rows", type=int, default=0, help=argparse.SUPPRESS)
    args = ap.parse_args()

    operators = _parse_csv(args.operators)
    element_types = _parse_csv(args.element_types)
    variants = _parse_csv(args.variants)
    if not operators:
        operators = list(DEFAULT_OPERATORS)
    if not element_types:
        element_types = list(DEFAULT_ELEMENT_TYPES)
    if not variants:
        variants = list(VARIANT_ORDER)

    out_dir = _make_out_dir(args.backend)
    option_rows = _filip_option_rows()
    if int(args.limit_option_rows) > 0:
        option_rows = option_rows[: int(args.limit_option_rows)]
    options_txt = _write_options_txt(out_dir, option_rows)
    resolved_device_index, device_resolution_reason = resolve_device_index(args.backend, int(args.device_index))

    try:
        problem = FemParametricProblem(
            FemParametricProblemConfig(
                backend=args.backend,
                device_index=resolved_device_index,
                repeats=args.repeats,
                execution_policy=args.execution_policy,
                n_elements_min=1,
                n_elements_max=1,
                n_qp_min=1,
                n_qp_max=1,
                element_types=element_types,
                operators=operators,
                dtypes=[args.dtype],
                algorithm_variants=variants,
                workgroup_sizes=[1, 32, 64, 128, 256, 512],
                use_workspace_for_pde_coeff_choices=[0, 1],
                use_workspace_for_geo_data_choices=[0, 1],
                use_workspace_for_shape_fun_choices=[0, 1],
                use_workspace_for_stiff_mat_choices=[0, 1],
                padding_choices=[0, 1],
                compute_all_shape_fun_der_choices=[0, 1],
                coal_read_choices=[0, 1],
                coal_write_choices=[0, 1],
                memory_budget_mb=args.memory_budget_mb,
                memory_budget_fraction=args.memory_budget_fraction,
                eval_cache_size=args.eval_cache_size,
                screening_repeats=args.screening_repeats,
                screening_prune_factor=args.screening_prune_factor,
                record_raw_artifacts=False,
            )
        )
    except Exception as e:
        print(f"[ERROR] Cannot initialize fem_parametric problem: {e}")
        raise SystemExit(2)

    workgroup_size = _suggest_workgroup(problem, int(args.workgroup_size))
    cases = _case_specs(
        problem=problem,
        profile=args.profile,
        element_types=element_types,
        operators=operators,
        dtype=args.dtype,
        requested_n_qp=int(args.n_qp),
        requested_n_elements=int(args.n_elements),
        workgroup_size=workgroup_size,
    )
    total_evals = len(cases) * len(variants) * len(option_rows)

    print("=== FILIP ORIGINAL (STRICT) ===")
    print(f"requested backend : {args.backend}")
    print(f"resolved backend  : {problem.mode.resolved_backend}")
    print(f"execution mode    : {problem.mode.execution_mode}")
    print(f"device            : {problem.mode.device_name}")
    print(f"device index req  : {int(args.device_index)}")
    print(f"device index used : {int(resolved_device_index)} ({device_resolution_reason})")
    print(f"profile           : {args.profile}")
    print(f"repeats           : {args.repeats}")
    print(f"variants          : {','.join(variants)}")
    print(f"option rows       : {len(option_rows)}")
    print(f"workgroup size    : {workgroup_size}")
    print("selected cases:")
    for case in cases:
        print(f"  - {_serialize_case(case)}")
    print(f"total evaluations : {total_evals}")

    eval_path = out_dir / "evaluations.jsonl"
    iter_path = out_dir / "iterations.jsonl"
    records: list[dict[str, Any]] = []
    best_overall: dict[str, Any] | None = None
    best_per_case: dict[str, dict[str, Any]] = {}
    feasible = 0
    complete = 0

    with eval_path.open("w", encoding="utf-8") as ef, iter_path.open("w", encoding="utf-8") as itf:
        for case_idx, case in enumerate(cases):
            case_key = f"{case['element_type']}__{case['operator']}"
            print(f"\n=== CASE {case_idx + 1}/{len(cases)}: {_serialize_case(case)} ===")
            for variant in variants:
                print(f"--- variant={variant.upper()} ---")
                for option_index, option_row in enumerate(option_rows):
                    cfg = dict(case)
                    cfg["algorithm_variant"] = variant
                    cfg.update(_option_row_to_config_flags(option_row))
                    res = problem.evaluate(cfg)
                    cfg_eff = res.artifacts.get("config_effective") if isinstance(res.artifacts.get("config_effective"), dict) else dict(cfg)
                    metrics = dict(res.metrics)
                    ns_per_unit = _ns_per_unit(metrics)
                    score = -ns_per_unit if res.status == "ok" and res.constraints_ok and math.isfinite(ns_per_unit) else -1e18
                    if res.status == "ok" and res.constraints_ok:
                        feasible += 1

                    complete += 1
                    record = {
                        "iteration": complete - 1,
                        "trial": complete - 1,
                        "brightness": score,
                        "score": score,
                        "status": str(res.status),
                        "constraints_ok": bool(res.constraints_ok),
                        "violations": list(res.violations),
                        "config_requested": dict(cfg),
                        "config_effective": dict(cfg_eff),
                        "metrics": metrics,
                        "artifacts": dict(res.artifacts),
                        "case_key": case_key,
                        "variant": variant,
                        "option_index": option_index,
                        "option_row": list(option_row),
                        "ns_per_unit": ns_per_unit,
                        "backend": problem.mode.resolved_backend,
                        "device": problem.mode.device_name,
                    }
                    records.append(record)

                    if res.status == "ok" and res.constraints_ok and math.isfinite(ns_per_unit):
                        current_best = best_per_case.get(case_key)
                        if current_best is None or ns_per_unit < _safe_float(current_best.get("ns_per_unit")):
                            best_per_case[case_key] = _best_payload(record)
                        if best_overall is None or ns_per_unit < _safe_float(best_overall.get("ns_per_unit")):
                            best_overall = _best_payload(record)

                    row = {
                        "timestamp": datetime.now().astimezone().isoformat(),
                        "iteration": complete - 1,
                        "trial": complete - 1,
                        "brightness": score,
                        "score": score,
                        "status": str(res.status),
                        "constraints_ok": int(res.constraints_ok),
                        "violations": "|".join(res.violations),
                        "case_key": case_key,
                        "variant": variant,
                        "option_index": option_index,
                        "option_row": list(option_row),
                        "config": dict(cfg_eff),
                        "metrics": metrics,
                        **_flatten_mapping("cfg_", dict(cfg_eff)),
                        **_flatten_mapping("metric_", metrics),
                    }
                    if "resolved_backend" in res.artifacts:
                        row["artifact_resolved_backend"] = res.artifacts.get("resolved_backend")
                    if "execution_mode" in res.artifacts:
                        row["artifact_execution_mode"] = res.artifacts.get("execution_mode")
                    if "device" in res.artifacts:
                        row["artifact_device"] = res.artifacts.get("device")
                    ef.write(json.dumps(row, ensure_ascii=True) + "\n")
                    best_ns = _safe_float(best_overall.get("ns_per_unit")) if best_overall is not None else float("nan")
                    itf.write(
                        json.dumps(
                            {
                                "iteration": complete - 1,
                                "completed_evals": complete,
                                "total_evals": total_evals,
                                "best_brightness": float(score if best_overall is None else -best_ns),
                                "best_score": float(score if best_overall is None else -best_ns),
                                "best_ns_per_unit": best_ns,
                                "feasible_trials": int(feasible),
                            },
                            ensure_ascii=True,
                        )
                        + "\n"
                    )
                    metrics_g = _safe_float(metrics.get("gflops_mean"))
                    print(
                        f"[{complete:4d}/{total_evals}] case={case_key} var={variant.upper()} opt={option_index:02d} "
                        f"status={res.status} ok={int(res.constraints_ok)} ns={(ns_per_unit if math.isfinite(ns_per_unit) else float('nan')):.3f} "
                        f"gflops={(metrics_g if math.isfinite(metrics_g) else float('nan')):.2f}"
                    )

    combined_csv, case_csvs = _write_case_csvs(
        out_dir=out_dir,
        records=records,
        option_rows=option_rows,
        backend=problem.mode.resolved_backend,
        device=problem.mode.device_name,
    )

    summary = {
        "method": "filip_original",
        "benchmark_mode": "strict_filip",
        "problem": problem.name,
        "backend": args.backend,
        "resolved_backend": problem.mode.resolved_backend,
        "execution_mode": problem.mode.execution_mode,
        "device": problem.mode.device_name,
        "device_index_requested": int(args.device_index),
        "device_index_used": int(resolved_device_index),
        "device_resolution_reason": str(device_resolution_reason),
        "profile": args.profile,
        "repeats": int(args.repeats),
        "dtype": args.dtype,
        "variants": variants,
        "operators": operators,
        "element_types": element_types,
        "workgroup_size": int(workgroup_size),
        "n_qp_requested": int(args.n_qp),
        "n_elements_requested": int(args.n_elements),
        "cases": cases,
        "option_rows": len(option_rows),
        "total_evaluations": int(total_evals),
        "feasible_evaluations": int(feasible),
        "options_txt": str(options_txt),
        "csv_dir": str(combined_csv.parent),
        "combined_csv": str(combined_csv),
        "case_csvs": case_csvs,
        "best_overall": best_overall or {},
        "best_per_case": best_per_case,
        "out_dir": str(out_dir),
    }
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
    (out_dir / "best.json").write_text(
        json.dumps(
            {
                "best_overall": best_overall or {},
                "best_per_case": best_per_case,
            },
            indent=2,
            ensure_ascii=True,
        ),
        encoding="utf-8",
    )

    if not args.no_article_plots:
        try:
            plot_summary = generate_article_plots(out_dir)
            summary["article_plots_dir"] = plot_summary.get("plots_dir", "")
            summary["article_plots"] = plot_summary.get("generated_plots", [])
        except Exception as e:
            summary["article_plots_error"] = str(e)
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")

    problem.close()
    print("=== FILIP ORIGINAL DONE ===")
    print(json.dumps(summary, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
