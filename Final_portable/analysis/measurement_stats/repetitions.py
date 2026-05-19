from __future__ import annotations

from dataclasses import dataclass, asdict
from statistics import mean, median, pstdev
from typing import Any, Sequence
import math
import numpy as np

from .confidence_intervals import mean_confidence_interval
from .outlier_detection import detect_outliers_mad


@dataclass
class SampleStatistics:
    repetitions: int
    warmup_repetitions: int
    valid_repetitions: int
    outliers_removed: int
    mean: float
    median: float
    std: float
    minimum: float
    maximum: float
    coefficient_of_variation: float
    ci95_low: float
    ci95_high: float
    values_used: list[float]
    values_dropped: list[float]

    def to_dict(self) -> dict[str, Any]:
        return asdict(self)


def compute_sample_statistics(
    values: Sequence[float],
    *,
    warmup_repetitions: int = 0,
    reject_outliers: bool = True,
) -> SampleStatistics:
    numeric = [float(v) for v in values if v is not None and math.isfinite(float(v))]
    repetitions = len(numeric)
    trimmed = numeric[int(max(warmup_repetitions, 0)) :]
    dropped: list[float] = []
    used = list(trimmed)
    if reject_outliers and len(trimmed) >= 4:
        mask = detect_outliers_mad(trimmed)
        used = [v for v, bad in zip(trimmed, mask) if not bad]
        dropped = [v for v, bad in zip(trimmed, mask) if bad]
        if not used:
            used = list(trimmed)
            dropped = []
    if not used:
        used = [0.0]
    mu = float(mean(used))
    med = float(median(used))
    std = float(pstdev(used)) if len(used) > 1 else 0.0
    vmin = float(min(used))
    vmax = float(max(used))
    cv = float(std / mu) if abs(mu) > 1e-15 else 0.0
    ci_low, ci_high = mean_confidence_interval(used, 0.95)
    return SampleStatistics(
        repetitions=repetitions,
        warmup_repetitions=int(max(warmup_repetitions, 0)),
        valid_repetitions=len(used),
        outliers_removed=len(dropped),
        mean=mu,
        median=med,
        std=std,
        minimum=vmin,
        maximum=vmax,
        coefficient_of_variation=cv,
        ci95_low=float(ci_low),
        ci95_high=float(ci_high),
        values_used=[float(v) for v in used],
        values_dropped=[float(v) for v in dropped],
    )
