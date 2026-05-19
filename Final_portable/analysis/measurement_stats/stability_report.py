from __future__ import annotations

from typing import Any


def build_stability_report(*, coefficient_of_variation: float, outliers_removed: int, valid_repetitions: int) -> dict[str, Any]:
    cv = float(coefficient_of_variation)
    if valid_repetitions < 3:
        status = "warning"
        label = "zbyt mało powtórzeń"
    elif cv <= 0.03 and outliers_removed == 0:
        status = "stable"
        label = "stabilny"
    elif cv <= 0.08:
        status = "acceptable"
        label = "akceptowalny"
    else:
        status = "unstable"
        label = "niestabilny"
    confidence = 0.95 if status == "stable" else (0.75 if status == "acceptable" else 0.4)
    return {
        "status": status,
        "label": label,
        "coefficient_of_variation": cv,
        "outliers_removed": int(outliers_removed),
        "valid_repetitions": int(valid_repetitions),
        "confidence": float(confidence),
    }
