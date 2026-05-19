from __future__ import annotations

from math import sqrt
from typing import Iterable, Any


def compare_numeric_sequences(reference: Iterable[float], candidate: Iterable[float], *, abs_tol: float = 1e-6, rel_tol: float = 1e-5) -> dict[str, Any]:
    ref = [float(v) for v in reference]
    cand = [float(v) for v in candidate]
    if len(ref) != len(cand):
        return {
            "status": "fail",
            "count_reference": len(ref),
            "count_candidate": len(cand),
            "max_abs_error": None,
            "max_rel_error": None,
            "reason": "różna liczba elementów",
        }
    max_abs = 0.0
    max_rel = 0.0
    sum_sq = 0.0
    sum_abs = 0.0
    for a, b in zip(ref, cand):
        diff = abs(a - b)
        rel = diff / max(abs(a), 1e-12)
        max_abs = max(max_abs, diff)
        max_rel = max(max_rel, rel)
        sum_sq += diff * diff
        sum_abs += diff
    rmse = sqrt(sum_sq / max(len(ref), 1))
    mae = sum_abs / max(len(ref), 1)
    status = "pass" if (max_abs <= abs_tol or max_rel <= rel_tol) else ("warning" if max_abs <= abs_tol * 10.0 or max_rel <= rel_tol * 10.0 else "fail")
    return {
        "status": status,
        "count_reference": len(ref),
        "count_candidate": len(cand),
        "max_abs_error": max_abs,
        "max_rel_error": max_rel,
        "rmse": rmse,
        "mae": mae,
        "abs_tol": abs_tol,
        "rel_tol": rel_tol,
    }
