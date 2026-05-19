from __future__ import annotations

from dataclasses import dataclass
import math
from typing import Iterable, List


@dataclass(frozen=True)
class ObjectiveTerm:
    metric: str
    sense: str = "max"  # max|min
    weight: float = 1.0

    def __post_init__(self) -> None:
        if self.sense not in ("max", "min"):
            raise ValueError(f"Invalid sense: {self.sense}")
        if self.weight <= 0:
            raise ValueError("weight must be > 0")


@dataclass
class ScoredItem:
    feasible: bool
    metrics: dict[str, float]
    violations: List[str]


class BrightnessModel:
    def brightness(self, items: List[ScoredItem]) -> List[float]:
        raise NotImplementedError


class WeightedSumBrightness(BrightnessModel):
    """Scalar brightness from weighted, normalized objective terms."""

    def __init__(self, terms: Iterable[ObjectiveTerm], infeasible_penalty: float = 10.0) -> None:
        self.terms = list(terms)
        if not self.terms:
            raise ValueError("WeightedSumBrightness requires at least one objective term")
        self.infeasible_penalty = float(infeasible_penalty)

    def brightness(self, items: List[ScoredItem]) -> List[float]:
        if not items:
            return []

        values_by_metric: dict[str, List[float]] = {t.metric: [] for t in self.terms}
        for it in items:
            if not it.feasible:
                continue
            for t in self.terms:
                v = it.metrics.get(t.metric)
                if v is not None and math.isfinite(float(v)):
                    values_by_metric[t.metric].append(float(v))

        metric_bounds: dict[str, tuple[float, float]] = {}
        for metric, vals in values_by_metric.items():
            if not vals:
                metric_bounds[metric] = (0.0, 1.0)
                continue
            lo = min(vals)
            hi = max(vals)
            if hi - lo < 1e-12:
                hi = lo + 1.0
            metric_bounds[metric] = (lo, hi)

        total_w = sum(t.weight for t in self.terms)
        out: List[float] = []
        for it in items:
            if not it.feasible:
                out.append(-self.infeasible_penalty - len(it.violations))
                continue

            score = 0.0
            for t in self.terms:
                lo, hi = metric_bounds[t.metric]
                raw_obj = it.metrics.get(t.metric)
                if raw_obj is None or not math.isfinite(float(raw_obj)):
                    # Missing/NaN metrics are always worst.
                    norm = 0.0
                else:
                    raw = float(raw_obj)
                    norm = (raw - lo) / (hi - lo)
                norm = min(1.0, max(0.0, norm))
                if t.sense == "min":
                    norm = 1.0 - norm
                score += t.weight * norm
            out.append(score / total_w)
        return out


class ParetoBrightness(BrightnessModel):
    """Pareto rank + crowding-distance brightness for multi-objective FA."""

    def __init__(self, terms: Iterable[ObjectiveTerm], infeasible_penalty: float = 10.0) -> None:
        self.terms = list(terms)
        if not self.terms:
            raise ValueError("ParetoBrightness requires at least one objective term")
        self.infeasible_penalty = float(infeasible_penalty)

    def _dominates(self, a: ScoredItem, b: ScoredItem) -> bool:
        better_or_equal_all = True
        strictly_better_any = False
        for t in self.terms:
            va_obj = a.metrics.get(t.metric)
            vb_obj = b.metrics.get(t.metric)
            if va_obj is None or not math.isfinite(float(va_obj)):
                return False
            if vb_obj is None or not math.isfinite(float(vb_obj)):
                # valid a dominates invalid b on this objective
                strictly_better_any = True
                continue
            va = float(va_obj)
            vb = float(vb_obj)
            if t.sense == "max":
                if va < vb:
                    better_or_equal_all = False
                    break
                if va > vb:
                    strictly_better_any = True
            else:
                if va > vb:
                    better_or_equal_all = False
                    break
                if va < vb:
                    strictly_better_any = True
        return better_or_equal_all and strictly_better_any

    def brightness(self, items: List[ScoredItem]) -> List[float]:
        n = len(items)
        if n == 0:
            return []

        feasible_idx = [i for i, it in enumerate(items) if it.feasible]
        infeasible_idx = [i for i, it in enumerate(items) if not it.feasible]

        rank = [10_000] * n
        crowding = [0.0] * n

        if feasible_idx:
            dom_count = {i: 0 for i in feasible_idx}
            dominates = {i: [] for i in feasible_idx}
            fronts: List[List[int]] = []

            for i in feasible_idx:
                for j in feasible_idx:
                    if i == j:
                        continue
                    if self._dominates(items[i], items[j]):
                        dominates[i].append(j)
                    elif self._dominates(items[j], items[i]):
                        dom_count[i] += 1

            current = [i for i in feasible_idx if dom_count[i] == 0]
            f = 0
            while current:
                fronts.append(current)
                for i in current:
                    rank[i] = f
                next_front: List[int] = []
                for i in current:
                    for j in dominates[i]:
                        dom_count[j] -= 1
                        if dom_count[j] == 0:
                            next_front.append(j)
                current = next_front
                f += 1

            for front in fronts:
                if len(front) <= 2:
                    for i in front:
                        crowding[i] = 1e9
                    continue
                for t in self.terms:
                    front_sorted = sorted(front, key=lambda idx: float(items[idx].metrics.get(t.metric, 0.0)))
                    crowding[front_sorted[0]] = 1e9
                    crowding[front_sorted[-1]] = 1e9
                    lo = float(items[front_sorted[0]].metrics.get(t.metric, 0.0))
                    hi = float(items[front_sorted[-1]].metrics.get(t.metric, 0.0))
                    den = hi - lo
                    if abs(den) < 1e-12:
                        continue
                    for k in range(1, len(front_sorted) - 1):
                        prev_v = float(items[front_sorted[k - 1]].metrics.get(t.metric, 0.0))
                        next_v = float(items[front_sorted[k + 1]].metrics.get(t.metric, 0.0))
                        crowding[front_sorted[k]] += abs(next_v - prev_v) / den

        out = [0.0] * n
        for i in feasible_idx:
            # Higher is better: lower rank + higher crowding.
            rank_term = 1.0 / (1.0 + float(rank[i]))
            crowding_term = 0.0 if crowding[i] >= 1e8 else min(1.0, crowding[i])
            out[i] = rank_term + 0.01 * crowding_term

        for i in infeasible_idx:
            out[i] = -self.infeasible_penalty - len(items[i].violations)

        return out


def parse_objective_terms(spec: str) -> List[ObjectiveTerm]:
    """
    Parse objective definition:
      "gbps_mean:max:1.0,j_per_gb:min:0.2"
    """
    terms: List[ObjectiveTerm] = []
    for chunk in spec.split(","):
        c = chunk.strip()
        if not c:
            continue
        parts = c.split(":")
        if len(parts) == 1:
            terms.append(ObjectiveTerm(metric=parts[0].strip(), sense="max", weight=1.0))
        elif len(parts) == 2:
            terms.append(ObjectiveTerm(metric=parts[0].strip(), sense=parts[1].strip(), weight=1.0))
        else:
            terms.append(
                ObjectiveTerm(
                    metric=parts[0].strip(),
                    sense=parts[1].strip(),
                    weight=float(parts[2].strip()),
                )
            )
    if not terms:
        raise ValueError("No objective terms parsed")
    return terms
