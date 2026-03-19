#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime
import json
import math
from pathlib import Path
import random
import sys
from typing import Any, Dict


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from device_resolution import resolve_device_index
from optimization.problems import FemParametricProblem, FemParametricProblemConfig
from analysis.filip_article_plots import generate_article_plots


def _parse_range(text: str, tp: type[int]):
    parts = [p.strip() for p in text.split(":") if p.strip()]
    if len(parts) != 2:
        raise ValueError(f"Invalid range: {text}. Expected min:max")
    lo = tp(parts[0])
    hi = tp(parts[1])
    if hi < lo:
        lo, hi = hi, lo
    return lo, hi


def _parse_csv(text: str) -> list[str]:
    return [x.strip() for x in text.split(",") if x.strip()]


def _make_out_dir(backend: str) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = ROOT / "data" / "optimization" / f"{ts}__filip_autotune__backend-{backend}"
    out.mkdir(parents=True, exist_ok=True)
    return out


def _objective_score(metrics: Dict[str, float], constraints_ok: bool, status: str) -> float:
    if status != "ok" or not constraints_ok:
        return -1e18
    g = float(metrics.get("gflops_mean", float("nan")))
    cv = float(metrics.get("cv_gflops", float("nan")))
    e = float(metrics.get("j_per_gflop", float("nan")))
    map_score = float(metrics.get("mapping_score", 0.0))

    if not math.isfinite(g) or g <= 0.0:
        return -1e18
    if not math.isfinite(cv):
        cv = 1.0
    if not math.isfinite(e):
        e = 0.0

    # Heuristic scalar objective:
    # maximize gflops, penalize instability and energy-per-gflop.
    score = g
    score -= abs(g) * min(max(cv, 0.0), 5.0) * 0.20
    score -= max(e, 0.0) * 0.05 * abs(g)
    score += map_score * 10.0
    return score


def main() -> None:
    ap = argparse.ArgumentParser(description="Filip-style FEM autotuning without firefly (random search).")
    ap.add_argument("--backend", choices=["cpu", "cuda", "hip", "metal", "opencl", "amd", "intel"], required=True)
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--trials", type=int, default=120)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--execution-policy", choices=["native_only", "allow_fallback"], default="native_only")
    ap.add_argument("--memory-budget-mb", type=int, default=0)
    ap.add_argument("--memory-budget-fraction", type=float, default=0.35)
    ap.add_argument("--eval-cache-size", type=int, default=4096)
    ap.add_argument("--screening-repeats", type=int, default=1)
    ap.add_argument("--screening-prune-factor", type=float, default=0.55)

    ap.add_argument("--fem-n-elements-range", default="20000:500000")
    ap.add_argument("--fem-n-qp-range", default="1:8")
    ap.add_argument("--fem-element-types", default="tet4,hex8")
    ap.add_argument("--fem-operators", default="diffusion,mass,convection,diffusion_mass,diffusion_convection_mass")
    ap.add_argument("--fem-dtypes", default="float32")
    ap.add_argument("--fem-variant-choices", default="qss,sqs,ssq")
    ap.add_argument("--fem-workgroup-sizes", default="32,64,128,256")
    ap.add_argument("--fem-use-workspace-pde-choices", default="0,1")
    ap.add_argument("--fem-use-workspace-geo-choices", default="0,1")
    ap.add_argument("--fem-use-workspace-shape-choices", default="0,1")
    ap.add_argument("--fem-use-workspace-stiff-choices", default="0,1")
    ap.add_argument("--fem-padding-choices", default="0,1")
    ap.add_argument("--fem-compute-all-shape-der-choices", default="0,1")
    ap.add_argument("--fem-coal-read-choices", default="0,1")
    ap.add_argument("--fem-coal-write-choices", default="0,1")
    ap.add_argument("--no-article-plots", action="store_true")
    args = ap.parse_args()

    n_elem_min, n_elem_max = _parse_range(args.fem_n_elements_range, int)
    n_qp_min, n_qp_max = _parse_range(args.fem_n_qp_range, int)
    element_types = _parse_csv(args.fem_element_types)
    operators = _parse_csv(args.fem_operators)
    dtypes = _parse_csv(args.fem_dtypes)
    variants = _parse_csv(args.fem_variant_choices)
    workgroup_sizes = [int(x) for x in _parse_csv(args.fem_workgroup_sizes)]
    ws_pde = [int(x) for x in _parse_csv(args.fem_use_workspace_pde_choices)]
    ws_geo = [int(x) for x in _parse_csv(args.fem_use_workspace_geo_choices)]
    ws_shape = [int(x) for x in _parse_csv(args.fem_use_workspace_shape_choices)]
    ws_stiff = [int(x) for x in _parse_csv(args.fem_use_workspace_stiff_choices)]
    paddings = [int(x) for x in _parse_csv(args.fem_padding_choices)]
    casfd = [int(x) for x in _parse_csv(args.fem_compute_all_shape_der_choices)]
    coal_read = [int(x) for x in _parse_csv(args.fem_coal_read_choices)]
    coal_write = [int(x) for x in _parse_csv(args.fem_coal_write_choices)]
    resolved_device_index, device_resolution_reason = resolve_device_index(args.backend, int(args.device_index))

    out_dir = _make_out_dir(args.backend)

    try:
        problem = FemParametricProblem(
            FemParametricProblemConfig(
                backend=args.backend,
                device_index=resolved_device_index,
                repeats=args.repeats,
                execution_policy=args.execution_policy,
                n_elements_min=n_elem_min,
                n_elements_max=n_elem_max,
                n_qp_min=n_qp_min,
                n_qp_max=n_qp_max,
                element_types=element_types,
                operators=operators,
                dtypes=dtypes,
                algorithm_variants=variants,
                workgroup_sizes=workgroup_sizes,
                use_workspace_for_pde_coeff_choices=ws_pde,
                use_workspace_for_geo_data_choices=ws_geo,
                use_workspace_for_shape_fun_choices=ws_shape,
                use_workspace_for_stiff_mat_choices=ws_stiff,
                padding_choices=paddings,
                compute_all_shape_fun_der_choices=casfd,
                coal_read_choices=coal_read,
                coal_write_choices=coal_write,
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

    rng = random.Random(args.seed)
    best_score = -1e18
    best_cfg: Dict[str, Any] = {}
    best_metrics: Dict[str, float] = {}
    feasible = 0

    eval_path = out_dir / "evaluations.jsonl"
    iter_path = out_dir / "iterations.jsonl"
    with eval_path.open("w", encoding="utf-8") as f, iter_path.open("w", encoding="utf-8") as itf:
        for trial in range(max(1, int(args.trials))):
            pos = problem.search_space.sample_position(rng)
            cfg = problem.search_space.decode(pos)
            res = problem.evaluate(cfg)
            score = _objective_score(res.metrics, res.constraints_ok, res.status)
            if res.status == "ok" and res.constraints_ok:
                feasible += 1
            row = {
                "iteration": trial,
                "trial": trial,
                "score": score,
                "brightness": score,
                "status": res.status,
                "constraints_ok": int(res.constraints_ok),
                "violations": "|".join(res.violations),
                "config": cfg,
                "metrics": res.metrics,
            }
            for key, value in sorted(res.metrics.items()):
                row[f"metric_{key}"] = value
            artifacts = dict(res.artifacts)
            if "resolved_backend" in artifacts:
                row["artifact_resolved_backend"] = artifacts.get("resolved_backend")
            if "execution_mode" in artifacts:
                row["artifact_execution_mode"] = artifacts.get("execution_mode")
            if "device" in artifacts:
                row["artifact_device"] = artifacts.get("device")
            f.write(json.dumps(row, ensure_ascii=True) + "\n")
            if score > best_score:
                best_score = score
                best_cfg = dict(cfg)
                best_metrics = dict(res.metrics)
            itf.write(
                json.dumps(
                    {
                        "iteration": trial,
                        "best_brightness": float(best_score),
                        "best_score": float(best_score),
                        "feasible_trials": int(feasible),
                    },
                    ensure_ascii=True,
                )
                + "\n"
            )

    summary = {
        "method": "random_search",
        "problem": problem.name,
        "backend": args.backend,
        "resolved_backend": problem.mode.resolved_backend,
        "execution_mode": problem.mode.execution_mode,
        "device": problem.mode.device_name,
        "device_index_requested": int(args.device_index),
        "device_index_used": int(resolved_device_index),
        "device_resolution_reason": str(device_resolution_reason),
        "trials": int(args.trials),
        "feasible_trials": int(feasible),
        "best_score": float(best_score),
        "best_config": best_cfg,
        "best_metrics": best_metrics,
        "out_dir": str(out_dir),
    }
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
    if not args.no_article_plots:
        try:
            plot_summary = generate_article_plots(out_dir)
            summary["article_plots_dir"] = plot_summary.get("plots_dir", "")
            summary["article_plots"] = plot_summary.get("generated_plots", [])
        except Exception as e:
            summary["article_plots_error"] = str(e)
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
    (out_dir / "best.json").write_text(
        json.dumps(
            {
                "best_score": float(best_score),
                "best_config": best_cfg,
                "best_metrics": best_metrics,
            },
            indent=2,
            ensure_ascii=True,
        ),
        encoding="utf-8",
    )

    problem.close()
    print("=== FILIP AUTOTUNING DONE ===")
    print(json.dumps(summary, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
