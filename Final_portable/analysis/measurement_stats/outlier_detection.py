from __future__ import annotations

from typing import Sequence
import numpy as np


def detect_outliers_mad(values: Sequence[float], threshold: float = 3.5) -> list[bool]:
    vals = np.asarray([float(v) for v in values], dtype=float)
    if vals.size == 0:
        return []
    median = float(np.median(vals))
    abs_dev = np.abs(vals - median)
    mad = float(np.median(abs_dev))
    if mad <= 1e-15:
        return [False] * int(vals.size)
    modified_z = 0.6745 * abs_dev / mad
    return [bool(v > threshold) for v in modified_z]
