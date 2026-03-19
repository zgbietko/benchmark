from __future__ import annotations

from dataclasses import dataclass, asdict
from datetime import datetime, timezone
import json
import math
import random
from pathlib import Path
from typing import Any, Dict, List, TextIO

from .objectives import BrightnessModel, ScoredItem
from .problem import OptimizationProblem, EvaluationResult


@dataclass
class FireflyConfig:
    population_size: int = 16
    iterations: int = 25
    alpha: float = 0.25
    beta0: float = 1.0
    gamma: float = 1.0
    alpha_damp: float = 0.97
    seed: int = 42

    def __post_init__(self) -> None:
        if self.population_size <= 0:
            raise ValueError("population_size must be > 0")
        if self.iterations <= 0:
            raise ValueError("iterations must be > 0")
        if self.alpha < 0:
            raise ValueError("alpha must be >= 0")
        if self.beta0 < 0:
            raise ValueError("beta0 must be >= 0")
        if self.gamma < 0:
            raise ValueError("gamma must be >= 0")
        if not (0.0 < self.alpha_damp <= 1.0):
            raise ValueError("alpha_damp must be in (0, 1]")


@dataclass
class FireflyAgent:
    idx: int
    position: List[float]
    config: Dict[str, Any] | None = None
    result: EvaluationResult | None = None
    brightness: float = float("-inf")


@dataclass
class OptimizationResult:
    best_config: Dict[str, Any]
    best_metrics: Dict[str, float]
    best_brightness: float
    pareto_front: List[Dict[str, Any]]
    out_dir: Path


class FireflyOptimizer:
    """Generic Firefly Algorithm implementation for mixed-variable benchmark tuning."""

    def __init__(
        self,
        problem: OptimizationProblem,
        objective_model: BrightnessModel,
        config: FireflyConfig,
        out_dir: Path,
    ) -> None:
        self.problem = problem
        self.objective_model = objective_model
        self.config = config
        self.out_dir = out_dir

        self.rng = random.Random(config.seed)
        self.population: List[FireflyAgent] = []
        self._eval_fp: TextIO | None = None
        self._iter_fp: TextIO | None = None

    def _initialize(self) -> None:
        self.population = []
        for i in range(self.config.population_size):
            pos = self.problem.search_space.sample_position(self.rng)
            self.population.append(FireflyAgent(idx=i, position=pos))

    def _open_log_files(self) -> None:
        self.out_dir.mkdir(parents=True, exist_ok=True)
        self._eval_fp = (self.out_dir / "evaluations.jsonl").open("w", encoding="utf-8")
        self._iter_fp = (self.out_dir / "iterations.jsonl").open("w", encoding="utf-8")

    def _close_log_files(self) -> None:
        if self._eval_fp is not None:
            self._eval_fp.close()
            self._eval_fp = None
        if self._iter_fp is not None:
            self._iter_fp.close()
            self._iter_fp = None

    def _write_eval_row(self, row: Dict[str, Any]) -> None:
        if self._eval_fp is not None:
            self._eval_fp.write(json.dumps(row, ensure_ascii=True) + "\n")

    def _write_iteration_row(self, row: Dict[str, Any]) -> None:
        if self._iter_fp is not None:
            self._iter_fp.write(json.dumps(row, ensure_ascii=True) + "\n")

    @staticmethod
    def _short_text(v: Any, max_len: int = 120) -> str:
        txt = str(v)
        if len(txt) <= max_len:
            return txt
        return txt[:max_len] + f"...(+{len(txt) - max_len} chars)"

    def _artifact_to_log_value(self, key: str, value: Any) -> tuple[Any, Dict[str, Any]]:
        extra: Dict[str, Any] = {}
        if isinstance(value, (str, int, float, bool)) or value is None:
            if isinstance(value, str):
                return self._short_text(value, max_len=256), extra
            return value, extra

        if isinstance(value, list):
            n = len(value)
            max_items = 8
            preview = [self._short_text(x, max_len=64) for x in value[:max_items]]
            txt = "|".join(preview)
            if n > max_items:
                txt = txt + f"|...(+{n - max_items} items)"
            extra[f"artifact_{key}_len"] = n
            return txt, extra

        if isinstance(value, dict):
            items = list(value.items())
            max_items = 8
            preview = [f"{k}={self._short_text(v, max_len=48)}" for k, v in items[:max_items]]
            txt = "|".join(preview)
            if len(items) > max_items:
                txt = txt + f"|...(+{len(items) - max_items} keys)"
            extra[f"artifact_{key}_len"] = len(items)
            return txt, extra

        return self._short_text(value, max_len=256), extra

    def _evaluate_population(self, iteration: int) -> None:
        scored_items: List[ScoredItem] = []

        for ff in self.population:
            cfg = self.problem.search_space.decode(ff.position)
            res = self.problem.evaluate(cfg)
            ff.config = cfg
            ff.result = res
            scored_items.append(
                ScoredItem(feasible=res.constraints_ok and res.status == "ok", metrics=res.metrics, violations=res.violations)
            )

        brightness = self.objective_model.brightness(scored_items)
        for ff, b in zip(self.population, brightness):
            ff.brightness = float(b)
            row: Dict[str, Any] = {
                "timestamp": datetime.now(timezone.utc).isoformat(),
                "iteration": iteration,
                "firefly_id": ff.idx,
                "brightness": ff.brightness,
                "status": ff.result.status if ff.result is not None else "unknown",
                "constraints_ok": int(ff.result.constraints_ok if ff.result is not None else False),
                "violations": "|".join(ff.result.violations) if ff.result is not None else "",
            }
            if ff.config is not None:
                for k, v in ff.config.items():
                    row[f"cfg_{k}"] = v
            if ff.result is not None:
                for k, v in ff.result.metrics.items():
                    row[f"metric_{k}"] = v
                for k, v in ff.result.artifacts.items():
                    key = f"artifact_{k}"
                    val, extra = self._artifact_to_log_value(k, v)
                    row[key] = val
                    row.update(extra)
            self._write_eval_row(row)

    def _move_population(self) -> None:
        alpha = self.config.alpha
        beta0 = self.config.beta0
        gamma = self.config.gamma

        new_positions: List[List[float]] = []
        for i, ff_i in enumerate(self.population):
            x = list(ff_i.position)
            for j, ff_j in enumerate(self.population):
                if j == i:
                    continue
                if ff_j.brightness <= ff_i.brightness:
                    continue
                r = self.problem.search_space.distance(x, ff_j.position)
                beta = beta0 * math.exp(-gamma * (r ** 2))
                for d in range(len(x)):
                    rand_term = alpha * (self.rng.random() - 0.5)
                    x[d] = x[d] + beta * (ff_j.position[d] - x[d]) + rand_term
            x = self.problem.search_space.clip_position(x)
            new_positions.append(x)

        for ff, new_pos in zip(self.population, new_positions):
            ff.position = new_pos

        self.config.alpha *= self.config.alpha_damp

    def _get_pareto_front(self) -> List[FireflyAgent]:
        valid = [ff for ff in self.population if ff.result is not None and ff.result.status == "ok" and ff.result.constraints_ok]
        if not valid:
            return []

        def dominates(a: FireflyAgent, b: FireflyAgent) -> bool:
            am = a.result.metrics
            bm = b.result.metrics
            # Dominance on common useful metrics if present.
            # Maximize perf, minimize energy indicators.
            terms = [
                ("gbps_mean", "max"),
                ("gflops_mean", "max"),
                ("j_per_gb", "min"),
                ("j_per_gflop", "min"),
                ("edp", "min"),
            ]
            active = [(m, s) for (m, s) in terms if (m in am and m in bm)]
            if not active:
                return False
            better_or_equal = True
            strictly = False
            for m, s in active:
                va = float(am[m])
                vb = float(bm[m])
                if s == "max":
                    if va < vb:
                        better_or_equal = False
                        break
                    if va > vb:
                        strictly = True
                else:
                    if va > vb:
                        better_or_equal = False
                        break
                    if va < vb:
                        strictly = True
            return better_or_equal and strictly

        front: List[FireflyAgent] = []
        for a in valid:
            dominated = False
            for b in valid:
                if a is b:
                    continue
                if dominates(b, a):
                    dominated = True
                    break
            if not dominated:
                front.append(a)
        return front

    def _write_history(self) -> None:
        if self._eval_fp is not None:
            self._eval_fp.flush()
        if self._iter_fp is not None:
            self._iter_fp.flush()

    def run(self) -> OptimizationResult:
        self._initialize()
        self._open_log_files()

        try:
            best: FireflyAgent | None = None
            for it in range(self.config.iterations):
                self._evaluate_population(iteration=it)
                self.population.sort(key=lambda ff: ff.brightness, reverse=True)

                if best is None or self.population[0].brightness > best.brightness:
                    best = FireflyAgent(
                        idx=self.population[0].idx,
                        position=list(self.population[0].position),
                        config=dict(self.population[0].config or {}),
                        result=self.population[0].result,
                        brightness=self.population[0].brightness,
                    )

                best_metrics = dict(best.result.metrics) if (best is not None and best.result is not None) else {}
                self._write_iteration_row(
                    {
                        "timestamp": datetime.now(timezone.utc).isoformat(),
                        "iteration": it,
                        "best_brightness": best.brightness if best is not None else float("-inf"),
                        "best_status": best.result.status if (best is not None and best.result is not None) else "unknown",
                        "best_metrics": best_metrics,
                        "alpha": self.config.alpha,
                    }
                )

                self._move_population()

            self._evaluate_population(iteration=self.config.iterations)
            self.population.sort(key=lambda ff: ff.brightness, reverse=True)
            if best is None or self.population[0].brightness > best.brightness:
                best = self.population[0]

            pareto = self._get_pareto_front()
            self._write_history()

            best_cfg = dict(best.config or {})
            best_metrics = dict(best.result.metrics if best.result is not None else {})

            summary = {
                "problem": self.problem.name,
                "config": asdict(self.config),
                "best_config": best_cfg,
                "best_metrics": best_metrics,
                "best_brightness": best.brightness,
                "pareto_size": len(pareto),
                "pareto": [
                    {
                        "brightness": ff.brightness,
                        "config": ff.config,
                        "metrics": (ff.result.metrics if ff.result is not None else {}),
                    }
                    for ff in pareto
                ],
            }
            (self.out_dir / "summary.json").write_text(json.dumps(summary, indent=2, ensure_ascii=True), encoding="utf-8")
            (self.out_dir / "best.json").write_text(
                json.dumps(
                    {
                        "best_config": best_cfg,
                        "best_metrics": best_metrics,
                        "best_brightness": best.brightness,
                    },
                    indent=2,
                    ensure_ascii=True,
                ),
                encoding="utf-8",
            )
            with (self.out_dir / "pareto_front.jsonl").open("w", encoding="utf-8") as f:
                for ff in pareto:
                    f.write(
                        json.dumps(
                            {
                                "brightness": ff.brightness,
                                "config": dict(ff.config or {}),
                                "metrics": dict(ff.result.metrics if ff.result is not None else {}),
                            },
                            ensure_ascii=True,
                        )
                        + "\\n"
                    )

            return OptimizationResult(
                best_config=best_cfg,
                best_metrics=best_metrics,
                best_brightness=best.brightness,
                pareto_front=[
                    {
                        "brightness": ff.brightness,
                        "config": dict(ff.config or {}),
                        "metrics": dict(ff.result.metrics if ff.result is not None else {}),
                    }
                    for ff in pareto
                ],
                out_dir=self.out_dir,
            )
        finally:
            self._close_log_files()
