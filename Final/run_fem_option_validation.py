#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from datetime import datetime
import json
import math
import os
from pathlib import Path
import sys
from typing import Any

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.provenance import collect_runtime_provenance, sha256_json
from device_resolution import resolve_device_index
from optimization.problems import FemParametricProblem, FemParametricProblemConfig

VARIANTS = ["qss", "sqs", "ssq"]


def _row(
    *,
    coal_read: int = 1,
    coal_write: int = 1,
    compute_all_shape_fun_der: int = 0,
    use_workspace_for_pde_coeff: int = 0,
    use_workspace_for_geo_data: int = 0,
    use_workspace_for_shape_fun: int = 0,
    use_workspace_for_stiff_mat: int = 0,
    padding: int = 1,
) -> list[int]:
    return [
        int(coal_read),
        int(coal_write),
        int(compute_all_shape_fun_der),
        int(use_workspace_for_pde_coeff),
        int(use_workspace_for_geo_data),
        int(use_workspace_for_shape_fun),
        int(use_workspace_for_stiff_mat),
        1 if int(padding) == 0 else 0,
        1 if int(padding) == 1 else 0,
    ]


EXACT_LIKE_ROW = _row()
WORKSPACE_HEAVY_ROW = _row(
    use_workspace_for_pde_coeff=1,
    use_workspace_for_geo_data=1,
    use_workspace_for_shape_fun=1,
    use_workspace_for_stiff_mat=1,
)
STREAMING_ROW = _row(coal_read=0, coal_write=0, padding=0)
COMPUTE_HEAVY_ROW = _row(compute_all_shape_fun_der=1)
MEMORY_COMPACT_ROW = _row(
    use_workspace_for_pde_coeff=1,
    use_workspace_for_geo_data=1,
    use_workspace_for_shape_fun=1,
    use_workspace_for_stiff_mat=1,
    padding=1,
)

PROBES: list[dict[str, Any]] = [
    {
        "id": "read_coalescing",
        "label": "Read coalescing",
        "category": "memory_access",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(coal_read=0),
        "baseline_label": "coalesced_read_on",
        "toggled_label": "coalesced_read_off",
        "alignment_control": "coal_read",
        "related_controls": ["coal_read"],
        "rationale": "Checks whether contiguous global-memory reads are rewarded on the current backend.",
    },
    {
        "id": "write_coalescing",
        "label": "Write coalescing",
        "category": "memory_access",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(coal_write=0),
        "baseline_label": "coalesced_write_on",
        "toggled_label": "coalesced_write_off",
        "alignment_control": "coal_write",
        "related_controls": ["coal_write"],
        "rationale": "Measures the cost of scattered output writes relative to coalesced output stores.",
    },
    {
        "id": "shape_derivative_strategy",
        "label": "Derivative computation strategy",
        "category": "compute_memory_tradeoff",
        "baseline": EXACT_LIKE_ROW,
        "toggled": COMPUTE_HEAVY_ROW,
        "baseline_label": "reuse_or_partial_derivatives",
        "toggled_label": "compute_all_derivatives",
        "alignment_control": "compute_all_shape_fun_der",
        "related_controls": ["compute_all_shape_fun_der"],
        "rationale": "Tests the compute-vs-memory tradeoff for evaluating all shape derivatives eagerly.",
    },
    {
        "id": "pde_coeff_cache",
        "label": "PDE coefficient workspace",
        "category": "workspace_reuse",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(use_workspace_for_pde_coeff=1),
        "baseline_label": "pde_coeff_direct",
        "toggled_label": "pde_coeff_workspace",
        "alignment_control": "use_workspace_for_pde_coeff",
        "related_controls": ["use_workspace_for_pde_coeff"],
        "rationale": "Isolates reuse of PDE coefficients through a workspace buffer.",
    },
    {
        "id": "geo_cache",
        "label": "Geometry workspace",
        "category": "workspace_reuse",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(use_workspace_for_geo_data=1),
        "baseline_label": "geo_direct",
        "toggled_label": "geo_workspace",
        "alignment_control": "use_workspace_for_geo_data",
        "related_controls": ["use_workspace_for_geo_data"],
        "rationale": "Checks whether temporary geometry staging helps this backend.",
    },
    {
        "id": "shape_fun_cache",
        "label": "Shape-function workspace",
        "category": "workspace_reuse",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(use_workspace_for_shape_fun=1),
        "baseline_label": "shape_fun_direct",
        "toggled_label": "shape_fun_workspace",
        "alignment_control": "use_workspace_for_shape_fun",
        "related_controls": ["use_workspace_for_shape_fun"],
        "rationale": "Measures reuse of precomputed shape-function values through workspace staging.",
    },
    {
        "id": "stiffness_cache",
        "label": "Stiffness workspace",
        "category": "workspace_reuse",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(use_workspace_for_stiff_mat=1),
        "baseline_label": "stiffness_direct",
        "toggled_label": "stiffness_workspace",
        "alignment_control": "use_workspace_for_stiff_mat",
        "related_controls": ["use_workspace_for_stiff_mat"],
        "rationale": "Measures whether staging partial stiffness terms in workspace reduces total kernel cost.",
    },
    {
        "id": "workspace_padding",
        "label": "Workspace padding",
        "category": "layout",
        "baseline": EXACT_LIKE_ROW,
        "toggled": _row(padding=0),
        "baseline_label": "padding_on",
        "toggled_label": "padding_off",
        "alignment_control": "padding",
        "related_controls": ["padding"],
        "rationale": "Captures sensitivity to padded workspace layouts and alignment effects.",
    },
    {
        "id": "workspace_bundle",
        "label": "Workspace bundle profile",
        "category": "profile",
        "baseline": EXACT_LIKE_ROW,
        "toggled": WORKSPACE_HEAVY_ROW,
        "baseline_label": "minimal_workspace_profile",
        "toggled_label": "workspace_heavy_profile",
        "alignment_control": "",
        "related_controls": [
            "use_workspace_for_pde_coeff",
            "use_workspace_for_geo_data",
            "use_workspace_for_shape_fun",
            "use_workspace_for_stiff_mat",
        ],
        "rationale": "Aggregates all workspace reuse controls into one exact-like profile comparison.",
    },
    {
        "id": "streaming_profile",
        "label": "Streaming memory profile",
        "category": "profile",
        "baseline": EXACT_LIKE_ROW,
        "toggled": STREAMING_ROW,
        "baseline_label": "coalesced_profile",
        "toggled_label": "streaming_relaxed_profile",
        "alignment_control": "",
        "related_controls": ["coal_read", "coal_write", "padding"],
        "rationale": "Tests a more streaming-friendly but less tightly packed memory access pattern.",
    },
    {
        "id": "compute_memory_balance",
        "label": "Compute vs workspace balance",
        "category": "profile",
        "baseline": WORKSPACE_HEAVY_ROW,
        "toggled": _row(
            compute_all_shape_fun_der=1,
            use_workspace_for_pde_coeff=1,
            use_workspace_for_geo_data=1,
            use_workspace_for_shape_fun=1,
            use_workspace_for_stiff_mat=1,
        ),
        "baseline_label": "workspace_reuse_balance",
        "toggled_label": "workspace_plus_compute_profile",
        "alignment_control": "",
        "related_controls": [
            "compute_all_shape_fun_der",
            "use_workspace_for_pde_coeff",
            "use_workspace_for_geo_data",
            "use_workspace_for_shape_fun",
            "use_workspace_for_stiff_mat",
        ],
        "rationale": "Separates pure reuse benefits from the extra compute-heavy derivative strategy.",
    },
    {
        "id": "memory_compact_profile",
        "label": "Memory-compact profile",
        "category": "profile",
        "baseline": STREAMING_ROW,
        "toggled": MEMORY_COMPACT_ROW,
        "baseline_label": "minimal_compaction",
        "toggled_label": "memory_compact_profile",
        "alignment_control": "",
        "related_controls": [
            "coal_read",
            "coal_write",
            "use_workspace_for_pde_coeff",
            "use_workspace_for_geo_data",
            "use_workspace_for_shape_fun",
            "use_workspace_for_stiff_mat",
            "padding",
        ],
        "rationale": "Approximates an exact-like memory-conscious profile using coalescing plus workspace reuse and padding.",
    },
]


def _try_import_matplotlib():
    mpl_cfg = ROOT / ".cache" / "matplotlib"
    mpl_cfg.mkdir(parents=True, exist_ok=True)
    os.environ.setdefault("MPLCONFIGDIR", str(mpl_cfg))
    try:
        import matplotlib

        matplotlib.use("Agg")
        import matplotlib.pyplot as plt  # type: ignore

        return plt
    except Exception:
        return None


def _write_overview_plots(summary: dict[str, Any], records: list[dict[str, Any]], out_dir: Path) -> dict[str, str]:
    plt = _try_import_matplotlib()
    if plt is None:
        return {}

    generated: dict[str, str] = {}

    probe_summary = summary.get("probe_summary") or {}
    probe_rows: list[tuple[str, float]] = []
    for probe_id, values in probe_summary.items():
        if not isinstance(values, dict):
            continue
        try:
            ratio = float(values.get("mean_delta_ratio", float("nan")))
        except Exception:
            ratio = float("nan")
        probe_rows.append((str(values.get("probe_label", probe_id)), ratio))
    probe_rows.sort(key=lambda item: item[1] if math.isfinite(item[1]) else float("inf"))
    if probe_rows:
        fig, ax = plt.subplots(figsize=(10, max(4.5, 0.4 * len(probe_rows) + 1.5)))
        labels = [item[0] for item in probe_rows]
        ratios = [item[1] for item in probe_rows]
        ys = list(range(len(labels)))
        colors = ["#15803d" if math.isfinite(v) and v < 1.0 else "#b45309" for v in ratios]
        ax.barh(ys, ratios, color=colors, alpha=0.9)
        ax.axvline(1.0, color="#334155", linestyle="--", linewidth=1.0)
        ax.set_yticks(ys)
        ax.set_yticklabels(labels, fontsize=8)
        ax.set_xlabel("Mean delta ratio (toggled / baseline)")
        ax.set_title("FEM option validation probes")
        ax.grid(True, axis="x", alpha=0.25)
        fig.tight_layout()
        out = out_dir / "fem_option_validation_probes.png"
        fig.savefig(out, dpi=180)
        plt.close(fig)
        generated["probe_plot"] = str(out)

    if records:
        probe_catalog = summary.get("probe_catalog") or {}
        probe_ids = list(probe_catalog.keys()) or sorted(
            {str(row.get("probe_id", "")) for row in records if str(row.get("probe_id", "")).strip()}
        )
        combo_keys = sorted(
            {
                f"{str(row.get('operator', '')).strip()}:{str(row.get('variant', '')).strip()}"
                for row in records
                if str(row.get("operator", "")).strip() and str(row.get("variant", "")).strip()
            }
        )
        if probe_ids and combo_keys:
            matrix: list[list[float]] = []
            for combo in combo_keys:
                operator, variant = combo.split(":", 1)
                row_vals: list[float] = []
                for probe_id in probe_ids:
                    vals: list[float] = []
                    for row in records:
                        if str(row.get("operator", "")).strip() != operator:
                            continue
                        if str(row.get("variant", "")).strip() != variant:
                            continue
                        if str(row.get("probe_id", "")).strip() != probe_id:
                            continue
                        try:
                            val = float(row.get("delta_ratio", float("nan")))
                        except Exception:
                            val = float("nan")
                        if math.isfinite(val):
                            vals.append(val)
                    row_vals.append(sum(vals) / len(vals) if vals else float("nan"))
                matrix.append(row_vals)
            flat = [val for row in matrix for val in row if math.isfinite(val)]
            if flat:
                import numpy as np  # type: ignore

                fig, ax = plt.subplots(figsize=(max(8, 0.6 * len(probe_ids) + 2), max(4, 0.45 * len(combo_keys) + 2)))
                vmax = max(max(flat), 1.0)
                vmin = min(min(flat), 1.0)
                img = ax.imshow(np.array(matrix, dtype=float), aspect="auto", cmap="coolwarm", vmin=vmin, vmax=vmax)
                fig.colorbar(img, ax=ax, label="Mean delta ratio")
                ax.set_xticks(list(range(len(probe_ids))))
                ax.set_xticklabels(
                    [str((probe_catalog.get(pid) or {}).get("label", pid)) for pid in probe_ids],
                    rotation=35,
                    ha="right",
                    fontsize=8,
                )
                ax.set_yticks(list(range(len(combo_keys))))
                ax.set_yticklabels(combo_keys, fontsize=8)
                ax.set_title("Probe sensitivity by operator and variant")
                fig.tight_layout()
                out = out_dir / "fem_option_validation_matrix.png"
                fig.savefig(out, dpi=180)
                plt.close(fig)
                generated["matrix_plot"] = str(out)

    return generated


def _make_out_dir(backend: str) -> Path:
    ts = datetime.now().strftime("%Y%m%d_%H%M%S_%f")
    out = ROOT / "data" / "fem_option_validation" / f"{ts}__fem_option_validation__backend-{backend}"
    out.mkdir(parents=True, exist_ok=True)
    return out


def _row_to_flags(row: list[int]) -> dict[str, int]:
    return {
        "coal_read": int(row[0]),
        "coal_write": int(row[1]),
        "compute_all_shape_fun_der": int(row[2]),
        "use_workspace_for_pde_coeff": int(row[3]),
        "use_workspace_for_geo_data": int(row[4]),
        "use_workspace_for_shape_fun": int(row[5]),
        "use_workspace_for_stiff_mat": int(row[6]),
        "padding": 1 if int(row[8]) else 0,
    }


def _ns_per_unit(metrics: dict[str, Any]) -> float:
    try:
        elapsed = float(metrics.get("elapsed_s_mean", float("nan")))
        n_elements = max(1.0, float(metrics.get("n_elements", 1.0)))
        n_qp = max(1.0, float(metrics.get("n_qp_effective", metrics.get("n_qp_requested", 1.0))))
        return elapsed * 1e9 / max(1.0, n_elements * n_qp)
    except Exception:
        return float("nan")


def _build_problem(backend: str, device_index: int, repeats: int) -> FemParametricProblem:
    return FemParametricProblem(
        FemParametricProblemConfig(
            backend=backend,
            device_index=device_index,
            repeats=max(1, int(repeats)),
            execution_policy="native_only",
            n_elements_min=1,
            n_elements_max=1,
            n_qp_min=1,
            n_qp_max=1,
            element_types=["prism6"],
            operators=["laplace", "test"],
            dtypes=["float32"],
            algorithm_variants=list(VARIANTS),
            workgroup_sizes=[1, 32, 64, 128, 256, 512],
            use_workspace_for_pde_coeff_choices=[0, 1],
            use_workspace_for_geo_data_choices=[0, 1],
            use_workspace_for_shape_fun_choices=[0, 1],
            use_workspace_for_stiff_mat_choices=[0, 1],
            padding_choices=[0, 1],
            compute_all_shape_fun_der_choices=[0, 1],
            coal_read_choices=[0, 1],
            coal_write_choices=[0, 1],
            memory_budget_mb=768,
            memory_budget_fraction=0.4,
            record_raw_artifacts=False,
        )
    )


def _evaluate(problem: FemParametricProblem, *, operator: str, variant: str, row: list[int], n_elements: int, n_qp: int, workgroup_size: int) -> dict[str, Any]:
    cfg = {
        "n_elements": int(n_elements),
        "n_qp": int(n_qp),
        "element_type": "prism6",
        "operator": str(operator),
        "dtype": "float32",
        "algorithm_variant": str(variant),
        "workgroup_size": int(workgroup_size),
        **_row_to_flags(row),
    }
    res = problem.evaluate(cfg)
    metrics = dict(res.metrics)
    return {
        "status": str(res.status),
        "constraints_ok": bool(res.constraints_ok),
        "violations": list(res.violations),
        "metrics": metrics,
        "ns_per_unit": _ns_per_unit(metrics),
        "artifacts": dict(res.artifacts),
        "config_effective": dict(res.artifacts.get("config_effective") or cfg),
    }


def main() -> None:
    ap = argparse.ArgumentParser(
        description=(
            "Run controlled FEM option-validation probes across backends. "
            "The probe suite mirrors the verification-relevant option space used by the exact campaign, "
            "but is intentionally presented as a backend-neutral validation tool."
        )
    )
    ap.add_argument("--backend", choices=["cpu", "metal", "cuda", "hip", "opencl", "intel", "amd", "auto"], default="auto")
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--operators", default="laplace,test")
    ap.add_argument("--variants", default=",".join(VARIANTS))
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--n-elements", type=int, default=4096)
    ap.add_argument("--n-qp", type=int, default=6)
    ap.add_argument("--workgroup-size", type=int, default=64)
    args = ap.parse_args()

    requested_backend = str(args.backend).strip().lower()
    backend = "opencl" if requested_backend in ("intel", "amd") else requested_backend
    if backend == "auto":
        backend = "metal" if sys.platform == "darwin" else "opencl"
    resolved_device_index, device_reason = resolve_device_index(backend, int(args.device_index))
    problem = _build_problem(backend, resolved_device_index, int(args.repeats))
    variants = [item.strip().lower() for item in str(args.variants).split(",") if item.strip()]
    operators = [item.strip().lower() for item in str(args.operators).split(",") if item.strip()]
    out_dir = _make_out_dir(problem.mode.resolved_backend)
    csv_path = out_dir / "fem_option_validation.csv"
    jsonl_path = out_dir / "records.jsonl"
    probe_summary_csv_path = out_dir / "probe_summary.csv"
    category_summary_csv_path = out_dir / "category_summary.csv"
    markdown_path = out_dir / "fem_option_validation.md"

    rows: list[dict[str, Any]] = []
    with jsonl_path.open("w", encoding="utf-8") as handle:
        for operator in operators:
            for variant in variants:
                for probe in PROBES:
                    baseline = _evaluate(
                        problem,
                        operator=operator,
                        variant=variant,
                        row=list(probe["baseline"]),
                        n_elements=int(args.n_elements),
                        n_qp=int(args.n_qp),
                        workgroup_size=int(args.workgroup_size),
                    )
                    toggled = _evaluate(
                        problem,
                        operator=operator,
                        variant=variant,
                        row=list(probe["toggled"]),
                        n_elements=int(args.n_elements),
                        n_qp=int(args.n_qp),
                        workgroup_size=int(args.workgroup_size),
                    )
                    baseline_ns = float(baseline["ns_per_unit"])
                    toggled_ns = float(toggled["ns_per_unit"])
                    delta_abs = toggled_ns - baseline_ns
                    delta_ratio = toggled_ns / baseline_ns if baseline_ns > 0 else float("nan")
                    row = {
                        "backend": problem.mode.resolved_backend,
                        "execution_mode": problem.mode.execution_mode,
                        "device": problem.mode.device_name,
                        "device_index_used": int(resolved_device_index),
                        "device_resolution_reason": device_reason,
                        "operator": operator,
                        "variant": variant,
                        "probe_id": str(probe["id"]),
                        "probe_label": str(probe["label"]),
                        "probe_category": str(probe.get("category", "")),
                        "probe_rationale": str(probe.get("rationale", "")),
                        "related_controls": list(probe.get("related_controls") or []),
                        "baseline_label": str(probe.get("baseline_label", "baseline")),
                        "toggled_label": str(probe.get("toggled_label", "toggled")),
                        "baseline_row": list(probe["baseline"]),
                        "toggled_row": list(probe["toggled"]),
                        "baseline_ns_per_unit": baseline_ns,
                        "toggled_ns_per_unit": toggled_ns,
                        "delta_ns_per_unit": delta_abs,
                        "delta_ratio": delta_ratio,
                        "baseline_status": baseline["status"],
                        "toggled_status": toggled["status"],
                        "baseline_constraints_ok": baseline["constraints_ok"],
                        "toggled_constraints_ok": toggled["constraints_ok"],
                        "baseline_metrics": baseline["metrics"],
                        "toggled_metrics": toggled["metrics"],
                    }
                    rows.append(row)
                    handle.write(json.dumps(row, ensure_ascii=True) + "\n")

    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow([
            "backend",
            "execution_mode",
            "device",
            "operator",
            "variant",
            "probe_id",
            "probe_label",
            "probe_category",
            "baseline_ns_per_unit",
            "toggled_ns_per_unit",
            "delta_ns_per_unit",
            "delta_ratio",
        ])
        for row in rows:
            writer.writerow([
                row["backend"],
                row["execution_mode"],
                row["device"],
                row["operator"],
                row["variant"],
                row["probe_id"],
                row["probe_label"],
                row["probe_category"],
                f"{float(row['baseline_ns_per_unit']):.9g}",
                f"{float(row['toggled_ns_per_unit']):.9g}",
                f"{float(row['delta_ns_per_unit']):.9g}",
                f"{float(row['delta_ratio']):.9g}",
            ])

    grouped: dict[str, dict[str, Any]] = {}
    category_grouped: dict[str, list[float]] = {}
    for row in rows:
        probe_id = str(row["probe_id"])
        probe_entry = grouped.setdefault(
            probe_id,
            {
                "probe_label": row["probe_label"],
                "probe_category": row["probe_category"],
                "probe_rationale": row["probe_rationale"],
                "baseline_label": row["baseline_label"],
                "toggled_label": row["toggled_label"],
                "related_controls": row["related_controls"],
                "baseline_row": row["baseline_row"],
                "toggled_row": row["toggled_row"],
                "delta_ratios": [],
            },
        )
        probe_entry["delta_ratios"].append(float(row["delta_ratio"]))
        category_grouped.setdefault(str(row["probe_category"]), []).append(float(row["delta_ratio"]))
    summary = {
        "created_at": datetime.now().astimezone().isoformat(),
        "workflow": "fem_option_validation",
        "experiment_class": "fem_option_validation",
        "backend": problem.mode.resolved_backend,
        "execution_mode": problem.mode.execution_mode,
        "device": problem.mode.device_name,
        "device_index_used": int(resolved_device_index),
        "device_resolution_reason": device_reason,
        "n_elements": int(args.n_elements),
        "n_qp": int(args.n_qp),
        "workgroup_size": int(args.workgroup_size),
        "repeats": int(args.repeats),
        "records": len(rows),
        "csv_path": str(csv_path),
        "jsonl_path": str(jsonl_path),
        "probe_summary_csv": str(probe_summary_csv_path),
        "category_summary_csv": str(category_summary_csv_path),
        "markdown_path": str(markdown_path),
        "method_note": (
            "Validation probes mirror the verification-relevant option controls used by the exact FEM campaign, "
            "but they run through the shared cross-platform problem layer."
        ),
        "exact_like_reference_row": EXACT_LIKE_ROW,
        "probe_catalog": {
            str(probe["id"]): {
                "label": str(probe["label"]),
                "category": str(probe.get("category", "")),
                "rationale": str(probe.get("rationale", "")),
                "alignment_control": str(probe.get("alignment_control", "")),
                "related_controls": list(probe.get("related_controls") or []),
                "baseline_label": str(probe.get("baseline_label", "baseline")),
                "toggled_label": str(probe.get("toggled_label", "toggled")),
                "baseline_row": list(probe["baseline"]),
                "toggled_row": list(probe["toggled"]),
            }
            for probe in PROBES
        },
        "probe_summary": {
            key: {
                "probe_label": str(values["probe_label"]),
                "probe_category": str(values["probe_category"]),
                "probe_rationale": str(values["probe_rationale"]),
                "baseline_label": str(values["baseline_label"]),
                "toggled_label": str(values["toggled_label"]),
                "related_controls": list(values["related_controls"]),
                "baseline_row": list(values["baseline_row"]),
                "toggled_row": list(values["toggled_row"]),
                "mean_delta_ratio": sum(values["delta_ratios"]) / max(1, len(values["delta_ratios"])),
                "max_delta_ratio": max(values["delta_ratios"]) if values["delta_ratios"] else None,
                "min_delta_ratio": min(values["delta_ratios"]) if values["delta_ratios"] else None,
                "samples": len(values["delta_ratios"]),
                "recommended_state": "enable" if values["delta_ratios"] and (sum(values["delta_ratios"]) / len(values["delta_ratios"])) < 1.0 else "disable",
            }
            for key, values in grouped.items()
        },
        "category_summary": {
            key: {
                "mean_delta_ratio": sum(vals) / max(1, len(vals)),
                "max_delta_ratio": max(vals) if vals else None,
                "min_delta_ratio": min(vals) if vals else None,
                "samples": len(vals),
            }
            for key, vals in category_grouped.items()
        },
    }

    with probe_summary_csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "probe_id",
                "probe_label",
                "probe_category",
                "recommended_state",
                "mean_delta_ratio",
                "min_delta_ratio",
                "max_delta_ratio",
                "samples",
                "baseline_label",
                "toggled_label",
                "related_controls",
            ]
        )
        for probe_id, values in sorted((summary.get("probe_summary") or {}).items()):
            if not isinstance(values, dict):
                continue
            writer.writerow(
                [
                    probe_id,
                    values.get("probe_label", ""),
                    values.get("probe_category", ""),
                    values.get("recommended_state", ""),
                    values.get("mean_delta_ratio", ""),
                    values.get("min_delta_ratio", ""),
                    values.get("max_delta_ratio", ""),
                    values.get("samples", ""),
                    values.get("baseline_label", ""),
                    values.get("toggled_label", ""),
                    ",".join(str(x) for x in values.get("related_controls", []) or []),
                ]
            )

    with category_summary_csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["category", "mean_delta_ratio", "min_delta_ratio", "max_delta_ratio", "samples"])
        for category, values in sorted((summary.get("category_summary") or {}).items()):
            if not isinstance(values, dict):
                continue
            writer.writerow(
                [
                    category,
                    values.get("mean_delta_ratio", ""),
                    values.get("min_delta_ratio", ""),
                    values.get("max_delta_ratio", ""),
                    values.get("samples", ""),
                ]
            )

    md_lines = [
        "# FEM option validation",
        "",
        f"- backend: `{summary.get('backend')}`",
        f"- execution_mode: `{summary.get('execution_mode')}`",
        f"- device: `{summary.get('device')}`",
        f"- records: `{summary.get('records')}`",
        f"- n_elements: `{summary.get('n_elements')}`",
        f"- n_qp: `{summary.get('n_qp')}`",
        f"- workgroup_size: `{summary.get('workgroup_size')}`",
        "",
        "## Probe summary",
        "",
        "| probe | category | recommended | mean delta ratio | min | max | samples |",
        "|---|---|---|---:|---:|---:|---:|",
    ]
    for _probe_id, values in sorted((summary.get("probe_summary") or {}).items()):
        if not isinstance(values, dict):
            continue
        md_lines.append(
            f"| {values.get('probe_label', _probe_id)} | {values.get('probe_category', '')} | "
            f"{values.get('recommended_state', '')} | {values.get('mean_delta_ratio', '')} | "
            f"{values.get('min_delta_ratio', '')} | {values.get('max_delta_ratio', '')} | {values.get('samples', '')} |"
        )
    category_summary = summary.get("category_summary") or {}
    if category_summary:
        md_lines.extend(
            [
                "",
                "## Category summary",
                "",
                "| category | mean delta ratio | min | max | samples |",
                "|---|---:|---:|---:|---:|",
            ]
        )
        for category, values in sorted(category_summary.items()):
            if not isinstance(values, dict):
                continue
            md_lines.append(
                f"| {category} | {values.get('mean_delta_ratio', '')} | {values.get('min_delta_ratio', '')} | "
                f"{values.get('max_delta_ratio', '')} | {values.get('samples', '')} |"
            )
    markdown_path.write_text("\n".join(md_lines) + "\n", encoding="utf-8")
    generated_plots = _write_overview_plots(summary, rows, out_dir)
    if generated_plots:
        summary["plots"] = generated_plots
    summary["provenance"] = collect_runtime_provenance(
        ROOT,
        extra_files={
            "csv": csv_path,
            "jsonl": jsonl_path,
            "probe_summary_csv": probe_summary_csv_path,
            "category_summary_csv": category_summary_csv_path,
            "markdown": markdown_path,
            **{key: Path(path) for key, path in generated_plots.items()},
        },
    )
    summary["summary_hash"] = sha256_json({k: v for k, v in summary.items() if k != "summary_hash"})
    (out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")

    print("=== FEM OPTION VALIDATION ===")
    print(f"backend            : {summary['backend']}")
    print(f"execution mode     : {summary['execution_mode']}")
    print(f"device             : {summary['device']}")
    print(f"operators          : {','.join(operators)}")
    print(f"variants           : {','.join(variants)}")
    print(f"records            : {summary['records']}")
    print(f"out dir            : {out_dir}")


if __name__ == "__main__":
    main()
