from __future__ import annotations

import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in __import__("sys").path:
    __import__("sys").path.insert(0, str(ROOT))

from analysis.bottleneck_classifier import (
    classify_bottleneck,
    classify_workload_class,
    normalize_bottleneck_family,
)

GPU_BACKENDS = {"metal", "cuda", "hip", "opencl"}


def _to_float(value: object) -> float | None:
    try:
        if value is None:
            return None
        text = str(value).strip()
        if not text or text.lower() == "nan":
            return None
        return float(text)
    except Exception:
        return None


def _read_csv_rows(path: Path) -> list[dict[str, str]]:
    if not path.exists():
        return []
    with path.open("r", encoding="utf-8", newline="") as handle:
        return list(csv.DictReader(handle))


def _load_roofline_peaks(roofline_json: Path) -> dict[tuple[str, str], dict[str, Any]]:
    if not roofline_json.exists():
        return {}
    try:
        payload = json.loads(roofline_json.read_text(encoding="utf-8"))
    except Exception:
        return {}
    peaks: dict[tuple[str, str], dict[str, Any]] = {}
    for item in payload.get("results", []) or []:
        record = item.get("record", {}) or {}
        target = str(item.get("target", "") or record.get("target", "")).strip().lower()
        backend = str(record.get("backend", "")).strip().lower()
        peaks[(target, backend)] = {
            "target": target,
            "backend": backend,
            "model": record.get("model", ""),
            "peak_bw_gbps": _to_float(record.get("peak_bw_gbps")),
            "peak_gflops": _to_float(record.get("peak_gflops")),
        }
    return peaks


def _source_area(summary_path: Path) -> str:
    parts = list(summary_path.parts)
    if "contracts" in parts:
        idx = parts.index("contracts")
        if idx + 1 < len(parts):
            return str(parts[idx + 1])
    return "unknown"


def _problem_size_label(row: dict[str, Any]) -> str:
    for key in ("problem_size", "size_bytes", "working_set_bytes", "n_elements", "n", "pages_touched", "size_mb"):
        if row.get(key) not in (None, ""):
            return str(row.get(key))
    for keys in (("m", "n", "k"), ("h", "w"), ("d", "h", "w")):
        vals = [row.get(k) for k in keys]
        if all(v not in (None, "") for v in vals):
            return "x".join(str(v) for v in vals)
    return ""


def _itemsize_from_dtype(dtype: str) -> float:
    return 8.0 if str(dtype).strip().lower() == "float64" else 4.0


def _kernel_ai_estimate(row: dict[str, Any]) -> float | None:
    kernel = str(row.get("kernel_name") or row.get("kernel") or "").strip().lower()
    benchmark = str(row.get("benchmark_name") or "").strip().lower()
    label = f"{benchmark} {kernel}".strip()
    dtype = str(row.get("dtype", "float32")).strip().lower()
    itemsize = _itemsize_from_dtype(dtype)

    explicit = _to_float(row.get("arithmetic_intensity") or row.get("ai_flop_per_byte"))
    if explicit is not None:
        return explicit

    if "gemm" in label:
        m = _to_float(row.get("m"))
        n = _to_float(row.get("n"))
        k = _to_float(row.get("k")) or _to_float(row.get("m"))
        if not (m and n and k):
            return None
        flops = 2.0 * m * n * k
        bytes_total = (m * k + k * n + m * n) * itemsize
        return flops / max(bytes_total, 1.0)

    if "spmv" in label:
        n = _to_float(row.get("n"))
        nnz_per_row = _to_float(row.get("nnz_per_row"))
        if not (n and nnz_per_row):
            return None
        nnz = n * nnz_per_row
        flops = 2.0 * nnz
        bytes_total = nnz * itemsize + nnz * 4.0 + nnz * itemsize + n * itemsize
        return flops / max(bytes_total, 1.0)

    if "fem_integration" in label:
        return _to_float(row.get("ai_flop_per_byte"))

    if "assembly_like" in label or "author_fem_assembly" in label:
        ai = _to_float(row.get("ai_flop_per_byte"))
        if ai is not None:
            return ai
        n_elements = _to_float(row.get("n_elements"))
        n_qp = _to_float(row.get("n_qp"))
        n_dofs = _to_float(row.get("n_dofs"))
        if not (n_elements and n_qp and n_dofs):
            return None
        flops = n_elements * n_qp * n_dofs * n_dofs * 6.0
        bytes_total = n_elements * n_qp * (3.0 * n_dofs + 2.0 * n_dofs * n_dofs) * itemsize
        return flops / max(bytes_total, 1.0)

    if benchmark == "fem" or kernel == "fem":
        n_elements = _to_float(row.get("n_elements"))
        n_qp = _to_float(row.get("n_qp"))
        if not (n_elements and n_qp):
            return None
        flops = n_elements * n_qp * (9.0 * 2.0)
        bytes_total = (n_elements * 9.0 + n_qp * 9.0 + n_elements) * itemsize
        return flops / max(bytes_total, 1.0)

    if "saxpy" in label or "axpy" in label:
        return 2.0 / max(3.0 * itemsize, 1.0)
    if "reduction" in label:
        return 0.0625
    if "stream" in label or "bandwidth" in label or "mem_copy" in label or "memcpy" in label:
        return 0.125
    if "stencil2d" in label:
        return 0.18
    if "stencil3d" in label:
        return 0.12
    if "fma" in label:
        return 8.0
    return None


def _infer_measured_gflops(*, measured_gflops: float | None, measured_bw: float | None, ai: float | None) -> float | None:
    if measured_gflops is not None:
        return measured_gflops
    if measured_bw is not None and ai is not None:
        return measured_bw * ai
    return None


def _infer_measured_bandwidth(*, measured_bw: float | None, measured_gflops: float | None, ai: float | None) -> float | None:
    if measured_bw is not None:
        return measured_bw
    if measured_gflops is not None and ai not in (None, 0.0):
        return measured_gflops / ai
    return None


def _roofline_prediction(ai: float | None, peak_bw: float | None, peak_gflops: float | None) -> tuple[float | None, str]:
    if ai is None or peak_bw is None or peak_gflops is None:
        return None, ""
    bw_ceiling = ai * peak_bw
    if bw_ceiling < peak_gflops:
        return bw_ceiling, "memory-bound"
    return peak_gflops, "compute-bound"


def _infer_target(backend: str, source_area: str) -> str:
    if backend in GPU_BACKENDS and source_area != "cpu":
        return "gpu"
    return "cpu"


def _peak_for_row(peaks: dict[tuple[str, str], dict[str, Any]], *, target: str, backend: str) -> dict[str, Any] | None:
    return peaks.get((target, backend)) or peaks.get((target, "cpu" if target == "cpu" else backend))


def _bool_str(value: bool | None) -> str:
    if value is None:
        return ""
    return "true" if value else "false"


def _dominant_label(labels: list[str]) -> str:
    filtered = [label for label in labels if label]
    if not filtered:
        return ""
    return Counter(filtered).most_common(1)[0][0]


def _accuracy(rows: list[dict[str, Any]], key: str) -> dict[str, Any]:
    considered = [row for row in rows if row.get(key) in ("true", "false")]
    if not considered:
        return {"considered": 0, "matches": 0, "accuracy": None}
    matches = sum(1 for row in considered if row.get(key) == "true")
    return {
        "considered": len(considered),
        "matches": matches,
        "accuracy": matches / len(considered),
    }


def _write_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    if not rows:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def _write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")


def _fallback_predicted_from_workload(workload_class: str) -> str:
    mapping = {
        "bandwidth-bound": "memory-bound",
        "latency-bound": "latency-bound",
        "compute-bound": "compute-bound",
        "irregular memory-bound": "irregular-memory-bound",
        "mixed stencil": "cache-bound",
        "mixed / application-bound": "mixed",
    }
    return mapping.get(workload_class, "mixed")


def build_model_validation_summary(*, session_dir: Path) -> dict[str, str]:
    session_dir = Path(session_dir).expanduser().resolve()
    roofline_json = session_dir / "roofline" / "roofline_result.json"
    peaks = _load_roofline_peaks(roofline_json)
    if not peaks:
        return {}

    summary_paths = sorted((session_dir / "contracts").rglob("summary.csv"))
    rows_out: list[dict[str, Any]] = []
    for summary_path in summary_paths:
        source_area = _source_area(summary_path)
        for row in _read_csv_rows(summary_path):
            backend = str(row.get("backend", "")).strip().lower() or "cpu"
            target = _infer_target(backend, source_area)
            peak = _peak_for_row(peaks, target=target, backend=backend)
            if peak is None:
                continue

            kernel_name = str(row.get("kernel_name") or row.get("kernel") or summary_path.stem)
            benchmark_name = str(row.get("benchmark_name") or summary_path.stem)
            ai = _kernel_ai_estimate(row)
            measured_gflops = _infer_measured_gflops(
                measured_gflops=_to_float(row.get("gflops")),
                measured_bw=_to_float(row.get("bandwidth_GBps")),
                ai=ai,
            )
            measured_bw = _infer_measured_bandwidth(
                measured_bw=_to_float(row.get("bandwidth_GBps")),
                measured_gflops=measured_gflops,
                ai=ai,
            )
            predicted, predicted_bottleneck = _roofline_prediction(ai, peak.get("peak_bw_gbps"), peak.get("peak_gflops"))
            workload_class = classify_workload_class(
                benchmark_name=benchmark_name,
                kernel_name=kernel_name,
                arithmetic_intensity=ai,
            )
            if not predicted_bottleneck:
                predicted_bottleneck = _fallback_predicted_from_workload(workload_class)

            measured_class = classify_bottleneck(
                benchmark_name=benchmark_name,
                kernel_name=kernel_name,
                arithmetic_intensity=ai,
                measured_gflops=measured_gflops,
                measured_bandwidth=measured_bw,
                predicted_gflops=predicted,
                latency_ns=_to_float(row.get("latency_mean_ns")),
            )
            measured_bottleneck = str(measured_class.get("bottleneck", ""))
            measured_family = normalize_bottleneck_family(measured_bottleneck)
            predicted_family = normalize_bottleneck_family(predicted_bottleneck)
            exact_match = predicted_bottleneck == measured_bottleneck if predicted_bottleneck and measured_bottleneck else None
            family_match = predicted_family == measured_family if predicted_family and measured_family else None
            achieved_pct = (measured_gflops / predicted * 100.0) if (measured_gflops is not None and predicted not in (None, 0.0)) else None
            error_pct = (abs(predicted - measured_gflops) / predicted * 100.0) if (measured_gflops is not None and predicted not in (None, 0.0)) else None
            pct_peak_compute = (measured_gflops / peak.get("peak_gflops") * 100.0) if (measured_gflops is not None and peak.get("peak_gflops") not in (None, 0.0)) else None
            pct_peak_bw = (measured_bw / peak.get("peak_bw_gbps") * 100.0) if (measured_bw is not None and peak.get("peak_bw_gbps") not in (None, 0.0)) else None

            rows_out.append(
                {
                    "platform": str(row.get("platform") or peak.get("model", "unknown")),
                    "backend": backend,
                    "target": target,
                    "source_area": source_area,
                    "benchmark_name": benchmark_name,
                    "kernel": kernel_name,
                    "problem_size": _problem_size_label(row),
                    "threads": str(row.get("threads", "")),
                    "block_size": str(row.get("block_size", "")),
                    "status": str(row.get("status", "")),
                    "workload_class": workload_class,
                    "arithmetic_intensity": "" if ai is None else ai,
                    "measured_gflops": "" if measured_gflops is None else measured_gflops,
                    "measured_bandwidth": "" if measured_bw is None else measured_bw,
                    "peak_gflops": "" if peak.get("peak_gflops") is None else peak.get("peak_gflops"),
                    "peak_bw_gbps": "" if peak.get("peak_bw_gbps") is None else peak.get("peak_bw_gbps"),
                    "measured_percent_of_peak_compute": "" if pct_peak_compute is None else pct_peak_compute,
                    "measured_percent_of_peak_bw": "" if pct_peak_bw is None else pct_peak_bw,
                    "roofline_predicted_gflops": "" if predicted is None else predicted,
                    "achieved_roofline_percent": "" if achieved_pct is None else achieved_pct,
                    "model_error_percent": "" if error_pct is None else error_pct,
                    "predicted_bottleneck": predicted_bottleneck,
                    "predicted_bottleneck_family": predicted_family,
                    "measured_bottleneck": measured_bottleneck,
                    "measured_bottleneck_family": measured_family,
                    "profiler_bottleneck": "",
                    "classifier_confidence": measured_class.get("confidence", ""),
                    "classifier_reason": measured_class.get("reason", ""),
                    "classification_match": _bool_str(family_match),
                    "classification_match_exact": _bool_str(exact_match),
                    "summary_source": str(summary_path.resolve()),
                }
            )

    if not rows_out:
        return {}

    summary_path = session_dir / "model_validation_summary.csv"
    _write_csv(summary_path, rows_out)

    matrix_rows: list[dict[str, Any]] = []
    grouped: dict[tuple[str, str, str, str], list[dict[str, Any]]] = defaultdict(list)
    for row in rows_out:
        grouped[(row["platform"], row["backend"], row["workload_class"], row["kernel"])].append(row)
    for (platform_name, backend, workload_class, kernel), items in sorted(grouped.items()):
        error_values = [float(item["model_error_percent"]) for item in items if item.get("model_error_percent") not in ("", None)]
        roofline_values = [float(item["achieved_roofline_percent"]) for item in items if item.get("achieved_roofline_percent") not in ("", None)]
        matrix_rows.append(
            {
                "platform": platform_name,
                "backend": backend,
                "workload_class": workload_class,
                "kernel": kernel,
                "rows": len(items),
                "dominant_predicted_bottleneck": _dominant_label([str(item.get("predicted_bottleneck", "")) for item in items]),
                "dominant_measured_bottleneck": _dominant_label([str(item.get("measured_bottleneck", "")) for item in items]),
                "dominant_predicted_family": _dominant_label([str(item.get("predicted_bottleneck_family", "")) for item in items]),
                "dominant_measured_family": _dominant_label([str(item.get("measured_bottleneck_family", "")) for item in items]),
                "mean_model_error_percent": (sum(error_values) / len(error_values)) if error_values else "",
                "mean_achieved_roofline_percent": (sum(roofline_values) / len(roofline_values)) if roofline_values else "",
                "classification_accuracy": _accuracy(items, "classification_match")["accuracy"],
                "classification_accuracy_exact": _accuracy(items, "classification_match_exact")["accuracy"],
            }
        )
    matrix_path = session_dir / "bottleneck_classification_matrix.csv"
    _write_csv(matrix_path, matrix_rows)

    overall = _accuracy(rows_out, "classification_match")
    overall_exact = _accuracy(rows_out, "classification_match_exact")
    real_rows = [row for row in rows_out if row.get("source_area") == "real_kernels"]
    real_accuracy = _accuracy(real_rows, "classification_match")
    real_accuracy_exact = _accuracy(real_rows, "classification_match_exact")

    grouped_backend: dict[str, list[dict[str, Any]]] = defaultdict(list)
    grouped_target: dict[str, list[dict[str, Any]]] = defaultdict(list)
    grouped_workload: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for row in rows_out:
        grouped_backend[str(row.get("backend", ""))].append(row)
        grouped_target[str(row.get("target", ""))].append(row)
        grouped_workload[str(row.get("workload_class", ""))].append(row)

    report = {
        "session_dir": str(session_dir),
        "row_count": len(rows_out),
        "overall_classification_accuracy": overall,
        "overall_exact_classification_accuracy": overall_exact,
        "real_kernel_classification_accuracy": real_accuracy,
        "real_kernel_exact_classification_accuracy": real_accuracy_exact,
        "by_backend": {
            backend: {
                "family_accuracy": _accuracy(items, "classification_match"),
                "exact_accuracy": _accuracy(items, "classification_match_exact"),
                "rows": len(items),
            }
            for backend, items in sorted(grouped_backend.items())
        },
        "by_target": {
            target: {
                "family_accuracy": _accuracy(items, "classification_match"),
                "exact_accuracy": _accuracy(items, "classification_match_exact"),
                "rows": len(items),
            }
            for target, items in sorted(grouped_target.items())
        },
        "by_workload_class": {
            workload: {
                "family_accuracy": _accuracy(items, "classification_match"),
                "exact_accuracy": _accuracy(items, "classification_match_exact"),
                "rows": len(items),
            }
            for workload, items in sorted(grouped_workload.items())
        },
        "notes": [
            "classification_match oznacza zgodność na poziomie rodziny bottlenecków.",
            "classification_match_exact oznacza dokładne dopasowanie etykiety bottlenecku.",
            "Raport bazuje na zestandaryzowanych summary.csv z katalogu contracts/, a nie na surowych powtórzeniach.",
        ],
    }
    report_path = session_dir / "model_validation_report.json"
    _write_json(report_path, report)

    return {
        "summary_csv": str(summary_path),
        "report_json": str(report_path),
        "bottleneck_matrix_csv": str(matrix_path),
    }


def main() -> None:
    ap = argparse.ArgumentParser(description="Build model validation artifacts from session contracts.")
    ap.add_argument("--session-dir", required=True)
    args = ap.parse_args()
    payload = build_model_validation_summary(session_dir=Path(args.session_dir))
    print(json.dumps(payload, indent=2, ensure_ascii=False))


if __name__ == "__main__":
    main()
