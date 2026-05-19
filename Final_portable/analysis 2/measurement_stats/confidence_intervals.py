from __future__ import annotations

from math import sqrt
from statistics import mean, stdev
from typing import Sequence

_T_CRITICAL_95 = {
    1: 12.706,
    2: 4.303,
    3: 3.182,
    4: 2.776,
    5: 2.571,
    6: 2.447,
    7: 2.365,
    8: 2.306,
    9: 2.262,
    10: 2.228,
    11: 2.201,
    12: 2.179,
    13: 2.160,
    14: 2.145,
    15: 2.131,
    16: 2.120,
    17: 2.110,
    18: 2.101,
    19: 2.093,
    20: 2.086,
    21: 2.080,
    22: 2.074,
    23: 2.069,
    24: 2.064,
    25: 2.060,
    26: 2.056,
    27: 2.052,
    28: 2.048,
    29: 2.045,
    30: 2.042,
}


def _critical_value_95(df: int) -> float:
    if df <= 0:
        return 0.0
    if df in _T_CRITICAL_95:
        return _T_CRITICAL_95[df]
    if df < 60:
        return 2.0
    return 1.96


def mean_confidence_interval(values: Sequence[float], confidence: float = 0.95) -> tuple[float, float]:
    vals = [float(v) for v in values]
    n = len(vals)
    if n == 0:
        return (0.0, 0.0)
    mu = mean(vals)
    if n == 1:
        return (mu, mu)
    if confidence != 0.95:
        # conservative fallback until more confidence levels are needed
        critical = 1.96
    else:
        critical = _critical_value_95(n - 1)
    sigma = stdev(vals)
    radius = critical * sigma / max(sqrt(n), 1e-12)
    return (mu - radius, mu + radius)
