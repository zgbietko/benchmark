from __future__ import annotations

from collections import defaultdict
from statistics import mean, median, pstdev
from typing import Any


def _to_float(v: Any) -> float | None:
    try:
        if v is None:
            return None
        s = str(v).strip()
        if s == "" or s.lower() == "nan":
            return None
        return float(s)
    except Exception:
        return None


def _to_int(v: Any) -> int | None:
    x = _to_float(v)
    if x is None:
        return None
    try:
        return int(x)
    except Exception:
        return None


def cache_boundaries_from_rows(rows: list[dict[str, Any]]) -> dict[str, int]:
    out: dict[str, int] = {}
    field_order = (
        ("eff_l1d_bytes", "E-L1D"),
        ("perf_l1d_bytes", "P-L1D"),
        ("eff_l2_bytes", "E-L2"),
        ("perf_l2_bytes", "P-L2"),
        ("l1d_bytes", "L1"),
        ("l2_bytes", "L2"),
        ("l3_bytes", "L3"),
    )
    for field, label in field_order:
        vals = [_to_int(r.get(field)) for r in rows]
        vals = [v for v in vals if v is not None and v > 0]
        if vals:
            out[label] = int(mean(vals))
    return out


def pointer_latency_points(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        size_b = _to_int(row.get("working_set_bytes"))
        latency = _to_float(row.get("latency_ns"))
        if size_b is None or size_b <= 0 or latency is None:
            continue
        grouped[size_b].append(row)

    points: list[dict[str, Any]] = []
    for size_b in sorted(grouped.keys()):
        sample = grouped[size_b]
        latencies = [_to_float(r.get("latency_ns")) for r in sample]
        latencies = [v for v in latencies if v is not None]
        if not latencies:
            continue
        residencies = [str(r.get("estimated_residency", "")).strip() for r in sample]
        residencies = [v for v in residencies if v]
        residency = residencies[0] if residencies else "unknown"
        points.append(
            {
                "working_set_bytes": size_b,
                "working_set_kb": size_b / 1024.0,
                "latency_ns_mean": mean(latencies),
                "residency": residency,
            }
        )
    return points


def aggregated_latency_points(
    rows: list[dict[str, Any]],
    *,
    x_field: str,
    label_field: str | None = None,
) -> list[dict[str, Any]]:
    grouped: dict[int, list[dict[str, Any]]] = defaultdict(list)
    for row in rows:
        x = _to_int(row.get(x_field))
        latency = _to_float(row.get("latency_ns"))
        if x is None or x <= 0 or latency is None:
            continue
        grouped[x].append(row)

    points: list[dict[str, Any]] = []
    for x in sorted(grouped.keys()):
        sample = grouped[x]
        latencies = [_to_float(r.get("latency_ns")) for r in sample]
        latencies = [v for v in latencies if v is not None]
        if not latencies:
            continue
        label = "unknown"
        if label_field:
            values = [str(r.get(label_field, "")).strip() for r in sample]
            values = [v for v in values if v]
            if values:
                label = values[0]
        points.append(
            {
                "x": x,
                "latency_ns_mean": mean(latencies),
                "latency_ns_median": median(latencies),
                "latency_ns_std": pstdev(latencies) if len(latencies) > 1 else 0.0,
                "n_runs": len(latencies),
                "label": label,
            }
        )
    return points


def latency_summary_by_residency(rows: list[dict[str, Any]]) -> list[dict[str, Any]]:
    grouped: dict[str, list[dict[str, Any]]] = defaultdict(list)
    for point in pointer_latency_points(rows):
        grouped[str(point.get("residency", "unknown"))].append(point)

    ordered = [
        "L1(all cores)",
        "P-L1 / E-L2",
        "L2(all cores)",
        "P-L2 / E-DRAM",
        "L1",
        "L2",
        "L3",
        "DRAM",
        "unknown",
    ]
    out: list[dict[str, Any]] = []
    for name in ordered:
        points = grouped.get(name, [])
        if not points:
            continue
        latencies = [float(p["latency_ns_mean"]) for p in points]
        sizes = [float(p["working_set_kb"]) for p in points]
        out.append(
            {
                "residency": name,
                "latency_ns_mean": mean(latencies),
                "working_set_kb_min": min(sizes),
                "working_set_kb_max": max(sizes),
                "n_points": len(points),
            }
        )
    return out


def add_cache_boundary_lines(ax: Any, rows: list[dict[str, Any]]) -> None:
    boundaries = cache_boundaries_from_rows(rows)
    ymax = None
    try:
        ymax = ax.get_ylim()[1]
    except Exception:
        pass
    for label, size_b in sorted(boundaries.items(), key=lambda item: item[1]):
        ax.axvline(size_b, color="#6b7280", linestyle="--", linewidth=1.0, alpha=0.7)
        if ymax is not None:
            ax.text(
                size_b,
                ymax * 0.96,
                label,
                rotation=90,
                va="top",
                ha="right",
                fontsize=8,
                color="#4b5563",
                backgroundcolor="white",
            )
