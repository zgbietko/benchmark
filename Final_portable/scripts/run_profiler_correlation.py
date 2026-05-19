#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from datetime import datetime
import json
import math
import os
from pathlib import Path
import statistics as stats
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.fem_validation_support import compute_option_alignment, compute_profile_proximity
from analysis.provenance import collect_runtime_provenance, sha256_json


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


def _write_plots(result: dict[str, Any], out_dir: Path) -> dict[str, str]:
    plt = _try_import_matplotlib()
    if plt is None:
        return {}

    generated: dict[str, str] = {}
    alignment = [row for row in (result.get("option_alignment") or []) if isinstance(row, dict)]
    if alignment:
        labels = [str(row.get("probe_label", row.get("probe_id", ""))) for row in alignment]
        vals: list[float] = []
        colors: list[str] = []
        for row in alignment:
            try:
                ratio = float(row.get("mean_delta_ratio", float("nan")))
            except Exception:
                ratio = float("nan")
            vals.append(ratio)
            colors.append("#15803d" if bool(row.get("supports_best_config")) else "#b91c1c")
        fig, ax = plt.subplots(figsize=(max(9, 0.55 * len(labels) + 2), 5.5))
        xs = list(range(len(labels)))
        ax.bar(xs, vals, color=colors, alpha=0.9)
        ax.axhline(1.0, color="#334155", linestyle="--", linewidth=1.0)
        ax.set_xticks(xs)
        ax.set_xticklabels(labels, rotation=35, ha="right", fontsize=8)
        ax.set_ylabel("Mean delta ratio")
        ax.set_title("Option alignment with best exact/native configuration")
        ax.grid(True, axis="y", alpha=0.25)
        fig.tight_layout()
        out = out_dir / "option_alignment.png"
        fig.savefig(out, dpi=180)
        plt.close(fig)
        generated["option_alignment_plot"] = str(out)

    profile_proximity = [row for row in (result.get("profile_proximity") or []) if isinstance(row, dict)]
    if profile_proximity:
        labels = [str(row.get("probe_label", row.get("probe_id", ""))) for row in profile_proximity]
        baseline = []
        toggled = []
        for row in profile_proximity:
            try:
                baseline.append(float(row.get("baseline_distance", float("nan"))))
            except Exception:
                baseline.append(float("nan"))
            try:
                toggled.append(float(row.get("toggled_distance", float("nan"))))
            except Exception:
                toggled.append(float("nan"))
        fig, ax = plt.subplots(figsize=(max(9, 0.55 * len(labels) + 2), 5.5))
        xs = list(range(len(labels)))
        width = 0.38
        ax.bar([x - width / 2 for x in xs], baseline, width=width, color="#475569", label="distance to baseline")
        ax.bar([x + width / 2 for x in xs], toggled, width=width, color="#2563eb", label="distance to toggled")
        ax.set_xticks(xs)
        ax.set_xticklabels(labels, rotation=35, ha="right", fontsize=8)
        ax.set_ylabel("Hamming distance")
        ax.set_title("Composite-profile proximity")
        ax.grid(True, axis="y", alpha=0.25)
        ax.legend()
        fig.tight_layout()
        out = out_dir / "profile_proximity.png"
        fig.savefig(out, dpi=180)
        plt.close(fig)
        generated["profile_proximity_plot"] = str(out)

    category_summary = result.get("category_summary") or {}
    if category_summary:
        categories = sorted(category_summary.keys())
        vals = []
        for cat in categories:
            try:
                vals.append(float((category_summary.get(cat) or {}).get("mean_delta_ratio", float("nan"))))
            except Exception:
                vals.append(float("nan"))
        if any(math.isfinite(v) for v in vals):
            fig, ax = plt.subplots(figsize=(max(7, 1.2 * len(categories) + 2), 4.8))
            colors = ["#0f766e" if math.isfinite(v) and v < 1.0 else "#7c2d12" for v in vals]
            ax.bar(categories, vals, color=colors, alpha=0.9)
            ax.axhline(1.0, color="#334155", linestyle="--", linewidth=1.0)
            ax.set_ylabel("Mean delta ratio")
            ax.set_title("Correlation category summary")
            ax.tick_params(axis="x", rotation=20)
            ax.grid(True, axis="y", alpha=0.25)
            fig.tight_layout()
            out = out_dir / "category_summary.png"
            fig.savefig(out, dpi=180)
            plt.close(fig)
            generated["category_summary_plot"] = str(out)

    return generated


def _load_json(path: Path) -> dict[str, Any]:
    return json.loads(Path(path).read_text(encoding="utf-8"))


def _numeric_summary_from_csv(path: Path) -> dict[str, Any]:
    with Path(path).open("r", encoding="utf-8", newline="") as handle:
        rows = list(csv.DictReader(handle))
    numeric: dict[str, list[float]] = {}
    for row in rows:
        for key, value in row.items():
            try:
                numeric.setdefault(key, []).append(float(value))
            except Exception:
                continue
    return {
        key: {
            "mean": float(stats.mean(vals)),
            "min": float(min(vals)),
            "max": float(max(vals)),
            "count": len(vals),
        }
        for key, vals in numeric.items()
        if vals
    }


def _load_profiler_report(path: Path) -> dict[str, Any]:
    p = Path(path)
    suffix = p.suffix.lower()
    if suffix == ".json":
        payload = _load_json(p)
        return {"path": str(p), "kind": "json", "payload": payload}
    if suffix == ".csv":
        payload = _numeric_summary_from_csv(p)
        return {"path": str(p), "kind": "csv", "payload": payload}
    raise SystemExit(f"Unsupported profiler report format: {p}")


def main() -> None:
    ap = argparse.ArgumentParser(
        description="Correlate FEM option-validation results with exact/native benchmark runs and optional profiler exports."
    )
    ap.add_argument("--optimization-dir", required=True, help="Path to an optimization/exact result directory with summary.json.")
    ap.add_argument("--fem-option-validation-dir", required=True, help="Path to run_fem_option_validation output directory.")
    ap.add_argument("--profiler-report", action="append", default=[], help="Optional JSON/CSV profiler export. Can be passed multiple times.")
    ap.add_argument("--out", default="", help="Optional output directory. Default: <optimization-dir>/profiler_correlation")
    args = ap.parse_args()

    optimization_dir = Path(args.optimization_dir).expanduser().resolve()
    option_dir = Path(args.fem_option_validation_dir).expanduser().resolve()
    out_dir = Path(args.out).expanduser().resolve() if str(args.out).strip() else (optimization_dir / "profiler_correlation")
    out_dir.mkdir(parents=True, exist_ok=True)

    opt_summary = _load_json(optimization_dir / "summary.json")
    probe_summary = _load_json(option_dir / "summary.json")
    profiler_reports = [_load_profiler_report(Path(path).expanduser().resolve()) for path in args.profiler_report]

    best_overall = dict(opt_summary.get("best_overall") or {})
    probe_delta = dict(probe_summary.get("probe_summary") or {})
    probe_catalog = dict(probe_summary.get("probe_catalog") or {})
    alignment = compute_option_alignment(best_overall, probe_delta, probe_catalog)
    profile_proximity = compute_profile_proximity(best_overall, probe_catalog)

    result = {
        "created_at": datetime.now().astimezone().isoformat(),
        "workflow": "profiler_correlation",
        "optimization_dir": str(optimization_dir),
        "fem_option_validation_dir": str(option_dir),
        "backend": opt_summary.get("backend"),
        "device": opt_summary.get("device"),
        "execution_mode": opt_summary.get("execution_mode"),
        "best_overall": best_overall,
        "option_probe_summary": probe_delta,
        "option_alignment": alignment,
        "profile_proximity": profile_proximity,
        "category_summary": probe_summary.get("category_summary") or {},
        "profiler_reports": profiler_reports,
    }

    json_path = out_dir / "profiler_correlation.json"
    summary_json_path = out_dir / "summary.json"
    md_path = out_dir / "profiler_correlation.md"
    alignment_csv_path = out_dir / "option_alignment.csv"
    profile_csv_path = out_dir / "profile_proximity.csv"
    category_csv_path = out_dir / "category_summary.csv"
    result["json_path"] = str(json_path)
    result["summary_json_path"] = str(summary_json_path)
    result["markdown_path"] = str(md_path)
    result["option_alignment_csv"] = str(alignment_csv_path)
    result["profile_proximity_csv"] = str(profile_csv_path)
    result["category_summary_csv"] = str(category_csv_path)
    with alignment_csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(
            [
                "probe_id",
                "probe_label",
                "best_config_enabled",
                "recommended_state",
                "mean_delta_ratio",
                "supports_best_config",
                "related_controls",
            ]
        )
        for row in alignment:
            writer.writerow(
                [
                    row.get("probe_id", ""),
                    row.get("probe_label", ""),
                    row.get("best_config_enabled", ""),
                    row.get("recommended_state", ""),
                    row.get("mean_delta_ratio", ""),
                    row.get("supports_best_config", ""),
                    ",".join(str(x) for x in row.get("related_controls", []) or []),
                ]
            )

    with profile_csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["probe_id", "probe_label", "closer_to", "baseline_distance", "toggled_distance"])
        for row in profile_proximity:
            writer.writerow(
                [
                    row.get("probe_id", ""),
                    row.get("probe_label", ""),
                    row.get("closer_to", ""),
                    row.get("baseline_distance", ""),
                    row.get("toggled_distance", ""),
                ]
            )

    with category_csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["category", "mean_delta_ratio", "min_delta_ratio", "max_delta_ratio", "samples"])
        for category, values in sorted((result.get("category_summary") or {}).items()):
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

    lines = [
        "# Profiler correlation report",
        "",
        f"Optimization dir: `{optimization_dir}`",
        f"FEM option validation dir: `{option_dir}`",
        f"Backend: `{result.get('backend')}`",
        f"Execution mode: `{result.get('execution_mode')}`",
        f"Device: `{result.get('device')}`",
        "",
        "## Best overall config",
        "",
        f"- variant: `{best_overall.get('variant')}`",
        f"- option_index: `{best_overall.get('option_index')}`",
        f"- combo_bits: `{best_overall.get('combo_bits')}`",
        f"- internal_ns_per_elem: `{best_overall.get('score_internal_ns_per_elem')}`",
        "",
        "## Option alignment",
        "",
        "| probe | enabled in best config | mean delta ratio | recommended state | supports best config |",
        "|---|---:|---:|---|---|",
    ]
    for row in alignment:
        lines.append(
            f"| {row['probe_label']} | {row['best_config_enabled']} | {row['mean_delta_ratio']} | {row['recommended_state']} | {row['supports_best_config']} |"
        )
    if profile_proximity:
        lines.extend(["", "## Profile proximity", "", "| profile | closer to | baseline distance | toggled distance |", "|---|---|---:|---:|"])
        for row in profile_proximity:
            lines.append(
                f"| {row['probe_label']} | {row['closer_to']} | {row['baseline_distance']} | {row['toggled_distance']} |"
            )
    category_summary = result.get("category_summary") or {}
    if category_summary:
        lines.extend(["", "## Category summary", "", "| category | mean delta ratio | min | max | samples |", "|---|---:|---:|---:|---:|"])
        for category, values in sorted(category_summary.items()):
            if not isinstance(values, dict):
                continue
            lines.append(
                f"| {category} | {values.get('mean_delta_ratio')} | {values.get('min_delta_ratio')} | {values.get('max_delta_ratio')} | {values.get('samples')} |"
            )
    if profiler_reports:
        lines.extend(["", "## Profiler inputs", ""])
        for report in profiler_reports:
            lines.append(f"- `{report['path']}` ({report['kind']})")
    md_path.write_text("\n".join(lines) + "\n", encoding="utf-8")
    generated_plots = _write_plots(result, out_dir)
    if generated_plots:
        result["plots"] = generated_plots
    result["provenance"] = collect_runtime_provenance(
        ROOT,
        extra_files={
            "option_alignment_csv": alignment_csv_path,
            "profile_proximity_csv": profile_csv_path,
            "category_summary_csv": category_csv_path,
            "markdown": md_path,
            **{key: Path(path) for key, path in generated_plots.items()},
        },
    )
    result["summary_hash"] = sha256_json({k: v for k, v in result.items() if k != "summary_hash"})
    json_path.write_text(json.dumps(result, indent=2, ensure_ascii=True), encoding="utf-8")
    summary_json_path.write_text(json.dumps(result, indent=2, ensure_ascii=True), encoding="utf-8")

    print("=== PROFILER CORRELATION ===")
    print(f"optimization dir    : {optimization_dir}")
    print(f"fem validation dir  : {option_dir}")
    print(f"profiler reports    : {len(profiler_reports)}")
    print(f"output json         : {json_path}")
    print(f"output markdown     : {md_path}")


if __name__ == "__main__":
    main()
