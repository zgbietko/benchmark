#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime
import json
import math
from pathlib import Path
import random
import statistics as stats
import sys
from typing import Any, Dict, List


ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from optimization.problems import FemParametricProblem, FemParametricProblemConfig


ALL_BACKENDS = ["cpu", "cuda", "hip", "opencl", "metal", "amd", "intel"]


def _parse_csv(raw: str) -> List[str]:
    return [x.strip() for x in raw.split(",") if x.strip()]


def _parse_range(raw: str) -> tuple[int, int]:
    parts = [x.strip() for x in raw.split(":") if x.strip()]
    if len(parts) != 2:
        raise ValueError(f"Invalid range '{raw}'. Use min:max")
    lo = int(parts[0])
    hi = int(parts[1])
    if hi < lo:
        lo, hi = hi, lo
    return lo, hi


def _parse_backends(raw: str) -> List[str]:
    txt = raw.strip().lower()
    if txt == "all":
        return list(ALL_BACKENDS)
    out: List[str] = []
    for b in _parse_csv(txt):
        if b not in ALL_BACKENDS:
            raise ValueError(f"Unsupported backend: {b}")
        if b not in out:
            out.append(b)
    if not out:
        raise ValueError("No backends selected")
    return out


def _rank_with_ties(values: List[float]) -> List[float]:
    indexed = list(enumerate(values))
    indexed.sort(key=lambda x: x[1])
    ranks = [0.0] * len(values)
    i = 0
    while i < len(indexed):
        j = i
        while j + 1 < len(indexed) and indexed[j + 1][1] == indexed[i][1]:
            j += 1
        avg_rank = (i + j) / 2.0 + 1.0
        for k in range(i, j + 1):
            ranks[indexed[k][0]] = avg_rank
        i = j + 1
    return ranks


def _spearman(xs: List[float], ys: List[float]) -> float:
    if len(xs) != len(ys) or len(xs) < 2:
        return float("nan")
    rx = _rank_with_ties(xs)
    ry = _rank_with_ties(ys)
    mx = stats.mean(rx)
    my = stats.mean(ry)
    num = sum((a - mx) * (b - my) for a, b in zip(rx, ry))
    denx = math.sqrt(sum((a - mx) ** 2 for a in rx))
    deny = math.sqrt(sum((b - my) ** 2 for b in ry))
    den = denx * deny
    if den <= 0:
        return float("nan")
    return float(num / den)


def _sample_configs(
    *,
    seed: int,
    count: int,
    n_elements_range: tuple[int, int],
    n_qp_range: tuple[int, int],
    element_types: List[str],
    operators: List[str],
    dtypes: List[str],
    variants: List[str],
    workgroup_sizes: List[int],
    flag_values: List[int],
) -> List[Dict[str, Any]]:
    rng = random.Random(seed)
    out: List[Dict[str, Any]] = []
    for _ in range(max(1, count)):
        out.append(
            {
                "n_elements": rng.randint(n_elements_range[0], n_elements_range[1]),
                "n_qp": rng.randint(n_qp_range[0], n_qp_range[1]),
                "element_type": rng.choice(element_types),
                "operator": rng.choice(operators),
                "dtype": rng.choice(dtypes),
                "algorithm_variant": rng.choice(variants),
                "workgroup_size": rng.choice(workgroup_sizes),
                "use_workspace_for_pde_coeff": rng.choice(flag_values),
                "use_workspace_for_geo_data": rng.choice(flag_values),
                "use_workspace_for_shape_fun": rng.choice(flag_values),
                "use_workspace_for_stiff_mat": rng.choice(flag_values),
                "padding": rng.choice(flag_values),
                "compute_all_shape_fun_der": rng.choice(flag_values),
                "coal_read": rng.choice(flag_values),
                "coal_write": rng.choice(flag_values),
            }
        )
    return out


def _make_problem(args: argparse.Namespace, backend: str) -> FemParametricProblem:
    return FemParametricProblem(
        FemParametricProblemConfig(
            backend=backend,
            device_index=args.device_index,
            repeats=args.repeats,
            execution_policy=args.execution_policy,
            n_elements_min=args.n_elements_range[0],
            n_elements_max=args.n_elements_range[1],
            n_qp_min=args.n_qp_range[0],
            n_qp_max=args.n_qp_range[1],
            element_types=args.element_types,
            operators=args.operators,
            dtypes=args.dtypes,
            algorithm_variants=args.variants,
            workgroup_sizes=args.workgroup_sizes,
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
            screening_repeats=args.screening_repeats,
            screening_prune_factor=args.screening_prune_factor,
            eval_cache_size=args.eval_cache_size,
        )
    )


def _evaluate_backend(
    *,
    args: argparse.Namespace,
    backend: str,
    configs: List[Dict[str, Any]],
) -> Dict[str, Any]:
    payload: Dict[str, Any] = {
        "backend": backend,
        "available": False,
        "error": "",
        "resolved_backend": None,
        "execution_mode": None,
        "mapping_score": None,
        "device": None,
        "rows": [],
    }

    try:
        problem = _make_problem(args, backend)
    except Exception as e:
        payload["error"] = str(e)
        return payload

    payload["available"] = True
    payload["resolved_backend"] = problem.mode.resolved_backend
    payload["execution_mode"] = problem.mode.execution_mode
    payload["mapping_score"] = problem.mode.mapping_score
    payload["device"] = problem.mode.device_name

    try:
        for idx, cfg in enumerate(configs):
            res = problem.evaluate(cfg)
            payload["rows"].append(
                {
                    "config_id": idx,
                    "status": res.status,
                    "constraints_ok": int(res.constraints_ok),
                    "gflops_mean": float(res.metrics.get("gflops_mean", float("nan"))),
                    "gbps_mean": float(res.metrics.get("gbps_mean", float("nan"))),
                    "cv_gflops": float(res.metrics.get("cv_gflops", float("nan"))),
                    "memory_estimated_bytes": float(res.metrics.get("memory_estimated_bytes", float("nan"))),
                }
            )
    finally:
        problem.close()
    return payload


def _valid_gflops(row: Dict[str, Any]) -> bool:
    if str(row.get("status", "")) != "ok":
        return False
    v = float(row.get("gflops_mean", float("nan")))
    return math.isfinite(v) and v > 0.0


def _compare_to_baseline(
    baseline_rows: List[Dict[str, Any]],
    target_rows: List[Dict[str, Any]],
    top_k: int,
) -> Dict[str, Any]:
    baseline_by_id = {int(r["config_id"]): r for r in baseline_rows}
    target_by_id = {int(r["config_id"]): r for r in target_rows}

    shared_ids: List[int] = []
    baseline_vals: List[float] = []
    target_vals: List[float] = []
    for cfg_id, br in baseline_by_id.items():
        tr = target_by_id.get(cfg_id)
        if tr is None:
            continue
        if not (_valid_gflops(br) and _valid_gflops(tr)):
            continue
        shared_ids.append(cfg_id)
        baseline_vals.append(float(br["gflops_mean"]))
        target_vals.append(float(tr["gflops_mean"]))

    spearman = _spearman(baseline_vals, target_vals)

    overlap = float("nan")
    if shared_ids:
        b_sorted = sorted(
            ((int(r["config_id"]), float(r["gflops_mean"])) for r in baseline_rows if _valid_gflops(r)),
            key=lambda x: x[1],
            reverse=True,
        )
        t_sorted = sorted(
            ((int(r["config_id"]), float(r["gflops_mean"])) for r in target_rows if _valid_gflops(r)),
            key=lambda x: x[1],
            reverse=True,
        )
        kb = {cfg_id for cfg_id, _ in b_sorted[: max(1, top_k)]}
        kt = {cfg_id for cfg_id, _ in t_sorted[: max(1, top_k)]}
        den = max(1, min(len(kb), len(kt)))
        overlap = len(kb.intersection(kt)) / den

    return {
        "shared_configs": len(shared_ids),
        "spearman_gflops": spearman,
        "topk_overlap": overlap,
    }


def _make_out_dir() -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = ROOT / "data" / "validation" / f"{ts}__fem_parametric_matrix"
    out.mkdir(parents=True, exist_ok=True)
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description="Cross-platform FEM parametric ranking matrix vs baseline.")
    ap.add_argument("--backends", default="cpu,cuda,hip,opencl,metal,amd,intel")
    ap.add_argument("--baseline", default="opencl")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--n-configs", type=int, default=20)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--repeats", type=int, default=2)
    ap.add_argument("--top-k", type=int, default=5)
    ap.add_argument("--execution-policy", choices=["native_only", "allow_fallback"], default="native_only")
    ap.add_argument("--memory-budget-mb", type=int, default=0)
    ap.add_argument("--memory-budget-fraction", type=float, default=0.35)
    ap.add_argument("--eval-cache-size", type=int, default=1024)
    ap.add_argument("--screening-repeats", type=int, default=1)
    ap.add_argument("--screening-prune-factor", type=float, default=0.55)

    ap.add_argument("--n-elements-range", default="20000:200000")
    ap.add_argument("--n-qp-range", default="1:8")
    ap.add_argument("--element-types", default="tet4,hex8")
    ap.add_argument("--operators", default="diffusion,mass,convection,diffusion_mass,diffusion_convection_mass")
    ap.add_argument("--dtypes", default="float32")
    ap.add_argument("--variants", default="qss,sqs,ssq")
    ap.add_argument("--workgroup-sizes", default="32,64,128,256")

    args = ap.parse_args()

    args.backends = _parse_backends(args.backends)
    args.baseline = str(args.baseline).lower().strip()
    if args.baseline not in ALL_BACKENDS:
        raise ValueError(f"Unsupported baseline backend: {args.baseline}")
    if args.baseline not in args.backends:
        args.backends.append(args.baseline)

    args.n_elements_range = _parse_range(args.n_elements_range)
    args.n_qp_range = _parse_range(args.n_qp_range)
    args.element_types = _parse_csv(args.element_types)
    args.operators = _parse_csv(args.operators)
    args.dtypes = _parse_csv(args.dtypes)
    args.variants = _parse_csv(args.variants)
    args.workgroup_sizes = [int(x) for x in _parse_csv(args.workgroup_sizes)]

    configs = _sample_configs(
        seed=args.seed,
        count=args.n_configs,
        n_elements_range=args.n_elements_range,
        n_qp_range=args.n_qp_range,
        element_types=args.element_types,
        operators=args.operators,
        dtypes=args.dtypes,
        variants=args.variants,
        workgroup_sizes=args.workgroup_sizes,
        flag_values=[0, 1],
    )

    out_dir = _make_out_dir()

    backend_payloads: List[Dict[str, Any]] = []
    for b in args.backends:
        payload = _evaluate_backend(args=args, backend=b, configs=configs)
        backend_payloads.append(payload)

    baseline_payload = next((p for p in backend_payloads if p["backend"] == args.baseline), None)

    comparisons: List[Dict[str, Any]] = []
    if baseline_payload is not None and baseline_payload.get("available"):
        baseline_rows = baseline_payload.get("rows", [])
        for payload in backend_payloads:
            if payload["backend"] == args.baseline:
                continue
            if not payload.get("available"):
                comparisons.append(
                    {
                        "backend": payload["backend"],
                        "available": 0,
                        "compare": None,
                        "error": payload.get("error", ""),
                    }
                )
                continue
            cmp = _compare_to_baseline(baseline_rows, payload.get("rows", []), args.top_k)
            comparisons.append(
                {
                    "backend": payload["backend"],
                    "available": 1,
                    "compare": cmp,
                    "error": "",
                }
            )

    summary = {
        "baseline": args.baseline,
        "configs_count": len(configs),
        "backends": args.backends,
        "backend_payloads": backend_payloads,
        "comparisons_vs_baseline": comparisons,
    }

    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")

    print("=== FEM PARAMETRIC MATRIX ===")
    print(f"out_dir: {out_dir}")
    print(f"baseline: {args.baseline}")
    print(f"configs: {len(configs)}")
    print()
    for payload in backend_payloads:
        status = "OK" if payload.get("available") else "FAIL"
        print(
            f"[{status}] backend={payload['backend']} "
            f"resolved={payload.get('resolved_backend')} "
            f"mode={payload.get('execution_mode')} device={payload.get('device')}"
        )
        if not payload.get("available"):
            print(f"  error={payload.get('error')}")

    if comparisons:
        print()
        print("--- vs baseline ---")
        for item in comparisons:
            if not item.get("available"):
                print(f"backend={item['backend']}: unavailable ({item.get('error', '')})")
                continue
            cmp = item["compare"]
            print(
                f"backend={item['backend']}: shared={cmp['shared_configs']}, "
                f"spearman={cmp['spearman_gflops']:.4f}, "
                f"topk_overlap={cmp['topk_overlap']:.4f}"
            )


if __name__ == "__main__":
    main()
