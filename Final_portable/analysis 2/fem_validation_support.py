from __future__ import annotations

from typing import Any


def config_to_exact_row(config: dict[str, Any]) -> list[int]:
    padding = int(config.get("padding", 0) or 0)
    return [
        int(config.get("coal_read", 0) or 0),
        int(config.get("coal_write", 0) or 0),
        int(config.get("compute_all_shape_fun_der", 0) or 0),
        int(config.get("use_workspace_for_pde_coeff", 0) or 0),
        int(config.get("use_workspace_for_geo_data", 0) or 0),
        int(config.get("use_workspace_for_shape_fun", 0) or 0),
        int(config.get("use_workspace_for_stiff_mat", 0) or 0),
        1 if padding == 0 else 0,
        1 if padding == 1 else 0,
    ]


def row_hamming_distance(lhs: list[int], rhs: list[int]) -> int:
    width = min(len(lhs), len(rhs))
    return sum(1 for idx in range(width) if int(lhs[idx]) != int(rhs[idx])) + abs(len(lhs) - len(rhs))


def compute_option_alignment(
    best_overall: dict[str, Any],
    probe_summary: dict[str, Any],
    probe_catalog: dict[str, Any] | None = None,
) -> list[dict[str, Any]]:
    config = dict(best_overall.get("config") or {})
    catalog = dict(probe_catalog or {})
    rows: list[dict[str, Any]] = []
    for probe_id, probe in sorted(probe_summary.items()):
        if not isinstance(probe, dict):
            continue
        meta = dict(catalog.get(probe_id) or {})
        alignment_control = str(meta.get("alignment_control", "")).strip()
        if not alignment_control:
            continue
        mean_ratio = probe.get("mean_delta_ratio")
        enabled = int(config.get(alignment_control, 0) or 0)
        support = None
        recommendation = None
        if mean_ratio is not None:
            recommendation = "enable" if float(mean_ratio) < 1.0 else "disable"
            support = bool((enabled == 1 and recommendation == "enable") or (enabled == 0 and recommendation == "disable"))
        rows.append(
            {
                "probe_id": probe_id,
                "probe_label": meta.get("label", probe_id),
                "category": meta.get("category", ""),
                "alignment_control": alignment_control,
                "best_config_enabled": enabled,
                "mean_delta_ratio": mean_ratio,
                "recommended_state": recommendation,
                "supports_best_config": support,
                "related_controls": list(meta.get("related_controls") or []),
                "rationale": meta.get("rationale", ""),
            }
        )
    return rows


def compute_profile_proximity(
    best_overall: dict[str, Any],
    probe_catalog: dict[str, Any] | None = None,
) -> list[dict[str, Any]]:
    catalog = dict(probe_catalog or {})
    config = dict(best_overall.get("config") or {})
    if not config:
        return []
    best_row = config_to_exact_row(config)
    rows: list[dict[str, Any]] = []
    for probe_id, meta_raw in sorted(catalog.items()):
        meta = dict(meta_raw or {})
        if str(meta.get("category", "")) != "profile":
            continue
        baseline_row = list(meta.get("baseline_row") or [])
        toggled_row = list(meta.get("toggled_row") or [])
        if not baseline_row or not toggled_row:
            continue
        baseline_dist = row_hamming_distance(best_row, baseline_row)
        toggled_dist = row_hamming_distance(best_row, toggled_row)
        rows.append(
            {
                "probe_id": probe_id,
                "probe_label": meta.get("label", probe_id),
                "baseline_label": meta.get("baseline_label", "baseline"),
                "toggled_label": meta.get("toggled_label", "toggled"),
                "baseline_distance": baseline_dist,
                "toggled_distance": toggled_dist,
                "closer_to": meta.get("baseline_label", "baseline") if baseline_dist <= toggled_dist else meta.get("toggled_label", "toggled"),
                "related_controls": list(meta.get("related_controls") or []),
                "rationale": meta.get("rationale", ""),
            }
        )
    return rows

