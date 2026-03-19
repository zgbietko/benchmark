from __future__ import annotations

from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Any, Dict, List

from .search_space import SearchSpace


@dataclass
class EvaluationResult:
    status: str
    metrics: Dict[str, float] = field(default_factory=dict)
    artifacts: Dict[str, Any] = field(default_factory=dict)
    constraints_ok: bool = True
    violations: List[str] = field(default_factory=list)


class OptimizationProblem(ABC):
    """Injectable benchmark problem definition for the FA optimizer."""

    @property
    @abstractmethod
    def name(self) -> str:
        raise NotImplementedError

    @property
    @abstractmethod
    def search_space(self) -> SearchSpace:
        raise NotImplementedError

    @abstractmethod
    def evaluate(self, config: Dict[str, Any]) -> EvaluationResult:
        raise NotImplementedError

    def close(self) -> None:
        """Optional cleanup hook (GPU contexts, file handles, etc.)."""
        return
