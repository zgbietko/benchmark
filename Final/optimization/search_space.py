from __future__ import annotations

from dataclasses import dataclass
import math
import random
from typing import Any, Iterable, List


@dataclass(frozen=True)
class ContinuousVariable:
    name: str
    low: float
    high: float

    kind: str = "continuous"

    def __post_init__(self) -> None:
        if self.high <= self.low:
            raise ValueError(f"{self.name}: high must be > low")

    def decode(self, unit_value: float) -> float:
        u = min(1.0, max(0.0, float(unit_value)))
        return self.low + u * (self.high - self.low)

    def encode(self, value: float) -> float:
        if self.high == self.low:
            return 0.0
        u = (float(value) - self.low) / (self.high - self.low)
        return min(1.0, max(0.0, u))


@dataclass(frozen=True)
class IntegerVariable:
    name: str
    low: int
    high: int
    step: int = 1

    kind: str = "integer"

    def __post_init__(self) -> None:
        if self.high < self.low:
            raise ValueError(f"{self.name}: high must be >= low")
        if self.step <= 0:
            raise ValueError(f"{self.name}: step must be > 0")

    def decode(self, unit_value: float) -> int:
        u = min(1.0, max(0.0, float(unit_value)))
        span = self.high - self.low
        if span <= 0:
            return self.low
        raw = self.low + int(round(u * span))
        snapped = self.low + int(round((raw - self.low) / self.step)) * self.step
        return min(self.high, max(self.low, snapped))

    def encode(self, value: int) -> float:
        if self.high == self.low:
            return 0.0
        v = min(self.high, max(self.low, int(value)))
        return float(v - self.low) / float(self.high - self.low)


@dataclass(frozen=True)
class CategoricalVariable:
    name: str
    choices: List[Any]

    kind: str = "categorical"

    def __post_init__(self) -> None:
        if len(self.choices) == 0:
            raise ValueError(f"{self.name}: choices cannot be empty")

    def decode(self, unit_value: float) -> Any:
        u = min(1.0, max(0.0, float(unit_value)))
        if len(self.choices) == 1:
            return self.choices[0]
        idx = int(round(u * (len(self.choices) - 1)))
        idx = min(len(self.choices) - 1, max(0, idx))
        return self.choices[idx]

    def encode(self, value: Any) -> float:
        if len(self.choices) == 1:
            return 0.0
        try:
            idx = self.choices.index(value)
        except ValueError:
            idx = 0
        return float(idx) / float(len(self.choices) - 1)


Variable = ContinuousVariable | IntegerVariable | CategoricalVariable


class SearchSpace:
    """Mixed-variable search space encoded internally as a unit hypercube [0, 1]^D."""

    def __init__(self, variables: Iterable[Variable]) -> None:
        self.variables: List[Variable] = list(variables)
        if not self.variables:
            raise ValueError("SearchSpace requires at least one variable")

    @property
    def dim(self) -> int:
        return len(self.variables)

    def sample_position(self, rng: random.Random) -> List[float]:
        return [rng.random() for _ in range(self.dim)]

    def clip_position(self, position: List[float]) -> List[float]:
        if len(position) != self.dim:
            raise ValueError(f"Position dimension mismatch: got {len(position)}, expected {self.dim}")
        return [min(1.0, max(0.0, float(x))) for x in position]

    def decode(self, position: List[float]) -> dict[str, Any]:
        p = self.clip_position(position)
        out: dict[str, Any] = {}
        for var, unit in zip(self.variables, p):
            out[var.name] = var.decode(unit)
        return out

    def encode(self, config: dict[str, Any]) -> List[float]:
        out: List[float] = []
        for var in self.variables:
            out.append(var.encode(config[var.name]))
        return out

    def distance(self, a: List[float], b: List[float]) -> float:
        if len(a) != self.dim or len(b) != self.dim:
            raise ValueError("distance: dimension mismatch")
        return math.sqrt(sum((float(x) - float(y)) ** 2 for x, y in zip(a, b)))
