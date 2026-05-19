#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import asdict
from datetime import datetime
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from device_resolution import resolve_device_index
from optimization.firefly import FireflyConfig, FireflyOptimizer
from optimization.objectives import ParetoBrightness, WeightedSumBrightness, parse_objective_terms
from optimization.problems import (
    AuthorAssemblyProblem,
    AuthorAssemblyProblemConfig,
    FemIntegrationProblem,
    FemIntegrationProblemConfig,
    FemParametricProblem,
    FemParametricProblemConfig,
    GpuFmaProblem,
    GpuFmaProblemConfig,
    GpuMemoryProblem,
    GpuMemoryProblemConfig,
)
from analysis.filip_article_plots import generate_article_plots
from analysis.optimization_diagnostics import generate_optimization_diagnostics


def _parse_range(text: str, tp: type[int] | type[float]):
    parts = [p.strip() for p in text.split(":") if p.strip()]
    if len(parts) != 2:
        raise ValueError(f"Invalid range: {text}. Expected 'min:max'")
    return tp(parts[0]), tp(parts[1])


def _parse_list_csv(text: str) -> list[str]:
    return [x.strip() for x in text.split(",") if x.strip()]


def _make_out_dir(problem: str, backend: str) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = ROOT / "data" / "optimization" / f"{ts}__{problem}__backend-{backend}"
    out.mkdir(parents=True, exist_ok=True)
    return out


def _best_overall_payload(
    *,
    best_config: dict[str, object],
    best_metrics: dict[str, float],
    best_brightness: float,
    backend: str,
    resolved_backend: str,
    execution_mode: str,
    device: str,
) -> dict[str, object]:
    payload: dict[str, object] = {
        "config": dict(best_config),
        "metrics": dict(best_metrics),
        "score": float(best_brightness),
        "best_brightness": float(best_brightness),
        "backend": backend,
        "resolved_backend": resolved_backend,
        "execution_mode": execution_mode,
        "device": device,
    }
    for key in ("internal_ns_per_elem", "ns_per_unit", "gflops_mean", "gbps_mean", "mapping_score"):
        if key in best_metrics:
            payload[f"score_{key}"] = best_metrics[key]
    return payload


def main() -> None:
    ap = argparse.ArgumentParser(description="Firefly-based autotuner for microbenchmarks.")
    ap.add_argument(
        "--problem",
        choices=["gpu_memory", "gpu_fma", "fem_integration", "fem_parametric", "author_assembly"],
        required=True,
    )
    ap.add_argument(
        "--backend",
        choices=["cpu", "cuda", "hip", "metal", "opencl", "amd", "intel"],
        required=True,
    )
    ap.add_argument("--device-index", type=int, default=0)

    # Problem-level measurement controls
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--min-elapsed-s", type=float, default=1e-4)
    ap.add_argument("--max-cv", type=float, default=0.05)

    # Memory ranges
    ap.add_argument("--size-mb-range", default="4:1024")
    ap.add_argument("--iters-range", default="5:200")
    ap.add_argument("--transfer-kinds", default="device_to_device,host_to_device,device_to_host")

    # FMA ranges
    ap.add_argument("--n-elements-m-range", default="0.25:16.0")
    ap.add_argument("--iters-inner-range", default="200:10000")
    ap.add_argument("--roofline-peak-gflops", type=float, default=None)
    ap.add_argument("--roofline-peak-bw-gbps", type=float, default=None)
    ap.add_argument("--arithmetic-intensity", type=float, default=None)

    # FEM integration ranges/options
    ap.add_argument("--fem-n-elements-range", default="20000:1000000")
    ap.add_argument("--fem-n-qp-range", default="1:8")
    ap.add_argument("--fem-element-types", default="tet4,hex8")
    ap.add_argument(
        "--fem-operators",
        default="diffusion,mass,convection,diffusion_mass,diffusion_convection_mass",
    )
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
    ap.add_argument(
        "--fem-mapped-max-n-fma-gpu",
        "--fem-surrogate-max-n-fma-gpu",
        dest="fem_mapped_max_n_fma_gpu",
        type=int,
        default=4000000,
    )
    ap.add_argument(
        "--fem-mapped-max-n-fma-light",
        "--fem-surrogate-max-n-fma-light",
        dest="fem_mapped_max_n_fma_light",
        type=int,
        default=1000000,
    )
    ap.add_argument(
        "--fem-mapped-max-buffer-mb-gpu",
        "--fem-surrogate-max-buffer-mb-gpu",
        dest="fem_mapped_max_buffer_mb_gpu",
        type=int,
        default=128,
    )
    ap.add_argument(
        "--fem-mapped-max-buffer-mb-light",
        "--fem-surrogate-max-buffer-mb-light",
        dest="fem_mapped_max_buffer_mb_light",
        type=int,
        default=64,
    )
    ap.add_argument(
        "--fem-mapped-max-mem-iters",
        "--fem-surrogate-max-mem-iters",
        dest="fem_mapped_max_mem_iters",
        type=int,
        default=256,
    )
    ap.add_argument(
        "--fem-mapped-max-inner-iters-gpu",
        "--fem-surrogate-max-inner-iters-gpu",
        dest="fem_mapped_max_inner_iters_gpu",
        type=int,
        default=10000,
    )
    ap.add_argument(
        "--fem-mapped-max-inner-iters-light",
        "--fem-surrogate-max-inner-iters-light",
        dest="fem_mapped_max_inner_iters_light",
        type=int,
        default=4096,
    )
    ap.add_argument("--fem-execution-policy", choices=["native_only", "allow_fallback"], default="native_only")
    ap.add_argument("--fem-memory-budget-mb", type=int, default=0)
    ap.add_argument("--fem-memory-budget-fraction", type=float, default=0.35)
    ap.add_argument("--fem-eval-cache-size", type=int, default=2048)
    ap.add_argument("--fem-screening-repeats", type=int, default=1)
    ap.add_argument("--fem-screening-prune-factor", type=float, default=0.55)
    ap.add_argument("--fem-record-raw-artifacts", action="store_true")
    ap.add_argument("--no-article-plots", action="store_true")

    # Author assembly-like ranges/options
    ap.add_argument("--assembly-n-elements-range", default="10000:250000")
    ap.add_argument("--assembly-n-qp-range", default="1:8")
    ap.add_argument("--assembly-n-dofs-choices", default="4,6,8")
    ap.add_argument("--assembly-variants", default="qss,sqs,ssq")
    ap.add_argument("--assembly-workspace-choices", default="0,1")
    ap.add_argument("--assembly-scatter-choices", default="0,1")
    ap.add_argument("--assembly-padding-choices", default="0,1")
    ap.add_argument("--assembly-dtypes", default="float32")

    # FA config
    ap.add_argument("--population", type=int, default=16)
    ap.add_argument("--iterations", type=int, default=25)
    ap.add_argument("--alpha", type=float, default=0.25)
    ap.add_argument("--beta0", type=float, default=1.0)
    ap.add_argument("--gamma", type=float, default=1.0)
    ap.add_argument("--alpha-damp", type=float, default=0.97)
    ap.add_argument("--seed", type=int, default=42)
    ap.add_argument("--elite-keep", type=int, default=1)

    # Objectives
    ap.add_argument("--objective-mode", choices=["weighted", "pareto"], default="weighted")
    ap.add_argument(
        "--objectives",
        default="",
        help="metric:sense:weight CSV, e.g. 'gbps_mean:max:1.0,j_per_gb:min:0.2'",
    )

    args = ap.parse_args()
    resolved_device_index, device_resolution_reason = resolve_device_index(args.backend, int(args.device_index))

    out_dir = _make_out_dir(args.problem, args.backend)

    try:
        if args.problem == "gpu_memory":
            size_min, size_max = _parse_range(args.size_mb_range, float)
            it_min, it_max = _parse_range(args.iters_range, int)
            transfer_kinds = _parse_list_csv(args.transfer_kinds)

            problem = GpuMemoryProblem(
                GpuMemoryProblemConfig(
                    backend=args.backend,
                    device_index=resolved_device_index,
                    repeats=args.repeats,
                    min_elapsed_s=args.min_elapsed_s,
                    max_cv=args.max_cv,
                    size_mb_min=size_min,
                    size_mb_max=size_max,
                    iters_min=it_min,
                    iters_max=it_max,
                    transfer_kinds=transfer_kinds,
                )
            )
            objective_spec = args.objectives or "gbps_mean:max:1.0,j_per_gb:min:0.2"
            if args.backend == "metal":
                print(
                    "[INFO] Metal safety caps enabled for memory problem: "
                    f"size_mb<= {problem.cfg.hard_size_mb_cap}, "
                    f"iters_inner<= {problem.cfg.hard_iters_cap}"
                )
                print(
                    "[INFO] Effective search ranges: "
                    f"size_mb={problem.effective_size_mb_min:.6f}:{problem.effective_size_mb_max:.6f}, "
                    f"iters_inner={problem.effective_iters_min}:{problem.effective_iters_max}"
                )
                for note in problem.range_adjustments:
                    print(f"[INFO] Range adjustment: {note}")

        elif args.problem == "gpu_fma":
            n_min, n_max = _parse_range(args.n_elements_m_range, float)
            it_min, it_max = _parse_range(args.iters_inner_range, int)

            problem = GpuFmaProblem(
                GpuFmaProblemConfig(
                    backend=args.backend,
                    device_index=resolved_device_index,
                    repeats=args.repeats,
                    min_elapsed_s=args.min_elapsed_s,
                    max_cv=args.max_cv,
                    n_elements_m_min=n_min,
                    n_elements_m_max=n_max,
                    iters_inner_min=it_min,
                    iters_inner_max=it_max,
                    roofline_peak_gflops=args.roofline_peak_gflops,
                    roofline_peak_bw_gbps=args.roofline_peak_bw_gbps,
                    arithmetic_intensity=args.arithmetic_intensity,
                )
            )
            objective_spec = args.objectives or "gflops_mean:max:1.0,j_per_gflop:min:0.3,edp:min:0.2"
        elif args.problem == "fem_integration":
            n_elem_min, n_elem_max = _parse_range(args.fem_n_elements_range, int)
            n_qp_min, n_qp_max = _parse_range(args.fem_n_qp_range, int)
            element_types = _parse_list_csv(args.fem_element_types)
            operators = _parse_list_csv(args.fem_operators)
            dtypes = _parse_list_csv(args.fem_dtypes)

            problem = FemIntegrationProblem(
                FemIntegrationProblemConfig(
                    backend=args.backend,
                    device_index=resolved_device_index,
                    repeats=args.repeats,
                    min_elapsed_s=args.min_elapsed_s,
                    max_cv=args.max_cv,
                    n_elements_min=n_elem_min,
                    n_elements_max=n_elem_max,
                    n_qp_min=n_qp_min,
                    n_qp_max=n_qp_max,
                    element_types=element_types,
                    operators=operators,
                    dtypes=dtypes,
                )
            )
            objective_spec = args.objectives or "gflops_mean:max:1.0,gbps_mean:max:0.2,cv_gflops:min:0.2"
        elif args.problem == "author_assembly":
            n_elem_min, n_elem_max = _parse_range(args.assembly_n_elements_range, int)
            n_qp_min, n_qp_max = _parse_range(args.assembly_n_qp_range, int)
            n_dofs = [int(x) for x in _parse_list_csv(args.assembly_n_dofs_choices)]
            variants = _parse_list_csv(args.assembly_variants)
            workspace_choices = [int(x) for x in _parse_list_csv(args.assembly_workspace_choices)]
            scatter_choices = [int(x) for x in _parse_list_csv(args.assembly_scatter_choices)]
            padding_choices = [int(x) for x in _parse_list_csv(args.assembly_padding_choices)]
            dtypes = _parse_list_csv(args.assembly_dtypes)

            problem = AuthorAssemblyProblem(
                AuthorAssemblyProblemConfig(
                    backend=args.backend,
                    device_index=resolved_device_index,
                    repeats=args.repeats,
                    min_elapsed_s=args.min_elapsed_s,
                    max_cv=args.max_cv,
                    n_elements_min=n_elem_min,
                    n_elements_max=n_elem_max,
                    n_qp_min=n_qp_min,
                    n_qp_max=n_qp_max,
                    n_dofs_choices=n_dofs,
                    variant_choices=variants,
                    workspace_choices=workspace_choices,
                    scatter_choices=scatter_choices,
                    padding_choices=padding_choices,
                    dtypes=dtypes,
                    mapped_max_n_fma_gpu=args.fem_mapped_max_n_fma_gpu,
                    mapped_max_n_fma_light=args.fem_mapped_max_n_fma_light,
                    mapped_max_buffer_mb_gpu=args.fem_mapped_max_buffer_mb_gpu,
                    mapped_max_buffer_mb_light=args.fem_mapped_max_buffer_mb_light,
                    mapped_max_mem_iters=args.fem_mapped_max_mem_iters,
                    mapped_max_inner_iters_gpu=args.fem_mapped_max_inner_iters_gpu,
                    mapped_max_inner_iters_light=args.fem_mapped_max_inner_iters_light,
                )
            )
            objective_spec = (
                args.objectives
                or "gflops_mean:max:1.0,ai_flop_per_byte_mean:max:0.2,j_per_gflop:min:0.2,cv_gflops:min:0.2"
            )
        else:
            n_elem_min, n_elem_max = _parse_range(args.fem_n_elements_range, int)
            n_qp_min, n_qp_max = _parse_range(args.fem_n_qp_range, int)
            element_types = _parse_list_csv(args.fem_element_types)
            operators = _parse_list_csv(args.fem_operators)
            dtypes = _parse_list_csv(args.fem_dtypes)
            variants = _parse_list_csv(args.fem_variant_choices)
            workgroup_sizes = [int(x) for x in _parse_list_csv(args.fem_workgroup_sizes)]
            ws_pde = [int(x) for x in _parse_list_csv(args.fem_use_workspace_pde_choices)]
            ws_geo = [int(x) for x in _parse_list_csv(args.fem_use_workspace_geo_choices)]
            ws_shape = [int(x) for x in _parse_list_csv(args.fem_use_workspace_shape_choices)]
            ws_stiff = [int(x) for x in _parse_list_csv(args.fem_use_workspace_stiff_choices)]
            paddings = [int(x) for x in _parse_list_csv(args.fem_padding_choices)]
            casfd = [int(x) for x in _parse_list_csv(args.fem_compute_all_shape_der_choices)]
            coal_read = [int(x) for x in _parse_list_csv(args.fem_coal_read_choices)]
            coal_write = [int(x) for x in _parse_list_csv(args.fem_coal_write_choices)]

            problem = FemParametricProblem(
                FemParametricProblemConfig(
                    backend=args.backend,
                    device_index=resolved_device_index,
                    repeats=args.repeats,
                    min_elapsed_s=args.min_elapsed_s,
                    max_cv=args.max_cv,
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
                    execution_policy=args.fem_execution_policy,
                    mapped_max_n_fma_gpu=args.fem_mapped_max_n_fma_gpu,
                    mapped_max_n_fma_light=args.fem_mapped_max_n_fma_light,
                    mapped_max_buffer_mb_gpu=args.fem_mapped_max_buffer_mb_gpu,
                    mapped_max_buffer_mb_light=args.fem_mapped_max_buffer_mb_light,
                    mapped_max_mem_iters=args.fem_mapped_max_mem_iters,
                    mapped_max_inner_iters_gpu=args.fem_mapped_max_inner_iters_gpu,
                    mapped_max_inner_iters_light=args.fem_mapped_max_inner_iters_light,
                    memory_budget_mb=args.fem_memory_budget_mb,
                    memory_budget_fraction=args.fem_memory_budget_fraction,
                    eval_cache_size=args.fem_eval_cache_size,
                    screening_repeats=args.fem_screening_repeats,
                    screening_prune_factor=args.fem_screening_prune_factor,
                    record_raw_artifacts=args.fem_record_raw_artifacts,
                )
            )
            objective_spec = (
                args.objectives
                or "gflops_mean:max:1.0,j_per_gflop:min:0.3,cv_gflops:min:0.2,mapping_score:max:0.05"
            )
    except Exception as e:
        print(f"[ERROR] Cannot initialize problem/backend: {e}")
        raise SystemExit(2)

    terms = parse_objective_terms(objective_spec)
    if args.objective_mode == "weighted":
        objective_model = WeightedSumBrightness(terms)
    else:
        objective_model = ParetoBrightness(terms)

    ff_cfg = FireflyConfig(
        population_size=args.population,
        iterations=args.iterations,
        alpha=args.alpha,
        beta0=args.beta0,
        gamma=args.gamma,
        alpha_damp=args.alpha_damp,
        seed=args.seed,
        elite_keep=args.elite_keep,
    )

    optimizer = FireflyOptimizer(
        problem=problem,
        objective_model=objective_model,
        config=ff_cfg,
        out_dir=out_dir,
    )

    try:
        result = optimizer.run()
    finally:
        problem.close()

    workflow_name = "filip_firefly" if args.problem == "fem_parametric" else "firefly_optimization"
    if args.problem == "author_assembly":
        workflow_name = "author_assembly_firefly"
    summary_txt = {
        "workflow": workflow_name,
        "experiment_class": "native_performance_campaign",
        "mode_label": "Native FEM performance campaign" if args.problem == "fem_parametric" else "Native parametric optimization campaign",
        "method": "firefly",
        "optimizer": "firefly",
        "problem": problem.name,
        "objective_mode": args.objective_mode,
        "objectives": objective_spec,
        "backend": args.backend,
        "resolved_backend": getattr(problem.mode, "resolved_backend", args.backend),
        "execution_mode": getattr(problem.mode, "execution_mode", "native"),
        "device": getattr(problem.mode, "device_name", args.backend),
        "device_index_requested": int(args.device_index),
        "device_index_used": int(resolved_device_index),
        "device_resolution_reason": str(device_resolution_reason),
        "best_brightness": result.best_brightness,
        "best_config": result.best_config,
        "best_metrics": result.best_metrics,
        "best_overall": _best_overall_payload(
            best_config=result.best_config,
            best_metrics=result.best_metrics,
            best_brightness=result.best_brightness,
            backend=args.backend,
            resolved_backend=getattr(problem.mode, "resolved_backend", args.backend),
            execution_mode=getattr(problem.mode, "execution_mode", "native"),
            device=getattr(problem.mode, "device_name", args.backend),
        ),
        "pareto_size": len(result.pareto_front),
        "out_dir": str(result.out_dir),
    }
    summary_path = result.out_dir / "summary.json"
    if summary_path.exists():
        merged = json.loads(summary_path.read_text(encoding="utf-8"))
    else:
        merged = {}
    merged.update(summary_txt)
    merged["firefly"] = {
        "config": asdict(ff_cfg),
        "summary": {
            "best_brightness": float(merged.get("best_brightness", result.best_brightness)),
            "unique_evaluations": int(merged.get("unique_evaluations", 0) or 0),
            "feasible_unique_evaluations": int(merged.get("feasible_unique_evaluations", 0) or 0),
            "pareto_size": int(merged.get("pareto_size", len(result.pareto_front)) or 0),
        },
        "artifacts": {
            "evaluations_jsonl": str(result.out_dir / "evaluations.jsonl"),
            "iterations_jsonl": str(result.out_dir / "iterations.jsonl"),
            "pareto_front_jsonl": str(result.out_dir / "pareto_front.jsonl"),
            "best_json": str(result.out_dir / "best.json"),
        },
    }
    summary_path.write_text(json.dumps(merged, indent=2, ensure_ascii=True), encoding="utf-8")
    try:
        diag_plots = generate_optimization_diagnostics(result.out_dir)
        if diag_plots:
            summary_txt["plots"] = diag_plots
            merged["plots"] = {**(merged.get("plots") or {}), **diag_plots}
    except Exception as e:
        summary_txt["optimization_diagnostics_error"] = str(e)
        merged["optimization_diagnostics_error"] = str(e)
    if args.problem == "fem_parametric" and not args.no_article_plots:
        try:
            plot_summary = generate_article_plots(result.out_dir)
            summary_txt["article_plots_dir"] = plot_summary.get("plots_dir", "")
            summary_txt["article_plots"] = plot_summary.get("generated_plots", [])
            summary_txt["figure_set_dir"] = plot_summary.get("thesis_core_dir", plot_summary.get("plots_dir", ""))
            summary_txt["figure_set_plots"] = plot_summary.get("figure_paths", plot_summary.get("generated_plots", []))
            summary_txt["figure_appendix_dir"] = plot_summary.get("appendix_dir", "")
            summary_txt["figure_appendix_plots"] = plot_summary.get("appendix_figure_paths", plot_summary.get("appendix_plots", []))
            summary_txt["figure_manifest_path"] = plot_summary.get("manifest_path", "")
            merged["article_plots_dir"] = summary_txt["article_plots_dir"]
            merged["article_plots"] = summary_txt["article_plots"]
            merged["figure_set_dir"] = summary_txt["figure_set_dir"]
            merged["figure_set_plots"] = summary_txt["figure_set_plots"]
            merged["figure_appendix_dir"] = summary_txt["figure_appendix_dir"]
            merged["figure_appendix_plots"] = summary_txt["figure_appendix_plots"]
            merged["figure_manifest_path"] = summary_txt["figure_manifest_path"]
        except Exception as e:
            summary_txt["article_plots_error"] = str(e)
            merged["article_plots_error"] = str(e)
    summary_path.write_text(json.dumps(merged, indent=2, ensure_ascii=True), encoding="utf-8")
    print("=== FIREFLY OPTIMIZATION DONE ===")
    print(json.dumps(summary_txt, indent=2, ensure_ascii=True))


if __name__ == "__main__":
    main()
