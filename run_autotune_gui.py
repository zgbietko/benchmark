#!/usr/bin/env python3
from __future__ import annotations

from array import array
from collections import deque
import csv
from dataclasses import dataclass
import json
import math
import os
from pathlib import Path
import platform
import plistlib
import queue
import re
import shlex
import shutil
import subprocess
import sys
import threading
import time
import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from tkinter.scrolledtext import ScrolledText
from typing import Any, Deque, Dict, Iterable, List, Tuple


ROOT = Path(__file__).resolve().parent
_mpl_cfg = ROOT / ".cache" / "matplotlib"
_mpl_cfg.mkdir(parents=True, exist_ok=True)
os.environ.setdefault("MPLCONFIGDIR", str(_mpl_cfg))

from device_catalog import discover_backends

try:
    import matplotlib
    matplotlib.use("TkAgg")
    from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg, NavigationToolbar2Tk
    from matplotlib.figure import Figure

    HAS_MPL = True
    MPL_IMPORT_ERROR = ""
except Exception as exc:
    HAS_MPL = False
    MPL_IMPORT_ERROR = f"{type(exc).__name__}: {exc}"
    FigureCanvasTkAgg = None  # type: ignore
    NavigationToolbar2Tk = None  # type: ignore
    Figure = None  # type: ignore


SCRIPT_CHOICES: List[str] = [
    "run_workflow.py",
    "run_all_benchmarks.py",
    "run_all_backends.py",
    "run_all_cpu_benchmarks.py",
    "run_all_gpu_benchmarks.py",
    "real_kernels/run_all_real_kernels.py",
    "run_firefly_optimization.py",
    "run_filip_original.py",
    "run_filip_reference_exact.py",
    "run_filip_autotuning.py",
    "run_fem_parametric_preflight.py",
    "run_device_discovery.py",
    "run_fem_parametric_matrix.py",
    "analysis/cpu_summary.py",
    "analysis/gpu_summary.py",
    "analysis/real_kernels_summary.py",
    "analysis/filip_article_plots.py",
    "analysis/compare_legacy_filip_xlsx.py",
    "analysis/roofline_model.py",
    "analysis/generate_plots.py",
    "analysis/report.py",
    "analysis/data_quality.py",
    "analysis/normalize_gpu_csv.py",
]

FILIP_CASE_CHOICES: List[str] = ["prism_pair", "laplace_prism", "test_prism", "portable"]
FILIP_CASE_DESCRIPTIONS: Dict[str, str] = {
    "prism_pair": "Filip-like prism campaign: laplace_prism + test_prism, 80 constrained option rows, QSS/SQS/SSQ.",
    "laplace_prism": "Strict Filip laplace_prism case on prism6 with 6 quadrature points.",
    "test_prism": "Strict Filip test_prism case on prism6 with 6 quadrature points.",
    "portable": "Portable fallback used earlier: tet4 with diffusion and diffusion_convection_mass.",
}
FILIP_MODE_CHOICES: List[str] = ["portable_sweep", "exact_reference"]
FILIP_MODE_DESCRIPTIONS: Dict[str, str] = {
    "portable_sweep": "Current project benchmark path. Exhaustive constrained sweep on the portable FEM harness.",
    "exact_reference": "Linux/OpenCL: original Filip path with mod_2022 rebuild and native OpenCL 'internal' timings. Apple/Metal: use the dedicated workflow fields below to point at an OpenCL exact dump root and replay translated Filip kernels on Metal. Without a replay dump root the fallback path is the exact-style Metal port.",
}

OPTIMIZATION_PLOT_PREFERENCE: List[str] = [
    "article_paper_option_times.png",
    "article_variant_option_times.png",
    "article_best_summary.png",
    "article_autotuning_overview.png",
    "article_memory_compute_breakdown.png",
    "article_backend_comparison.png",
]


WORKFLOWS: Dict[str, Dict[str, str]] = {
    "1. CPU benchmark": {
        "id": "cpu_benchmark",
        "description": "CPU microbenchmarks + session plots + CPU roofline.",
    },
    "2. GPU benchmark": {
        "id": "gpu_benchmark",
        "description": "Auto-detected GPU microbenchmarks + session plots + GPU roofline.",
    },
    "3. Real kernels CPU": {
        "id": "cpu_real_kernels",
        "description": "CPU microbenchmarks + CPU real kernels + plots + CPU roofline.",
    },
    "4. Real kernels GPU": {
        "id": "gpu_real_kernels",
        "description": "GPU microbenchmarks + GPU real kernels. HIP/OpenCL fall back to portable FEM-integration subset.",
    },
    "5. Filip original": {
        "id": "filip_original",
        "description": "Filip benchmark workflow. Choose either the portable sweep or the exact original OpenCL reference path.",
    },
    "6. Filip autotuning": {
        "id": "filip_autotune",
        "description": "Filip thesis-style FEM autotuning without firefly. Uses session-safe backend mapping and bounded defaults.",
    },
    "7. Filip firefly": {
        "id": "filip_firefly",
        "description": "Filip thesis-style FEM autotuning with the firefly optimizer and live evaluation progress.",
    },
}


TEMPLATES: Dict[str, Dict[str, str]] = {
    "[Campaign] Benchmarks (paper, auto)": {
        "script": "run_all_benchmarks.py",
        "args": "--profile paper --platform-profile auto --arch auto --backend auto --device-index 0 --no-real-kernels --no-interactive-arch",
    },
    "[Campaign] Benchmarks + real_kernels": {
        "script": "run_all_benchmarks.py",
        "args": "--profile paper --platform-profile auto --arch auto --backend auto --device-index 0 --with-real-kernels --no-interactive-arch",
    },
    "[Campaign] All GPU backends": {
        "script": "run_all_backends.py",
        "args": "--profile paper --platform-profile auto --arch auto --device-index 0 --no-interactive-arch",
    },
    "[CPU] All CPU benchmarks": {
        "script": "run_all_cpu_benchmarks.py",
        "args": "--arch-profile auto",
    },
    "[GPU] All GPU benchmarks (auto backend)": {
        "script": "run_all_gpu_benchmarks.py",
        "args": "--platform-profile auto --arch auto --backend auto --device-index 0",
    },
    "[GPU] List devices (all backends)": {
        "script": "run_all_gpu_benchmarks.py",
        "args": "--platform-profile auto --arch auto --backend all --list-devices",
    },
    "[Discovery] Available devices": {
        "script": "run_device_discovery.py",
        "args": "--backends auto",
    },
    "[Real Kernels] Core suite": {
        "script": "real_kernels/run_all_real_kernels.py",
        "args": "--backend all --device-index 0 --runs 3",
    },
    "[Real Kernels] + FEM integration": {
        "script": "real_kernels/run_all_real_kernels.py",
        "args": "--backend all --device-index 0 --runs 3 --with-fem-integration --fem-integration-element-type tet4 --fem-integration-operator diffusion_mass --fem-integration-n-qp 4",
    },
    "[Firefly] GPU memory": {
        "script": "run_firefly_optimization.py",
        "args": "--problem gpu_memory --backend metal --population 16 --iterations 25 --repeats 3 --size-mb-range 4:256 --iters-range 5:100 --objective-mode weighted --objectives gbps_mean:max:1.0,j_per_gb:min:0.2",
    },
    "[Filip] Original benchmark": {
        "script": "run_filip_original.py",
        "args": (
            "--backend metal "
            "--profile paper "
            "--repeats 3 "
            "--filip-mode portable_sweep "
            "--benchmark-case prism_pair "
            "--variants qss,sqs,ssq "
            "--dtype float32"
        ),
    },
    "[Filip] Exact reference benchmark": {
        "script": "run_workflow.py",
        "args": (
            "--workflow filip_original "
            "--profile paper "
            "--backend intel "
            "--filip-mode exact_reference "
            "--filip-case prism_pair"
        ),
    },
    "[Analysis] Legacy XLSX compare": {
        "script": "analysis/compare_legacy_filip_xlsx.py",
        "args": "--current-operator laplace",
    },
    "[Firefly] GPU FMA": {
        "script": "run_firefly_optimization.py",
        "args": "--problem gpu_fma --backend metal --population 16 --iterations 25 --repeats 3 --n-elements-m-range 0.25:16.0 --iters-inner-range 200:10000 --objective-mode weighted --objectives gflops_mean:max:1.0,j_per_gflop:min:0.3,edp:min:0.2",
    },
    "[Firefly] FEM Parametric (Metal safe)": {
        "script": "run_firefly_optimization.py",
        "args": (
            "--problem fem_parametric "
            "--backend metal "
            "--population 12 "
            "--iterations 20 "
            "--repeats 3 "
            "--fem-execution-policy native_only "
            "--fem-n-elements-range 20000:300000 "
            "--fem-n-qp-range 1:8 "
            "--fem-element-types tet4,hex8 "
            "--fem-operators diffusion,mass,convection,diffusion_mass,diffusion_convection_mass "
            "--fem-dtypes float32 "
            "--fem-variant-choices qss,sqs,ssq "
            "--fem-workgroup-sizes 32,64,128,256 "
            "--fem-use-workspace-pde-choices 0,1 "
            "--fem-use-workspace-geo-choices 0,1 "
            "--fem-use-workspace-shape-choices 0,1 "
            "--fem-use-workspace-stiff-choices 0,1 "
            "--fem-padding-choices 0,1 "
            "--fem-compute-all-shape-der-choices 0,1 "
            "--fem-coal-read-choices 0,1 "
            "--fem-coal-write-choices 0,1 "
            "--fem-memory-budget-mb 768 "
            "--fem-screening-repeats 1 "
            "--fem-screening-prune-factor 0.6 "
            "--fem-mapped-max-n-fma-light 300000 "
            "--fem-mapped-max-buffer-mb-light 32 "
            "--fem-mapped-max-mem-iters 64 "
            "--fem-mapped-max-inner-iters-light 2048"
        ),
    },
    "[Firefly] FEM Parametric (CUDA)": {
        "script": "run_firefly_optimization.py",
        "args": (
            "--problem fem_parametric "
            "--backend cuda "
            "--population 24 "
            "--iterations 40 "
            "--repeats 5 "
            "--fem-execution-policy native_only "
            "--fem-n-elements-range 20000:500000 "
            "--fem-n-qp-range 1:8 "
            "--fem-element-types tet4,hex8 "
            "--fem-operators diffusion,mass,convection,diffusion_mass,diffusion_convection_mass "
            "--fem-dtypes float32,float64 "
            "--fem-variant-choices qss,sqs,ssq "
            "--fem-workgroup-sizes 32,64,128,256,512 "
            "--fem-use-workspace-pde-choices 0,1 "
            "--fem-use-workspace-geo-choices 0,1 "
            "--fem-use-workspace-shape-choices 0,1 "
            "--fem-use-workspace-stiff-choices 0,1 "
            "--fem-padding-choices 0,1 "
            "--fem-compute-all-shape-der-choices 0,1 "
            "--fem-coal-read-choices 0,1 "
            "--fem-coal-write-choices 0,1"
        ),
    },
    "[Validation] Preflight (all backends)": {
        "script": "run_fem_parametric_preflight.py",
        "args": "--backend all --execution-policy native_only --strict",
    },
    "[Validation] Matrix (vs OpenCL)": {
        "script": "run_fem_parametric_matrix.py",
        "args": (
            "--backends cpu,cuda,hip,opencl,metal,amd,intel "
            "--baseline opencl "
            "--n-configs 24 "
            "--repeats 2 "
            "--execution-policy native_only"
        ),
    },
    "[Analysis] CPU summary (latest)": {
        "script": "analysis/cpu_summary.py",
        "args": "--mode latest",
    },
    "[Analysis] GPU summary (latest)": {
        "script": "analysis/gpu_summary.py",
        "args": "--mode latest",
    },
    "[Analysis] Real kernels summary": {
        "script": "analysis/real_kernels_summary.py",
        "args": "",
    },
    "[Analysis] Filip article plots": {
        "script": "analysis/filip_article_plots.py",
        "args": "--mode latest",
    },
    "[Analysis] Roofline both (session)": {
        "script": "analysis/roofline_model.py",
        "args": "--target both --backend metal --ai 8 --bytes 1000000000 --scope session --session latest",
    },
    "[Analysis] Generate plots": {
        "script": "analysis/generate_plots.py",
        "args": "",
    },
    "[Analysis] Report + plots": {
        "script": "analysis/report.py",
        "args": "--mode latest --roofline-target both --roofline-backend metal --roofline-ai 8 --roofline-bytes 1000000000 --with-plots",
    },
    "[Analysis] Data quality (strict session)": {
        "script": "analysis/data_quality.py",
        "args": "--scope session --strict",
    },
    "[Maintenance] Normalize GPU CSV (latest)": {
        "script": "analysis/normalize_gpu_csv.py",
        "args": "--mode latest",
    },
}


def _is_root() -> bool:
    if hasattr(os, "geteuid"):
        return os.geteuid() == 0
    return False


def _get_arg_value(args: List[str], name: str, default: str) -> str:
    try:
        idx = args.index(name)
    except ValueError:
        return default
    if idx + 1 < len(args):
        return args[idx + 1]
    return default


def _to_int_or_default(text: str, default: int) -> int:
    try:
        return int(text)
    except Exception:
        return int(default)


def _parse_csv_arg(text: str) -> List[str]:
    return [item.strip() for item in str(text).split(",") if item.strip()]


def _format_hms(seconds: float) -> str:
    s = max(0, int(seconds))
    h = s // 3600
    m = (s % 3600) // 60
    sec = s % 60
    return f"{h:02d}:{m:02d}:{sec:02d}"


def _script_match(script: str, target: str) -> bool:
    s = script.strip().replace("\\", "/")
    t = target.strip().replace("\\", "/")
    return s == t or s.endswith("/" + t)


def _read_json(path: Path) -> Dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _read_jsonl(path: Path, max_points: int = 8000) -> List[Dict[str, Any]]:
    if not path.exists():
        return []
    rows: List[Dict[str, Any]] = []
    stride = 1
    idx = 0
    with path.open("r", encoding="utf-8") as f:
        for line in f:
            line = line.strip()
            if not line:
                continue
            try:
                obj = json.loads(line)
            except Exception:
                continue
            if idx % stride == 0:
                rows.append(obj)
                if len(rows) > max_points:
                    # Keep bounded memory by thinning samples online.
                    rows = rows[::2]
                    stride *= 2
            idx += 1
    return rows


def _prepend_env_path(env: Dict[str, str], key: str, values: List[str]) -> None:
    merged: List[str] = []
    for value in values + env.get(key, "").split(os.pathsep):
        value = str(value).strip()
        if value and value not in merged:
            merged.append(value)
    if merged:
        env[key] = os.pathsep.join(merged)


def _preferred_python_executable() -> str:
    venv_python = ROOT / ".venv" / "bin" / "python"
    if venv_python.exists() and os.access(venv_python, os.X_OK):
        return str(venv_python)
    return sys.executable


def _build_runtime_env() -> Dict[str, str]:
    env = dict(os.environ)
    env["MPLCONFIGDIR"] = str(_mpl_cfg)
    if platform.system().lower() == "linux":
        _prepend_env_path(env, "PATH", [str(ROOT / ".venv" / "bin"), "/opt/intel/oneapi/compiler/latest/linux/bin"])
        lib_paths = [
            "/opt/intel/oneapi/compiler/latest/linux/compiler/lib/intel64_lin",
            "/opt/intel/oneapi/mkl/latest/lib/intel64",
        ]
        _prepend_env_path(env, "LD_LIBRARY_PATH", lib_paths)
        env.setdefault("OCL_ICD_FILENAMES", str(env.get("OCL_ICD_FILENAMES", "")))
    return env


def _preview_float32_binary(path: Path, max_values: int = 64) -> Dict[str, Any]:
    raw = path.read_bytes()
    values = array("f")
    usable = len(raw) - (len(raw) % 4)
    if usable > 0:
        values.frombytes(raw[:usable])
    data = list(values)
    if not data:
        return {
            "output_path": str(path),
            "scalar_type": "float32",
            "count": 0,
            "bytes": len(raw),
            "min": 0.0,
            "max": 0.0,
            "mean": 0.0,
            "std": 0.0,
            "l2_norm": 0.0,
            "nonzero_count": 0,
            "first_values": [],
            "truncated": False,
        }
    count = len(data)
    total = float(sum(data))
    mean = total / float(count)
    sq_sum = float(sum(v * v for v in data))
    variance = max(0.0, sq_sum / float(count) - mean * mean)
    return {
        "output_path": str(path),
        "scalar_type": "float32",
        "count": count,
        "bytes": usable,
        "min": float(min(data)),
        "max": float(max(data)),
        "mean": mean,
        "std": math.sqrt(variance),
        "l2_norm": math.sqrt(sq_sum),
        "nonzero_count": sum(1 for v in data if v != 0.0),
        "first_values": [float(v) for v in data[:max_values]],
        "truncated": bool(count > max_values),
    }


def _materialize_output_csv_from_bin(bin_path: Path) -> Path:
    raw = bin_path.read_bytes()
    values = array("f")
    usable = len(raw) - (len(raw) % 4)
    if usable > 0:
        values.frombytes(raw[:usable])
    csv_path = bin_path.with_suffix(".csv")
    with csv_path.open("w", encoding="utf-8", newline="") as handle:
        writer = csv.writer(handle)
        writer.writerow(["index", "value"])
        for idx, value in enumerate(values):
            writer.writerow([idx, f"{float(value):.9g}"])
    return csv_path


def _safe_float(value: Any) -> float | None:
    try:
        if value is None:
            return None
        out = float(value)
        if math.isfinite(out):
            return out
    except Exception:
        return None
    return None


def _fmt_value(value: float | None, suffix: str, digits: int = 1) -> str:
    if value is None or not math.isfinite(value):
        return "n/a"
    return f"{value:.{digits}f}{suffix}"


def _norm_tokens(text: str) -> Tuple[str, ...]:
    clean = re.sub(r"[^a-z0-9]+", " ", str(text).lower())
    return tuple(part for part in clean.split() if part)


def _flatten_tree(obj: Any, prefix: Tuple[str, ...] = ()) -> List[Tuple[Tuple[str, ...], Any]]:
    out: List[Tuple[Tuple[str, ...], Any]] = []
    if isinstance(obj, dict):
        for key, value in obj.items():
            out.extend(_flatten_tree(value, prefix + _norm_tokens(str(key))))
        return out
    if isinstance(obj, list):
        for idx, value in enumerate(obj):
            out.extend(_flatten_tree(value, prefix + (str(idx),)))
        return out
    out.append((prefix, obj))
    return out


def _iter_dict_nodes(obj: Any) -> Iterable[Dict[str, Any]]:
    if isinstance(obj, dict):
        yield obj
        for value in obj.values():
            yield from _iter_dict_nodes(value)
    elif isinstance(obj, list):
        for value in obj:
            yield from _iter_dict_nodes(value)


def _best_numeric_match(
    flat_items: Iterable[Tuple[Tuple[str, ...], Any]],
    required_token_sets: Iterable[Tuple[str, ...]],
    *,
    forbidden_tokens: Iterable[str] = (),
) -> float | None:
    forb = set(forbidden_tokens)
    best_score = -1
    best_value: float | None = None
    for tokens, raw in flat_items:
        value = _safe_float(raw)
        if value is None or isinstance(raw, bool):
            continue
        token_set = set(tokens)
        if forb and token_set.intersection(forb):
            continue
        matched = False
        for required in required_token_sets:
            if all(tok in token_set for tok in required):
                matched = True
                score = len(required) * 10 - len(token_set)
                if score > best_score:
                    best_score = score
                    best_value = value
                break
        if matched:
            continue
    return best_value


def _coerce_watts(value: float | None) -> float | None:
    if value is None:
        return None
    if value > 1000.0:
        return value / 1000.0
    return value


@dataclass
class LiveMetricsSnapshot:
    timestamp_s: float
    source: str = ""
    note: str = ""
    process_cpu_pct: float | None = None
    process_mem_pct: float | None = None
    process_rss_mb: float | None = None
    gpu_util_pct: float | None = None
    gpu_mem_util_pct: float | None = None
    gpu_power_w: float | None = None
    cpu_power_w: float | None = None
    process_energy_impact: float | None = None
    process_gpu_time_ms: float | None = None


class LiveMetricsSampler:
    def __init__(self) -> None:
        self.system = platform.system().lower()
        self.powermetrics_path = shutil.which("powermetrics")
        self.nvidia_smi_path = shutil.which("nvidia-smi")
        self.rocm_smi_path = shutil.which("rocm-smi")

    def describe(self) -> str:
        parts = ["process:ps"]
        if self.system == "darwin" and self.powermetrics_path:
            parts.append("gpu/power:powermetrics")
        elif self.nvidia_smi_path:
            parts.append("gpu/power:nvidia-smi")
        elif self.rocm_smi_path:
            parts.append("gpu/power:rocm-smi")
        else:
            parts.append("gpu/power:unavailable")
        return " | ".join(parts)

    def sample(self, pid: int) -> LiveMetricsSnapshot:
        snap = LiveMetricsSnapshot(timestamp_s=time.monotonic(), source=self.describe())
        self._sample_process(pid, snap)
        if self.system == "darwin" and self.powermetrics_path:
            self._sample_powermetrics(pid, snap)
        elif self.nvidia_smi_path:
            self._sample_nvidia(snap)
        elif self.rocm_smi_path:
            self._sample_rocm(snap)
        return snap

    def _sample_process(self, pid: int, snap: LiveMetricsSnapshot) -> None:
        if os.name == "nt":
            snap.note = "Live process CPU telemetry is not implemented for Windows in this GUI build."
            return
        cmd = ["ps", "-p", str(pid), "-o", "pcpu=,pmem=,rss="]
        try:
            res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=2.0, check=False)
        except Exception as exc:
            snap.note = f"ps failed: {exc}"
            return
        if res.returncode != 0:
            snap.note = (res.stderr or "ps failed").strip()
            return
        parts = res.stdout.strip().split()
        if len(parts) >= 3:
            snap.process_cpu_pct = _safe_float(parts[0])
            snap.process_mem_pct = _safe_float(parts[1])
            rss_kb = _safe_float(parts[2])
            if rss_kb is not None:
                snap.process_rss_mb = rss_kb / 1024.0

    def _sample_powermetrics(self, pid: int, snap: LiveMetricsSnapshot) -> None:
        if not _is_root():
            snap.note = "Run GUI with sudo to enable GPU usage and power telemetry on macOS."
            return
        cmd = [
            self.powermetrics_path or "powermetrics",
            "--samplers",
            "cpu_power,gpu_power",
            "--show-process-energy",
            "--show-process-gpu",
            "--handle-invalid-values",
            "--format",
            "plist",
            "-i",
            "1000",
            "-n",
            "1",
        ]
        try:
            res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, timeout=4.5, check=False)
        except Exception as exc:
            snap.note = f"powermetrics failed: {exc}"
            return
        if res.returncode != 0:
            err = res.stderr.decode("utf-8", "replace").strip()
            snap.note = err or "powermetrics returned an error"
            return
        chunks = [chunk for chunk in res.stdout.split(b"\x00") if chunk.strip()]
        if not chunks:
            snap.note = "powermetrics returned no samples"
            return
        try:
            payload = plistlib.loads(chunks[-1])
        except Exception as exc:
            snap.note = f"powermetrics plist parse failed: {exc}"
            return

        flat = _flatten_tree(payload)
        snap.cpu_power_w = _coerce_watts(
            _best_numeric_match(
                flat,
                [
                    ("cpu", "power"),
                    ("processor", "power"),
                    ("package", "power"),
                ],
                forbidden_tokens=("gpu", "ane"),
            )
        )
        snap.gpu_power_w = _coerce_watts(
            _best_numeric_match(
                flat,
                [
                    ("gpu", "power"),
                    ("graphics", "power"),
                ],
            )
        )
        snap.gpu_util_pct = _best_numeric_match(
            flat,
            [
                ("gpu", "active", "residency"),
                ("gpu", "hw", "active", "residency"),
                ("gpu", "active"),
                ("gpu", "utilization"),
            ],
            forbidden_tokens=("process",),
        )

        proc_node = self._find_process_node(payload, pid)
        if proc_node is None:
            return
        proc_flat = _flatten_tree(proc_node)
        snap.process_energy_impact = _best_numeric_match(
            proc_flat,
            [
                ("energy", "impact"),
                ("energy",),
            ],
        )
        snap.process_gpu_time_ms = _best_numeric_match(
            proc_flat,
            [
                ("gpu", "time"),
                ("gpu", "ms"),
                ("graphics", "time"),
            ],
        )

    def _find_process_node(self, payload: Any, pid: int) -> Dict[str, Any] | None:
        for node in _iter_dict_nodes(payload):
            for key, value in node.items():
                token_set = set(_norm_tokens(str(key)))
                if "pid" not in token_set and "process" not in token_set:
                    continue
                pid_val = _safe_float(value)
                if pid_val is not None and int(pid_val) == int(pid):
                    return node
        return None

    def _sample_nvidia(self, snap: LiveMetricsSnapshot) -> None:
        cmd = [
            self.nvidia_smi_path or "nvidia-smi",
            "--query-gpu=utilization.gpu,utilization.memory,power.draw",
            "--format=csv,noheader,nounits",
        ]
        try:
            res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=2.5, check=False)
        except Exception as exc:
            snap.note = f"nvidia-smi failed: {exc}"
            return
        if res.returncode != 0:
            snap.note = (res.stderr or "nvidia-smi failed").strip()
            return
        rows = [line.strip() for line in res.stdout.splitlines() if line.strip()]
        if not rows:
            return
        gpu_vals: List[float] = []
        mem_vals: List[float] = []
        power_vals: List[float] = []
        for row in rows:
            parts = [part.strip() for part in row.split(",")]
            if len(parts) < 3:
                continue
            g = _safe_float(parts[0])
            m = _safe_float(parts[1])
            p = _safe_float(parts[2])
            if g is not None:
                gpu_vals.append(g)
            if m is not None:
                mem_vals.append(m)
            if p is not None:
                power_vals.append(p)
        if gpu_vals:
            snap.gpu_util_pct = max(gpu_vals)
        if mem_vals:
            snap.gpu_mem_util_pct = max(mem_vals)
        if power_vals:
            snap.gpu_power_w = max(power_vals)

    def _sample_rocm(self, snap: LiveMetricsSnapshot) -> None:
        cmd = [self.rocm_smi_path or "rocm-smi", "--showuse", "--showpower", "--json"]
        try:
            res = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, timeout=3.0, check=False)
        except Exception as exc:
            snap.note = f"rocm-smi failed: {exc}"
            return
        if res.returncode != 0:
            snap.note = (res.stderr or "rocm-smi failed").strip()
            return
        try:
            payload = json.loads(res.stdout)
        except Exception as exc:
            snap.note = f"rocm-smi json parse failed: {exc}"
            return
        flat = _flatten_tree(payload)
        snap.gpu_util_pct = _best_numeric_match(flat, [("gpu", "use"), ("gpu", "utilization"), ("use",)])
        snap.gpu_power_w = _coerce_watts(_best_numeric_match(flat, [("power",), ("average", "power")]))


class AutotuneGui(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title("Benchmark and Autotuning GUI")
        self.geometry("1400x900")

        self.proc: subprocess.Popen[str] | None = None
        self.log_queue: queue.Queue[Any] = queue.Queue()
        self.output_lines: List[str] = []
        self.log_char_count = 0
        self.current_script: str = ""
        self.last_result_dir: Path | None = None
        self.last_compare_dir: Path | None = None
        self.latest_optimization_data: Dict[str, Any] = {}
        self.latest_matrix_data: Dict[str, Any] = {}
        self.workflow_device_items: List[Dict[str, Any]] = []
        self.run_start_ts = 0.0
        self.last_output_ts = 0.0
        self.progress_mode = "idle"
        self.progress_total = 0
        self.progress_current = 0
        self.progress_out_dir: Path | None = None
        self.progress_known_dirs: set[str] = set()
        self.progress_eval_path: Path | None = None
        self.progress_eval_offset = 0
        self.progress_eval_lines = 0
        self.progress_tick_token = 0
        self.monitor_sampler = LiveMetricsSampler()
        self.monitor_thread: threading.Thread | None = None
        self.monitor_stop_event = threading.Event()
        self.monitor_pid: int | None = None
        self.monitor_last_ts: float | None = None
        self.monitor_est_energy_j = 0.0
        self.monitor_hist_t: Deque[float] = deque(maxlen=180)
        self.monitor_hist_cpu: Deque[float] = deque(maxlen=180)
        self.monitor_hist_mem_pct: Deque[float] = deque(maxlen=180)
        self.monitor_hist_rss_mb: Deque[float] = deque(maxlen=180)
        self.monitor_hist_gpu: Deque[float] = deque(maxlen=180)
        self.monitor_hist_gpu_mem: Deque[float] = deque(maxlen=180)
        self.monitor_hist_cpu_w: Deque[float] = deque(maxlen=180)
        self.monitor_hist_gpu_w: Deque[float] = deque(maxlen=180)

        self._build_ui()
        self._reset_monitor_state()
        self._set_root_status()

    def _build_ui(self) -> None:
        top = ttk.Frame(self)
        top.pack(fill="x", padx=8, pady=6)

        self.status_var = tk.StringVar(value="")
        self.root_var = tk.StringVar(value="")
        ttk.Label(top, textvariable=self.status_var).pack(side="left", padx=(0, 16))
        ttk.Label(top, textvariable=self.root_var).pack(side="left")

        nb = ttk.Notebook(self)
        nb.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        self.tab_run = ttk.Frame(nb)
        self.tab_results = ttk.Frame(nb)
        self.tab_plots = ttk.Frame(nb)
        self.tab_workflows = ttk.Frame(nb)
        nb.add(self.tab_workflows, text="Workflows")
        nb.add(self.tab_run, text="Launcher")
        nb.add(self.tab_results, text="Results")
        nb.add(self.tab_plots, text="Plots")

        self._build_workflows_tab()
        self._build_run_tab()
        self._build_results_tab()
        self._build_plots_tab()

    def _build_workflows_tab(self) -> None:
        top = ttk.Frame(self.tab_workflows)
        top.pack(fill="x", padx=8, pady=8)

        ttk.Label(top, text="Workflow").grid(row=0, column=0, sticky="w")
        self.workflow_var = tk.StringVar(value=list(WORKFLOWS.keys())[0])
        workflow_combo = ttk.Combobox(
            top,
            textvariable=self.workflow_var,
            values=list(WORKFLOWS.keys()),
            state="readonly",
            width=34,
        )
        workflow_combo.grid(row=0, column=1, sticky="we", padx=6)
        workflow_combo.bind("<<ComboboxSelected>>", lambda _e: self._refresh_workflow_description())

        ttk.Label(top, text="Profile").grid(row=0, column=2, sticky="w")
        self.workflow_profile_var = tk.StringVar(value="paper")
        ttk.Combobox(
            top,
            textvariable=self.workflow_profile_var,
            values=["quick", "paper", "full"],
            state="readonly",
            width=10,
        ).grid(row=0, column=3, sticky="w", padx=6)

        ttk.Label(top, text="Platform").grid(row=0, column=4, sticky="w")
        self.workflow_platform_var = tk.StringVar(value="auto")
        platform_combo = ttk.Combobox(
            top,
            textvariable=self.workflow_platform_var,
            values=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"],
            state="readonly",
            width=12,
        )
        platform_combo.grid(row=0, column=5, sticky="w", padx=6)

        ttk.Label(top, text="Backend").grid(row=1, column=0, sticky="w", pady=(8, 0))
        self.workflow_backend_var = tk.StringVar(value="auto")
        backend_combo = ttk.Combobox(
            top,
            textvariable=self.workflow_backend_var,
            values=["auto", "cpu", "metal", "cuda", "hip", "opencl", "amd", "intel"],
            state="readonly",
            width=12,
        )
        backend_combo.grid(row=1, column=1, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Device").grid(row=1, column=2, sticky="w", pady=(8, 0))
        self.workflow_device_var = tk.StringVar(value="0")
        ttk.Entry(top, textvariable=self.workflow_device_var, width=8).grid(row=1, column=3, sticky="w", padx=6, pady=(8, 0))

        ttk.Button(top, text="Refresh Devices", command=self._refresh_workflow_devices).grid(
            row=1, column=4, sticky="w", padx=6, pady=(8, 0)
        )

        ttk.Label(top, text="Detected device").grid(row=2, column=0, sticky="w", pady=(8, 0))
        self.workflow_device_choice_var = tk.StringVar(value="")
        self.workflow_device_combo = ttk.Combobox(
            top,
            textvariable=self.workflow_device_choice_var,
            values=[],
            state="readonly",
            width=88,
        )
        self.workflow_device_combo.grid(row=2, column=1, columnspan=5, sticky="we", padx=6, pady=(8, 0))
        self.workflow_device_combo.bind("<<ComboboxSelected>>", lambda _e: self._apply_workflow_device_selection(pin_backend=True))

        self.workflow_device_note_var = tk.StringVar(value="Click 'Refresh Devices' to detect CUDA/OpenCL/CPU devices.")
        ttk.Label(
            top,
            textvariable=self.workflow_device_note_var,
            wraplength=1180,
            justify="left",
        ).grid(row=3, column=0, columnspan=6, sticky="we", padx=0, pady=(6, 0))

        ttk.Label(top, text="Roofline AI").grid(row=4, column=0, sticky="w", pady=(8, 0))
        self.workflow_ai_var = tk.StringVar(value="8")
        ttk.Entry(top, textvariable=self.workflow_ai_var, width=10).grid(row=4, column=1, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Roofline Bytes").grid(row=4, column=2, sticky="w", pady=(8, 0))
        self.workflow_bytes_var = tk.StringVar(value="1000000000")
        ttk.Entry(top, textvariable=self.workflow_bytes_var, width=14).grid(row=4, column=3, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Real runs").grid(row=4, column=4, sticky="w", pady=(8, 0))
        self.workflow_real_runs_var = tk.StringVar(value="3")
        ttk.Entry(top, textvariable=self.workflow_real_runs_var, width=8).grid(row=4, column=5, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Repeats").grid(row=5, column=0, sticky="w", pady=(8, 0))
        self.workflow_repeats_var = tk.StringVar(value="3")
        ttk.Entry(top, textvariable=self.workflow_repeats_var, width=8).grid(row=5, column=1, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Trials").grid(row=5, column=2, sticky="w", pady=(8, 0))
        self.workflow_trials_var = tk.StringVar(value="96")
        ttk.Entry(top, textvariable=self.workflow_trials_var, width=8).grid(row=5, column=3, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Population").grid(row=5, column=4, sticky="w", pady=(8, 0))
        self.workflow_population_var = tk.StringVar(value="12")
        ttk.Entry(top, textvariable=self.workflow_population_var, width=8).grid(row=5, column=5, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Iterations").grid(row=6, column=0, sticky="w", pady=(8, 0))
        self.workflow_iterations_var = tk.StringVar(value="20")
        ttk.Entry(top, textvariable=self.workflow_iterations_var, width=8).grid(row=6, column=1, sticky="w", padx=6, pady=(8, 0))

        ttk.Label(top, text="Filip case").grid(row=6, column=2, sticky="w", pady=(8, 0))
        self.workflow_filip_case_var = tk.StringVar(value="prism_pair")
        ttk.Combobox(
            top,
            textvariable=self.workflow_filip_case_var,
            values=FILIP_CASE_CHOICES,
            state="readonly",
            width=16,
        ).grid(row=6, column=3, sticky="w", padx=6, pady=(8, 0))

        self.workflow_filip_case_note_var = tk.StringVar(value=FILIP_CASE_DESCRIPTIONS["prism_pair"])
        ttk.Label(
            top,
            textvariable=self.workflow_filip_case_note_var,
            wraplength=620,
            justify="left",
        ).grid(row=6, column=4, columnspan=2, sticky="we", padx=6, pady=(8, 0))

        ttk.Label(top, text="Filip mode").grid(row=7, column=0, sticky="w", pady=(8, 0))
        self.workflow_filip_mode_var = tk.StringVar(value="portable_sweep")
        ttk.Combobox(
            top,
            textvariable=self.workflow_filip_mode_var,
            values=FILIP_MODE_CHOICES,
            state="readonly",
            width=16,
        ).grid(row=7, column=1, sticky="w", padx=6, pady=(8, 0))

        self.workflow_filip_mode_note_var = tk.StringVar(value=FILIP_MODE_DESCRIPTIONS["portable_sweep"])
        ttk.Label(
            top,
            textvariable=self.workflow_filip_mode_note_var,
            wraplength=860,
            justify="left",
        ).grid(row=7, column=2, columnspan=4, sticky="we", padx=6, pady=(8, 0))

        ttk.Label(top, text="Filip mod_2022").grid(row=8, column=0, sticky="w", pady=(8, 0))
        self.workflow_filip_modfem_var = tk.StringVar(value="")
        ttk.Entry(top, textvariable=self.workflow_filip_modfem_var).grid(
            row=8, column=1, columnspan=4, sticky="we", padx=6, pady=(8, 0)
        )
        ttk.Button(top, text="Browse", command=self._browse_workflow_filip_modfem).grid(
            row=8, column=5, sticky="w", padx=6, pady=(8, 0)
        )

        ttk.Label(top, text="Input override").grid(row=9, column=0, sticky="w", pady=(8, 0))
        self.workflow_filip_input_override_var = tk.StringVar(value="")
        ttk.Entry(top, textvariable=self.workflow_filip_input_override_var).grid(
            row=9, column=1, columnspan=4, sticky="we", padx=6, pady=(8, 0)
        )
        ttk.Button(top, text="Browse", command=self._browse_workflow_filip_input_override).grid(
            row=9, column=5, sticky="w", padx=6, pady=(8, 0)
        )

        self.workflow_filip_dump_launch_var = tk.BooleanVar(value=False)
        ttk.Checkbutton(
            top,
            text="Dump OpenCL launch artifacts",
            variable=self.workflow_filip_dump_launch_var,
        ).grid(row=10, column=0, columnspan=2, sticky="w", pady=(8, 0))

        ttk.Label(top, text="Replay dump root").grid(row=10, column=2, sticky="w", pady=(8, 0))
        self.workflow_filip_replay_dump_root_var = tk.StringVar(value="")
        ttk.Entry(top, textvariable=self.workflow_filip_replay_dump_root_var).grid(
            row=10, column=3, columnspan=2, sticky="we", padx=6, pady=(8, 0)
        )
        ttk.Button(top, text="Browse", command=self._browse_workflow_filip_replay_dump_root).grid(
            row=10, column=5, sticky="w", padx=6, pady=(8, 0)
        )

        top.columnconfigure(1, weight=1)
        top.columnconfigure(5, weight=1)

        actions = ttk.Frame(self.tab_workflows)
        actions.pack(fill="x", padx=8, pady=(0, 8))
        ttk.Button(actions, text="Run Selected Workflow", command=self._run_selected_workflow).pack(side="left")
        ttk.Button(actions, text="Send To Launcher", command=self._mirror_workflow_to_launcher).pack(side="left", padx=6)

        self.workflow_desc_var = tk.StringVar(value="")
        ttk.Label(self.tab_workflows, textvariable=self.workflow_desc_var, wraplength=1200, justify="left").pack(
            anchor="w", padx=8, pady=(0, 8)
        )

        ttk.Label(self.tab_workflows, text="Workflow-specific extra args").pack(anchor="w", padx=8)
        self.workflow_extra_text = ScrolledText(self.tab_workflows, height=4, wrap="word")
        self.workflow_extra_text.pack(fill="x", padx=8, pady=(0, 8))

        self.workflow_preview_var = tk.StringVar(value="")
        ttk.Label(self.tab_workflows, textvariable=self.workflow_preview_var, wraplength=1200, justify="left").pack(
            anchor="w", padx=8, pady=(0, 8)
        )

        for var in (
            self.workflow_profile_var,
            self.workflow_device_var,
            self.workflow_ai_var,
            self.workflow_bytes_var,
            self.workflow_real_runs_var,
            self.workflow_repeats_var,
            self.workflow_trials_var,
            self.workflow_population_var,
            self.workflow_iterations_var,
            self.workflow_filip_case_var,
            self.workflow_filip_mode_var,
            self.workflow_filip_modfem_var,
            self.workflow_filip_input_override_var,
            self.workflow_filip_replay_dump_root_var,
        ):
            var.trace_add("write", lambda *_args: self._refresh_workflow_description())
        self.workflow_filip_dump_launch_var.trace_add("write", lambda *_args: self._refresh_workflow_description())

        self.workflow_var.trace_add("write", lambda *_args: self._on_workflow_device_context_changed())
        self.workflow_platform_var.trace_add("write", lambda *_args: self._on_workflow_device_context_changed())
        self.workflow_backend_var.trace_add("write", lambda *_args: self._on_workflow_device_context_changed())
        platform_combo.bind("<<ComboboxSelected>>", lambda _e: self._refresh_workflow_devices())
        backend_combo.bind("<<ComboboxSelected>>", lambda _e: self._refresh_workflow_devices())

        self._refresh_workflow_description()
        self._refresh_workflow_devices()

    def _build_run_tab(self) -> None:
        frm_top = ttk.Frame(self.tab_run)
        frm_top.pack(fill="x", padx=8, pady=8)

        ttk.Label(frm_top, text="Template").grid(row=0, column=0, sticky="w")
        self.template_var = tk.StringVar(value=list(TEMPLATES.keys())[0])
        self.template_combo = ttk.Combobox(
            frm_top,
            textvariable=self.template_var,
            values=list(TEMPLATES.keys()),
            state="readonly",
            width=42,
        )
        self.template_combo.grid(row=0, column=1, sticky="we", padx=6)
        ttk.Button(frm_top, text="Apply Template", command=self._apply_template).grid(row=0, column=2, padx=6)

        ttk.Label(frm_top, text="Script").grid(row=1, column=0, sticky="w", pady=(8, 0))
        self.script_var = tk.StringVar(value="run_firefly_optimization.py")
        self.script_combo = ttk.Combobox(
            frm_top,
            textvariable=self.script_var,
            values=SCRIPT_CHOICES,
            state="normal",
            width=42,
        )
        self.script_combo.grid(row=1, column=1, sticky="we", padx=6, pady=(8, 0))

        btns = ttk.Frame(frm_top)
        btns.grid(row=1, column=2, pady=(8, 0))
        ttk.Button(btns, text="Run", command=self._run_command).pack(side="left", padx=2)
        ttk.Button(btns, text="Stop", command=self._stop_command).pack(side="left", padx=2)
        ttk.Button(btns, text="Load Latest Optimization", command=self._load_latest_optimization).pack(side="left", padx=2)
        ttk.Button(btns, text="Load Latest Matrix", command=self._load_latest_matrix).pack(side="left", padx=2)
        ttk.Button(btns, text="Load Latest Session", command=self._load_latest_session).pack(side="left", padx=2)
        ttk.Button(btns, text="Load Latest Report", command=self._load_latest_report).pack(side="left", padx=2)
        ttk.Button(btns, text="Show Latest Plot", command=self._show_latest_plot).pack(side="left", padx=2)

        frm_top.columnconfigure(1, weight=1)

        ttk.Label(self.tab_run, text="Arguments").pack(anchor="w", padx=8)
        self.args_text = ScrolledText(self.tab_run, height=8, wrap="word")
        self.args_text.pack(fill="x", padx=8, pady=(0, 8))

        ttk.Label(self.tab_run, text="Live Output").pack(anchor="w", padx=8)
        self.log_text = ScrolledText(self.tab_run, height=24, wrap="none")
        self.log_text.pack(fill="both", expand=True, padx=8, pady=(0, 8))
        self.log_text.configure(state="disabled")

        progress_row = ttk.Frame(self.tab_run)
        progress_row.pack(fill="x", padx=8, pady=(0, 8))
        self.progress_var = tk.DoubleVar(value=0.0)
        self.progress_bar = ttk.Progressbar(
            progress_row,
            orient="horizontal",
            mode="determinate",
            variable=self.progress_var,
            maximum=100.0,
        )
        self.progress_bar.pack(side="left", fill="x", expand=True)
        self.progress_label_var = tk.StringVar(value="Idle")
        ttk.Label(progress_row, textvariable=self.progress_label_var, width=52, anchor="w").pack(side="left", padx=(8, 0))

        monitor_box = ttk.LabelFrame(self.tab_run, text="Live Utilization")
        monitor_box.pack(fill="x", padx=8, pady=(0, 8))

        monitor_info = ttk.Frame(monitor_box)
        monitor_info.pack(fill="x", expand=False, padx=8, pady=(6, 2))

        self.monitor_state_var = tk.StringVar(value="Idle")
        self.monitor_source_var = tk.StringVar(value=self.monitor_sampler.describe())
        self.monitor_proc_cpu_var = tk.StringVar(value="n/a")
        self.monitor_proc_mem_var = tk.StringVar(value="n/a")
        self.monitor_proc_rss_var = tk.StringVar(value="n/a")
        self.monitor_gpu_util_var = tk.StringVar(value="n/a")
        self.monitor_gpu_mem_var = tk.StringVar(value="n/a")
        self.monitor_gpu_power_var = tk.StringVar(value="n/a")
        self.monitor_cpu_power_var = tk.StringVar(value="n/a")
        self.monitor_energy_var = tk.StringVar(value="0.0 J")
        self.monitor_proc_energy_var = tk.StringVar(value="n/a")
        self.monitor_proc_gpu_time_var = tk.StringVar(value="n/a")
        self.monitor_note_var = tk.StringVar(value="Waiting for benchmark start.")

        rows = [
            ("State", self.monitor_state_var, "Source", self.monitor_source_var),
            ("Process CPU", self.monitor_proc_cpu_var, "Process MEM", self.monitor_proc_mem_var),
            ("Process RSS", self.monitor_proc_rss_var, "GPU usage", self.monitor_gpu_util_var),
            ("GPU mem", self.monitor_gpu_mem_var, "GPU power", self.monitor_gpu_power_var),
            ("CPU power", self.monitor_cpu_power_var, "Est. energy", self.monitor_energy_var),
            ("Proc energy", self.monitor_proc_energy_var, "Proc GPU time", self.monitor_proc_gpu_time_var),
        ]
        for row_idx, (label_a, var_a, label_b, var_b) in enumerate(rows):
            ttk.Label(monitor_info, text=label_a).grid(row=row_idx, column=0, sticky="w", padx=(0, 6), pady=1)
            ttk.Label(monitor_info, textvariable=var_a, width=18).grid(row=row_idx, column=1, sticky="w", padx=(0, 16), pady=1)
            ttk.Label(monitor_info, text=label_b).grid(row=row_idx, column=2, sticky="w", padx=(0, 6), pady=1)
            ttk.Label(monitor_info, textvariable=var_b, width=32).grid(row=row_idx, column=3, sticky="w", pady=1)
        ttk.Label(monitor_info, text="Note").grid(row=len(rows), column=0, sticky="nw", padx=(0, 6), pady=(4, 1))
        ttk.Label(monitor_info, textvariable=self.monitor_note_var, wraplength=620, justify="left").grid(
            row=len(rows),
            column=1,
            columnspan=3,
            sticky="we",
            pady=(4, 1),
        )
        monitor_info.columnconfigure(3, weight=1)

        if HAS_MPL:
            self.monitor_plot_host = ttk.Frame(monitor_box)
            self.monitor_plot_host.pack(fill="both", expand=True, padx=8, pady=(2, 8))
            self.monitor_figure = Figure(figsize=(9.0, 6.2), dpi=100)
            self.monitor_canvas = FigureCanvasTkAgg(self.monitor_figure, master=self.monitor_plot_host)
            self.monitor_canvas.get_tk_widget().pack(fill="both", expand=True)
            self._refresh_monitor_plot()
        else:
            self.monitor_plot_host = None
            self.monitor_figure = None
            self.monitor_canvas = None

        self._apply_template()

    def _build_results_tab(self) -> None:
        frm_top = ttk.Frame(self.tab_results)
        frm_top.pack(fill="x", padx=8, pady=8)

        ttk.Label(frm_top, text="Result Directory").pack(side="left")
        self.result_dir_var = tk.StringVar(value="")
        ttk.Entry(frm_top, textvariable=self.result_dir_var).pack(side="left", fill="x", expand=True, padx=6)
        ttk.Button(frm_top, text="Browse", command=self._browse_result_dir).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Load as Optimization", command=self._load_selected_optimization).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Load as Matrix", command=self._load_selected_matrix).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Show Best Exact Output CSV", command=self._show_best_exact_output).pack(side="left", padx=2)

        self.results_text = ScrolledText(self.tab_results, wrap="word")
        self.results_text.pack(fill="both", expand=True, padx=8, pady=(0, 8))

    def _build_plots_tab(self) -> None:
        frm_top = ttk.Frame(self.tab_plots)
        frm_top.pack(fill="x", padx=8, pady=8)

        ttk.Button(frm_top, text="Optimization Plot", command=self._show_current_optimization_plot).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Firefly Convergence", command=self._plot_firefly_convergence).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Firefly Scatter", command=self._plot_firefly_scatter).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Matrix Comparison", command=self._plot_matrix_comparison).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Legacy Compare", command=self._run_legacy_xlsx_compare).pack(side="left", padx=2)
        ttk.Button(frm_top, text="Save Figure", command=self._save_current_figure).pack(side="left", padx=12)

        self.plot_host = ttk.Frame(self.tab_plots)
        self.plot_host.pack(fill="both", expand=True, padx=8, pady=(0, 8))

        if HAS_MPL:
            assert Figure is not None
            assert FigureCanvasTkAgg is not None
            assert NavigationToolbar2Tk is not None
            self.figure = Figure(figsize=(12, 7), dpi=100)
            self.canvas = FigureCanvasTkAgg(self.figure, master=self.plot_host)
            self.canvas.get_tk_widget().pack(fill="both", expand=True)
            toolbar = NavigationToolbar2Tk(self.canvas, self.plot_host)
            toolbar.update()
            self.canvas.draw_idle()
        else:
            self.figure = None
            self.canvas = None
            msg = "matplotlib is unavailable in this environment."
            if MPL_IMPORT_ERROR:
                msg += f"\nReason: {MPL_IMPORT_ERROR}"
            ttk.Label(self.plot_host, text=msg, justify="left", wraplength=900).pack(anchor="center", pady=20)

    def _set_root_status(self) -> None:
        if _is_root():
            self.root_var.set("Root mode: YES (energy measurement should work if backend supports it)")
        else:
            self.root_var.set("Root mode: NO (run with sudo for reliable energy measurement)")
        status = f"Workspace: {ROOT}"
        if not HAS_MPL and MPL_IMPORT_ERROR:
            status += f" | matplotlib disabled: {MPL_IMPORT_ERROR}"
        self.status_var.set(status)

    def _reset_monitor_state(self) -> None:
        self.monitor_stop_event = threading.Event()
        self.monitor_pid = None
        self.monitor_last_ts = None
        self.monitor_est_energy_j = 0.0
        self.monitor_hist_t.clear()
        self.monitor_hist_cpu.clear()
        self.monitor_hist_mem_pct.clear()
        self.monitor_hist_rss_mb.clear()
        self.monitor_hist_gpu.clear()
        self.monitor_hist_gpu_mem.clear()
        self.monitor_hist_cpu_w.clear()
        self.monitor_hist_gpu_w.clear()
        self.monitor_state_var.set("Idle")
        self.monitor_source_var.set(self.monitor_sampler.describe())
        self.monitor_proc_cpu_var.set("n/a")
        self.monitor_proc_mem_var.set("n/a")
        self.monitor_proc_rss_var.set("n/a")
        self.monitor_gpu_util_var.set("n/a")
        self.monitor_gpu_mem_var.set("n/a")
        self.monitor_gpu_power_var.set("n/a")
        self.monitor_cpu_power_var.set("n/a")
        self.monitor_energy_var.set("0.0 J")
        self.monitor_proc_energy_var.set("n/a")
        self.monitor_proc_gpu_time_var.set("n/a")
        self.monitor_note_var.set("Waiting for benchmark start.")
        self._refresh_monitor_plot()

    def _start_live_monitor(self, pid: int) -> None:
        self._stop_live_monitor(join_timeout=0.2)
        self._reset_monitor_state()
        self.monitor_pid = int(pid)
        self.monitor_state_var.set(f"Running (pid={pid})")
        self.monitor_note_var.set("Sampling process CPU/memory and best-effort GPU/power telemetry.")
        stop_event = self.monitor_stop_event
        sampler = self.monitor_sampler

        def _worker() -> None:
            while not stop_event.is_set():
                sample_started = time.monotonic()
                snap = sampler.sample(pid)
                self.log_queue.put(("metrics", snap))
                wait_left = 1.0
                elapsed = time.monotonic() - sample_started
                if elapsed < wait_left:
                    if stop_event.wait(wait_left - elapsed):
                        break

        self.monitor_thread = threading.Thread(target=_worker, daemon=True)
        self.monitor_thread.start()

    def _stop_live_monitor(self, join_timeout: float = 0.8) -> None:
        try:
            self.monitor_stop_event.set()
        except Exception:
            pass
        thread = self.monitor_thread
        self.monitor_thread = None
        if thread is not None and thread.is_alive():
            try:
                thread.join(timeout=join_timeout)
            except Exception:
                pass

    def _apply_live_metrics(self, snap: LiveMetricsSnapshot) -> None:
        self.monitor_source_var.set(snap.source or self.monitor_sampler.describe())
        self.monitor_proc_cpu_var.set(_fmt_value(snap.process_cpu_pct, "%", 1))
        self.monitor_proc_mem_var.set(_fmt_value(snap.process_mem_pct, "%", 1))
        self.monitor_proc_rss_var.set(_fmt_value(snap.process_rss_mb, " MB", 1))
        self.monitor_gpu_util_var.set(_fmt_value(snap.gpu_util_pct, "%", 1))
        self.monitor_gpu_mem_var.set(_fmt_value(snap.gpu_mem_util_pct, "%", 1))
        self.monitor_gpu_power_var.set(_fmt_value(snap.gpu_power_w, " W", 2))
        self.monitor_cpu_power_var.set(_fmt_value(snap.cpu_power_w, " W", 2))
        self.monitor_proc_energy_var.set(_fmt_value(snap.process_energy_impact, "", 2))
        self.monitor_proc_gpu_time_var.set(_fmt_value(snap.process_gpu_time_ms, " ms", 2))
        if snap.note:
            self.monitor_note_var.set(snap.note)
        elif _is_root() or platform.system().lower() != "darwin":
            self.monitor_note_var.set("Live telemetry active.")
        else:
            self.monitor_note_var.set("CPU telemetry active. Start GUI with sudo to enable macOS GPU/power telemetry.")

        if self.monitor_last_ts is not None:
            dt = max(0.0, snap.timestamp_s - self.monitor_last_ts)
            total_power = 0.0
            have_power = False
            for val in (snap.cpu_power_w, snap.gpu_power_w):
                if val is not None and math.isfinite(val):
                    total_power += float(val)
                    have_power = True
            if have_power:
                self.monitor_est_energy_j += total_power * dt
        self.monitor_last_ts = snap.timestamp_s
        self.monitor_energy_var.set(f"{self.monitor_est_energy_j:.2f} J")

        rel_t = max(0.0, snap.timestamp_s - self.run_start_ts) if self.run_start_ts > 0.0 else 0.0
        self.monitor_hist_t.append(rel_t)
        self.monitor_hist_cpu.append(snap.process_cpu_pct if snap.process_cpu_pct is not None else float("nan"))
        self.monitor_hist_mem_pct.append(snap.process_mem_pct if snap.process_mem_pct is not None else float("nan"))
        self.monitor_hist_rss_mb.append(snap.process_rss_mb if snap.process_rss_mb is not None else float("nan"))
        self.monitor_hist_gpu.append(snap.gpu_util_pct if snap.gpu_util_pct is not None else float("nan"))
        self.monitor_hist_gpu_mem.append(snap.gpu_mem_util_pct if snap.gpu_mem_util_pct is not None else float("nan"))
        self.monitor_hist_cpu_w.append(snap.cpu_power_w if snap.cpu_power_w is not None else float("nan"))
        self.monitor_hist_gpu_w.append(snap.gpu_power_w if snap.gpu_power_w is not None else float("nan"))
        self._refresh_monitor_plot()

    def _refresh_monitor_plot(self) -> None:
        if not HAS_MPL or getattr(self, "monitor_figure", None) is None or getattr(self, "monitor_canvas", None) is None:
            return
        assert self.monitor_figure is not None
        assert self.monitor_canvas is not None

        self.monitor_figure.clear()
        axes = self.monitor_figure.subplots(4, 1, sharex=True)
        xs = list(self.monitor_hist_t)

        if xs:
            ax_cpu, ax_mem, ax_gpu, ax_power = axes

            ax_cpu.plot(xs, list(self.monitor_hist_cpu), color="#1d4ed8", linewidth=1.4)
            ax_cpu.set_ylabel("CPU %")
            ax_cpu.set_title("Process CPU usage")

            ax_mem.plot(xs, list(self.monitor_hist_rss_mb), color="#7c3aed", linewidth=1.4, label="RSS MB")
            ax_mem_r = ax_mem.twinx()
            ax_mem_r.plot(xs, list(self.monitor_hist_mem_pct), color="#0f766e", linewidth=1.1, alpha=0.85, label="MEM %")
            ax_mem.set_ylabel("RSS MB")
            ax_mem_r.set_ylabel("MEM %")
            ax_mem.set_title("Process memory")

            ax_gpu.plot(xs, list(self.monitor_hist_gpu), color="#b45309", linewidth=1.4, label="GPU %")
            ax_gpu.plot(xs, list(self.monitor_hist_gpu_mem), color="#be185d", linewidth=1.1, alpha=0.85, label="GPU MEM %")
            ax_gpu.set_ylabel("GPU %")
            ax_gpu.set_title("GPU usage")
            ax_gpu.legend(loc="upper right", fontsize=7, ncol=2)

            ax_power.plot(xs, list(self.monitor_hist_cpu_w), color="#334155", linewidth=1.4, label="CPU W")
            ax_power.plot(xs, list(self.monitor_hist_gpu_w), color="#dc2626", linewidth=1.4, label="GPU W")
            ax_power.set_ylabel("W")
            ax_power.set_xlabel("Elapsed time [s]")
            ax_power.set_title(f"Power and estimated energy ({self.monitor_est_energy_j:.2f} J)")
            ax_power.legend(loc="upper right", fontsize=7, ncol=2)

            ax_cpu.set_ylim(bottom=0.0)
            ax_gpu.set_ylim(bottom=0.0)
            ax_power.set_ylim(bottom=0.0)
            ax_mem.set_ylim(bottom=0.0)
            ax_mem_r.set_ylim(bottom=0.0)
        else:
            for ax, title, ylabel in zip(
                axes,
                [
                    "Process CPU usage",
                    "Process memory",
                    "GPU usage",
                    "Power and estimated energy",
                ],
                ["CPU %", "RSS MB", "GPU %", "W"],
            ):
                ax.text(0.5, 0.5, "No live samples yet", transform=ax.transAxes, ha="center", va="center")
                ax.set_title(title)
                ax.set_ylabel(ylabel)
            axes[-1].set_xlabel("Elapsed time [s]")

        for ax in axes:
            ax.grid(True, alpha=0.25)
        self.monitor_figure.tight_layout()
        self.monitor_canvas.draw_idle()

    def _append_log(self, text: str) -> None:
        max_chars = 2_000_000
        trim_chars = 400_000
        self.log_text.configure(state="normal")
        self.log_text.insert("end", text)
        self.log_text.see("end")
        self.log_char_count += len(text)
        if self.log_char_count > max_chars:
            self.log_text.delete("1.0", f"1.0+{trim_chars}c")
            self.log_char_count = max_chars - trim_chars
        self.log_text.configure(state="disabled")

    @staticmethod
    def _case_name_from_exact_key(case_key: str) -> str:
        key = str(case_key).strip().lower()
        if key == "prism6__laplace":
            return "laplace_prism"
        if key == "prism6__test":
            return "test_prism"
        return key or "unknown_case"

    @staticmethod
    def _best_exact_output_preview_path(summary: Dict[str, Any]) -> Path | None:
        numerical = summary.get("numerical_outputs") or {}
        preview = str(numerical.get("best_output_preview", "")).strip()
        if preview:
            path = Path(preview)
            if path.exists():
                return path
        preview = str(numerical.get("example_output_preview", "")).strip()
        if preview:
            path = Path(preview)
            if path.exists():
                return path
        return None

    @classmethod
    def _best_exact_output_csv_path(cls, summary: Dict[str, Any], base_dir: Path | None = None) -> Path | None:
        numerical = summary.get("numerical_outputs") or {}
        csv_path = str(numerical.get("best_output_csv", "")).strip()
        if csv_path:
            path = Path(csv_path)
            if path.exists():
                return path
        csv_path = str(numerical.get("example_output_csv", "")).strip()
        if csv_path:
            path = Path(csv_path)
            if path.exists():
                return path

        bin_path = cls._best_exact_output_bin_path(summary, base_dir=base_dir)
        if bin_path is not None:
            candidate = bin_path.with_suffix(".csv")
            if candidate.exists():
                return candidate

        best = summary.get("best_overall") or {}
        best_case = cls._case_name_from_exact_key(str(best.get("case_key", "")))
        best_variant = str(best.get("variant", "")).strip()
        best_option_index = int(best.get("option_index", -1))
        roots: List[Path] = []
        for raw in [
            str(numerical.get("root", "")).strip(),
            str(summary.get("launch_dumps_root", "")).strip(),
            str(summary.get("out_dir", "")).strip(),
            str(base_dir) if base_dir is not None else "",
        ]:
            if raw:
                path = Path(raw)
                if path.exists() and path not in roots:
                    roots.append(path)

        if best_variant and best_option_index >= 0:
            for root in roots:
                candidate = root / best_case / best_variant / f"opt_{best_option_index:03d}" / "el_data_out.csv"
                if candidate.exists():
                    return candidate

        for root in roots:
            for candidate in root.rglob("el_data_out.csv"):
                if candidate.is_file():
                    return candidate
        return None

    @classmethod
    def _best_exact_output_bin_path(cls, summary: Dict[str, Any], base_dir: Path | None = None) -> Path | None:
        numerical = summary.get("numerical_outputs") or {}
        output_path = str(numerical.get("best_output_path", "")).strip()
        if output_path:
            path = Path(output_path)
            if path.exists():
                return path

        best = summary.get("best_overall") or {}
        best_case = cls._case_name_from_exact_key(str(best.get("case_key", "")))
        best_variant = str(best.get("variant", "")).strip()
        best_option_index = int(best.get("option_index", -1))
        roots: List[Path] = []
        for raw in [
            str(numerical.get("root", "")).strip(),
            str(summary.get("launch_dumps_root", "")).strip(),
            str(summary.get("out_dir", "")).strip(),
            str(base_dir) if base_dir is not None else "",
        ]:
            if raw:
                path = Path(raw)
                if path.exists() and path not in roots:
                    roots.append(path)

        if best_variant and best_option_index >= 0:
            for root in roots:
                candidate = root / best_case / best_variant / f"opt_{best_option_index:03d}" / "el_data_out.bin"
                if candidate.exists():
                    return candidate

        for root in roots:
            for candidate in root.rglob("el_data_out.bin"):
                if candidate.is_file():
                    return candidate
        return None

    def _load_best_exact_output_preview(self, summary: Dict[str, Any]) -> Dict[str, Any] | None:
        preview_path = self._best_exact_output_preview_path(summary)
        if preview_path is not None:
            preview = _read_json(preview_path)
            preview["_preview_source"] = str(preview_path)
            return preview
        base_dir = self.last_result_dir if isinstance(self.last_result_dir, Path) else None
        bin_path = self._best_exact_output_bin_path(summary, base_dir=base_dir)
        if bin_path is None:
            return None
        preview = _preview_float32_binary(bin_path)
        preview["_preview_source"] = str(bin_path)
        return preview

    def _load_best_exact_output_csv_text(self, summary: Dict[str, Any], max_rows: int = 128) -> str | None:
        base_dir = self.last_result_dir if isinstance(self.last_result_dir, Path) else None
        csv_path = self._best_exact_output_csv_path(summary, base_dir=base_dir)
        if csv_path is None:
            bin_path = self._best_exact_output_bin_path(summary, base_dir=base_dir)
            if bin_path is None:
                return None
            csv_path = _materialize_output_csv_from_bin(bin_path)
        lines = [
            "Best exact output CSV",
            "",
            f"source: {csv_path}",
            "",
        ]
        with csv_path.open("r", encoding="utf-8", newline="") as handle:
            reader = csv.reader(handle)
            rows = list(reader)
        if not rows:
            lines.append("(empty csv)")
            return "\n".join(lines)

        header = rows[0]
        data_rows = rows[1:]
        lines.append(",".join(header))
        for row in data_rows[:max_rows]:
            lines.append(",".join(row))
        remaining = len(data_rows) - min(len(data_rows), max_rows)
        if remaining > 0:
            lines.append("")
            lines.append(f"... truncated {remaining} more rows")
        return "\n".join(lines)

    def _format_exact_results_payload(self, payload: Dict[str, Any], summary: Dict[str, Any]) -> str:
        lines: List[str] = []
        lines.append("Filip Exact Results")
        lines.append("")
        lines.append(f"Backend: {summary.get('backend', '')}")
        lines.append(f"Execution mode: {summary.get('execution_mode', '')}")
        lines.append(f"Benchmark case: {summary.get('benchmark_case', '')}")
        lines.append(f"Device: {summary.get('device', '')}")
        lines.append(f"Out dir: {summary.get('out_dir', '')}")
        lines.append(f"Combined CSV: {summary.get('combined_csv', '')}")

        numerical = summary.get("numerical_outputs") or {}
        lines.append("")
        lines.append("Numerical outputs")
        lines.append(f"Available: {'yes' if numerical.get('available') else 'no'}")
        if numerical.get("root"):
            lines.append(f"Root: {numerical.get('root')}")
        if numerical.get("records_with_output") is not None:
            lines.append(f"Saved records: {numerical.get('records_with_output')}")
        note = str(numerical.get("note", "")).strip()
        if note:
            lines.append(f"Note: {note}")
        if summary.get("launch_dumps_root"):
            lines.append(f"OpenCL launch dumps: {summary.get('launch_dumps_root')}")
        if summary.get("replay_dump_root"):
            lines.append(f"Replay dump root: {summary.get('replay_dump_root')}")
        if summary.get("translated_sources_root"):
            lines.append(f"Translated Metal kernels: {summary.get('translated_sources_root')}")

        best = summary.get("best_overall") or {}
        if best:
            lines.append("")
            lines.append("Best overall")
            lines.append(f"Case key: {best.get('case_key', '')}")
            lines.append(f"Variant: {best.get('variant', '')}")
            lines.append(f"Option index: {best.get('option_index', '')}")
            lines.append(f"Combo bits: {best.get('combo_bits', '')}")
            lines.append(f"Internal ns/elem: {best.get('score_internal_ns_per_elem', '')}")
            lines.append(f"Kernel ns/unit: {best.get('score_kernel_ns_per_unit', '')}")
            if numerical.get("best_output_path"):
                lines.append(f"Best output bin: {numerical.get('best_output_path')}")
            if numerical.get("best_output_csv"):
                lines.append(f"Best output csv: {numerical.get('best_output_csv')}")
            if numerical.get("best_output_preview"):
                lines.append(f"Best output preview: {numerical.get('best_output_preview')}")

        validation = summary.get("validation_summary") or {}
        if validation:
            lines.append("")
            lines.append("Validation")
            for key in [
                "records_with_validation",
                "records_checked",
                "records_with_expected_output",
                "records_within_tolerance",
                "records_out_of_tolerance",
                "worst_max_abs_diff",
                "worst_rms_diff",
            ]:
                if key in validation:
                    lines.append(f"{key}: {validation.get(key)}")

        preview = None
        try:
            preview = self._load_best_exact_output_preview(summary)
        except Exception as exc:
            lines.append("")
            lines.append(f"Best output preview failed to load: {exc}")
        if isinstance(preview, dict):
            lines.append("")
            lines.append("Best output preview")
            preview_source = str(preview.get("_preview_source", "")).strip()
            if preview_source:
                lines.append(f"source: {preview_source}")
            for key in [
                "count",
                "bytes",
                "min",
                "max",
                "mean",
                "std",
                "l2_norm",
                "nonzero_count",
            ]:
                if key in preview:
                    lines.append(f"{key}: {preview.get(key)}")
            first_values = preview.get("first_values")
            if isinstance(first_values, list):
                lines.append(f"first_values: {first_values}")
            if "truncated" in preview:
                lines.append(f"truncated: {bool(preview.get('truncated'))}")
            preview_validation = preview.get("validation")
            if isinstance(preview_validation, dict) and preview_validation:
                lines.append(f"preview_validation: {json.dumps(preview_validation, ensure_ascii=True)}")
        elif self._best_exact_output_bin_path(summary, base_dir=self.last_result_dir) is not None:
            try:
                preview = self._load_best_exact_output_preview(summary)
            except Exception as exc:
                lines.append("")
                lines.append(f"Best output preview failed to load: {exc}")

        try:
            csv_text = self._load_best_exact_output_csv_text(summary, max_rows=16)
        except Exception as exc:
            lines.append("")
            lines.append(f"Best output csv failed to load: {exc}")
        else:
            if csv_text:
                lines.append("")
                lines.append(csv_text)

        if isinstance(payload, dict) and "evaluations" in payload:
            lines.append("")
            lines.append(f"Loaded evaluations: {len(payload.get('evaluations') or [])}")
        return "\n".join(lines)

    def _format_results_payload(self, payload: Any) -> str:
        if isinstance(payload, str):
            return payload
        if isinstance(payload, dict):
            summary = payload.get("summary") if isinstance(payload.get("summary"), dict) else payload
            if (
                isinstance(summary, dict)
                and str(summary.get("workflow", "")).strip() == "filip_original"
                and str(summary.get("filip_mode", "")).strip() == "exact_reference"
            ):
                return self._format_exact_results_payload(payload if isinstance(payload, dict) else {}, summary)
            return json.dumps(payload, indent=2, ensure_ascii=True)
        return json.dumps(payload, indent=2, ensure_ascii=True)

    def _set_results_text(self, payload: Any) -> None:
        self.results_text.delete("1.0", "end")
        self.results_text.insert("end", self._format_results_payload(payload))

    def _show_best_exact_output(self) -> None:
        summary: Dict[str, Any] = {}
        if isinstance(self.latest_optimization_data, dict) and isinstance(self.latest_optimization_data.get("summary"), dict):
            summary = self.latest_optimization_data.get("summary") or {}
        elif self.last_result_dir is not None and (self.last_result_dir / "summary.json").exists():
            try:
                summary = _read_json(self.last_result_dir / "summary.json")
            except Exception as exc:
                messagebox.showerror("Load failed", str(exc))
                return
        if not summary:
            messagebox.showwarning("No exact results", "Load an exact Filip run first.")
            return
        try:
            csv_text = self._load_best_exact_output_csv_text(summary)
        except Exception as exc:
            messagebox.showerror("Load failed", str(exc))
            return
        if csv_text is not None:
            self._set_results_text(csv_text)
            base_dir = self.last_result_dir if isinstance(self.last_result_dir, Path) else None
            csv_path = self._best_exact_output_csv_path(summary, base_dir=base_dir)
            self.status_var.set(f"Loaded exact output csv: {csv_path.name if csv_path is not None else 'csv'}")
            return
        try:
            preview = self._load_best_exact_output_preview(summary)
        except Exception as exc:
            messagebox.showerror("Load failed", str(exc))
            return
        if preview is None:
            messagebox.showwarning("No output preview", "This run does not expose saved numerical outputs.")
            return
        self._set_results_text(preview)
        source = str(preview.get("_preview_source", "")).strip()
        self.status_var.set(f"Loaded exact output preview: {Path(source).name if source else 'preview'}")

    def _apply_template(self) -> None:
        tpl = TEMPLATES.get(self.template_var.get())
        if tpl is None:
            return
        self.script_var.set(tpl["script"])
        self.args_text.delete("1.0", "end")
        self.args_text.insert("1.0", tpl["args"])

    def _workflow_command(self) -> tuple[str, str]:
        spec = WORKFLOWS.get(self.workflow_var.get(), {})
        workflow_id = spec.get("id", "cpu_benchmark")
        args: List[str] = [
            "--workflow",
            workflow_id,
            "--profile",
            self.workflow_profile_var.get().strip() or "paper",
            "--platform-profile",
            self.workflow_platform_var.get().strip() or "auto",
            "--backend",
            self.workflow_backend_var.get().strip() or "auto",
            "--device-index",
            self.workflow_device_var.get().strip() or "0",
            "--roofline-ai",
            self.workflow_ai_var.get().strip() or "8",
            "--roofline-bytes",
            self.workflow_bytes_var.get().strip() or "1000000000",
            "--real-runs",
            self.workflow_real_runs_var.get().strip() or "3",
            "--repeats",
            self.workflow_repeats_var.get().strip() or "3",
            "--trials",
            self.workflow_trials_var.get().strip() or "96",
            "--population",
            self.workflow_population_var.get().strip() or "12",
            "--iterations",
            self.workflow_iterations_var.get().strip() or "20",
            "--filip-case",
            self.workflow_filip_case_var.get().strip() or "prism_pair",
            "--filip-mode",
            self.workflow_filip_mode_var.get().strip() or "portable_sweep",
        ]
        filip_modfem_dir = self.workflow_filip_modfem_var.get().strip()
        if filip_modfem_dir:
            args.extend(["--filip-modfem-dir", filip_modfem_dir])
        filip_input_override = self.workflow_filip_input_override_var.get().strip()
        if filip_input_override:
            args.extend(["--filip-input-override", filip_input_override])
        if bool(self.workflow_filip_dump_launch_var.get()):
            args.append("--filip-dump-launch-artifacts")
        replay_dump_root = self.workflow_filip_replay_dump_root_var.get().strip()
        if replay_dump_root:
            args.extend(["--filip-replay-dump-root", replay_dump_root])
        extra = self.workflow_extra_text.get("1.0", "end").strip()
        if extra:
            try:
                args.extend(shlex.split(extra, posix=True))
            except Exception:
                pass
        return "run_workflow.py", " ".join(shlex.quote(x) for x in args)

    def _refresh_workflow_description(self) -> None:
        spec = WORKFLOWS.get(self.workflow_var.get(), {})
        filip_case = self.workflow_filip_case_var.get().strip() or "prism_pair"
        filip_mode = self.workflow_filip_mode_var.get().strip() or "portable_sweep"
        replay_dump_root = self.workflow_filip_replay_dump_root_var.get().strip()
        dump_launch = bool(self.workflow_filip_dump_launch_var.get())
        self.workflow_filip_case_note_var.set(FILIP_CASE_DESCRIPTIONS.get(filip_case, ""))
        self.workflow_filip_mode_note_var.set(FILIP_MODE_DESCRIPTIONS.get(filip_mode, ""))
        desc = spec.get("description", "")
        if spec.get("id") == "filip_original":
            extra = FILIP_CASE_DESCRIPTIONS.get(filip_case, "")
            mode_extra = FILIP_MODE_DESCRIPTIONS.get(filip_mode, "")
            if extra or mode_extra:
                desc = f"{desc} Selected case: {extra} Selected mode: {mode_extra}"
            if filip_mode == "exact_reference":
                exact_bits: List[str] = []
                if dump_launch:
                    exact_bits.append("OpenCL launch dumps enabled")
                if replay_dump_root:
                    exact_bits.append(f"Metal replay root: {replay_dump_root}")
                if exact_bits:
                    desc = f"{desc} Exact extras: {'; '.join(exact_bits)}."
        self.workflow_desc_var.set(desc)
        script, args = self._workflow_command()
        self.workflow_preview_var.set(f"Command preview: python3 {script} {args}")

    def _browse_workflow_filip_modfem(self) -> None:
        initial_dir = self.workflow_filip_modfem_var.get().strip() or str(ROOT)
        selected = filedialog.askdirectory(initialdir=initial_dir, title="Select Filip mod_2022 directory")
        if selected:
            self.workflow_filip_modfem_var.set(selected)

    def _browse_workflow_filip_input_override(self) -> None:
        raw_initial = self.workflow_filip_input_override_var.get().strip()
        initial_dir = str(Path(raw_initial).expanduser().resolve().parent) if raw_initial else str(ROOT)
        selected = filedialog.askopenfilename(
            initialdir=initial_dir,
            title="Select input_interactive override",
            filetypes=[("Text files", "*.txt"), ("All files", "*.*")],
        )
        if selected:
            self.workflow_filip_input_override_var.set(selected)

    def _browse_workflow_filip_replay_dump_root(self) -> None:
        initial_dir = self.workflow_filip_replay_dump_root_var.get().strip() or str(ROOT)
        selected = filedialog.askdirectory(initialdir=initial_dir, title="Select OpenCL exact replay dump root")
        if selected:
            self.workflow_filip_replay_dump_root_var.set(selected)

    def _on_workflow_device_context_changed(self) -> None:
        self._refresh_workflow_description()
        self._refresh_workflow_devices()

    def _workflow_discovery_backends(self) -> List[str]:
        requested = self.workflow_backend_var.get().strip().lower() or "auto"
        workflow_id = WORKFLOWS.get(self.workflow_var.get(), {}).get("id", "")
        platform_profile = self.workflow_platform_var.get().strip().lower() or "auto"

        if requested in ("cpu", "cuda", "hip", "opencl", "metal"):
            return [requested]
        if requested == "intel":
            return ["intel"]
        if requested == "amd":
            return ["amd", "hip", "opencl"]

        if workflow_id in ("cpu_benchmark", "cpu_real_kernels"):
            return ["cpu"]
        if workflow_id in ("gpu_benchmark", "gpu_real_kernels"):
            if platform_profile == "apple":
                return ["metal", "opencl"]
            if platform_profile == "nvidia":
                return ["cuda", "opencl"]
            if platform_profile == "amd":
                return ["amd", "hip", "opencl"]
            if platform_profile in ("intel_arc", "intel_igpu"):
                return ["intel"]
            return ["cuda", "hip", "opencl", "intel", "amd", "metal"]
        return ["cpu", "cuda", "hip", "opencl", "intel", "amd", "metal"]

    def _refresh_workflow_devices(self) -> None:
        backends = self._workflow_discovery_backends()
        discovery_payload: Dict[str, Any] | None = None
        try:
            cmd = [
                _preferred_python_executable(),
                str(ROOT / "run_device_discovery.py"),
                "--backends",
                ",".join(backends),
                "--json",
            ]
            res = subprocess.run(
                cmd,
                cwd=str(ROOT),
                env=_build_runtime_env(),
                stdout=subprocess.PIPE,
                stderr=subprocess.PIPE,
                text=True,
                timeout=15.0,
                check=False,
            )
            if res.returncode == 0 and res.stdout.strip():
                discovery_payload = json.loads(res.stdout)
        except Exception:
            discovery_payload = None

        if discovery_payload is not None:
            discovery_rows = list(discovery_payload.get("backends", []))
            items = [dev for item in discovery_rows for dev in item.get("devices", [])]
        else:
            try:
                discovery = discover_backends(backends)
                discovery_rows = [item.to_dict() for item in discovery]
                items = [dev for item in discovery_rows for dev in item.get("devices", [])]
            except Exception as exc:
                self.workflow_device_items = []
                self.workflow_device_combo.configure(values=[])
                self.workflow_device_choice_var.set("")
                self.workflow_device_note_var.set(f"Device discovery failed: {type(exc).__name__}: {exc}")
                return

        self.workflow_device_items = items
        labels = [str(item.get("label", "")) for item in items]
        self.workflow_device_combo.configure(values=labels)
        if not items:
            reasons = []
            for item in discovery_rows:
                reason = item.get("error") or "no_devices_found"
                reasons.append(f"{item.get('backend', 'unknown')}: {reason}")
            self.workflow_device_choice_var.set("")
            self.workflow_device_note_var.set(
                f"No devices found for backends: {', '.join(backends)}. "
                f"Reasons: {'; '.join(reasons)}"
            )
            return

        current_backend = self.workflow_backend_var.get().strip().lower() or "auto"
        current_device = self.workflow_device_var.get().strip() or "0"
        selected_label = ""
        for item in items:
            item_backend = str(item.get("backend", "")).lower()
            item_index = str(item.get("device_index", ""))
            if current_backend in ("auto", "intel", "amd"):
                if item_index == current_device:
                    selected_label = str(item.get("label", ""))
                    break
            elif item_backend == current_backend and item_index == current_device:
                selected_label = str(item.get("label", ""))
                break
        if not selected_label:
            selected_label = labels[0]
        self.workflow_device_choice_var.set(selected_label)
        matched = next((item for item in items if str(item.get("label", "")) == selected_label), None)
        if matched is not None:
            self.workflow_device_note_var.set(
                f"Detected {len(items)} device(s). Selected entry: {matched.get('label', '')}. "
                "Choose a device from the list to pin backend and device-index."
            )

    def _apply_workflow_device_selection(self, *, pin_backend: bool = True) -> None:
        label = self.workflow_device_choice_var.get().strip()
        if not label:
            return
        for item in self.workflow_device_items:
            if str(item.get("label", "")) != label:
                continue
            backend = str(item.get("backend", "")).strip().lower()
            device_index = int(item.get("device_index", 0))
            if self.workflow_device_var.get().strip() != str(device_index):
                self.workflow_device_var.set(str(device_index))
            if pin_backend and backend and self.workflow_backend_var.get().strip().lower() != backend:
                self.workflow_backend_var.set(backend)
            vendor = str(item.get("vendor", "") or "").strip()
            platform_name = str(item.get("platform_name", "") or "").strip()
            suffix = []
            if vendor:
                suffix.append(vendor)
            if platform_name:
                suffix.append(platform_name)
            suffix_txt = " | ".join(suffix)
            if suffix_txt:
                self.workflow_device_note_var.set(
                    f"Selected backend={backend}, device-index={device_index}, device={item.get('device_name', '')} | {suffix_txt}"
                )
            else:
                self.workflow_device_note_var.set(
                    f"Selected backend={backend}, device-index={device_index}, device={item.get('device_name', '')}"
                )
            return

    def _mirror_workflow_to_launcher(self) -> None:
        script, args = self._workflow_command()
        self.script_var.set(script)
        self.args_text.delete("1.0", "end")
        self.args_text.insert("1.0", args)

    def _run_selected_workflow(self) -> None:
        self._mirror_workflow_to_launcher()
        self._run_command()

    def _run_command(self) -> None:
        if self.proc is not None and self.proc.poll() is None:
            messagebox.showwarning("Process running", "A command is already running.")
            return

        script = self.script_var.get().strip()
        raw_path = Path(script)
        script_path = raw_path if raw_path.is_absolute() else (ROOT / raw_path)
        if not script_path.exists():
            messagebox.showerror("Missing script", f"Script not found: {script_path}")
            return

        raw_args = self.args_text.get("1.0", "end").strip()
        try:
            args = shlex.split(raw_args, posix=True)
        except Exception as e:
            messagebox.showerror("Invalid args", f"Cannot parse arguments: {e}")
            return

        cmd = [sys.executable, str(script_path), *args]
        self.current_script = script_path.as_posix()
        self.output_lines = []
        self._reset_progress_state(script=self.current_script, args=args)
        self._append_log(f"\n=== RUN: {' '.join(cmd)} ===\n")
        self.status_var.set(f"Running: {script}")
        runtime_env = _build_runtime_env()

        try:
            self.proc = subprocess.Popen(
                cmd,
                cwd=str(ROOT),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
                env=runtime_env,
            )
        except Exception as e:
            self.proc = None
            messagebox.showerror("Run failed", str(e))
            self.status_var.set("Run failed")
            return

        self._start_live_monitor(self.proc.pid)

        def _reader_thread() -> None:
            assert self.proc is not None
            assert self.proc.stdout is not None
            for line in self.proc.stdout:
                self.log_queue.put(("line", line))
            code = self.proc.wait()
            self.log_queue.put(("done", code))

        t = threading.Thread(target=_reader_thread, daemon=True)
        t.start()
        self.after(100, self._poll_log_queue)
        self.after(200, lambda: self._tick_progress(self.progress_tick_token))

    def _poll_log_queue(self) -> None:
        keep_polling = False
        done_seen = False
        while True:
            try:
                kind, payload = self.log_queue.get_nowait()
            except queue.Empty:
                break

            if kind == "line":
                line = str(payload)
                self.output_lines.append(line)
                if len(self.output_lines) > 50000:
                    self.output_lines = self.output_lines[-30000:]
                self._append_log(line)
                self.last_output_ts = time.monotonic()
                keep_polling = True
            elif kind == "metrics":
                if isinstance(payload, LiveMetricsSnapshot):
                    self._apply_live_metrics(payload)
                    keep_polling = True
            elif kind == "done":
                code = int(payload)
                self._on_process_done(code)
                done_seen = True

        if done_seen:
            return
        if keep_polling or (self.proc is not None and self.proc.poll() is None):
            self.after(100, self._poll_log_queue)

    def _on_process_done(self, exit_code: int) -> None:
        self._stop_live_monitor()
        self.progress_tick_token += 1
        if self.progress_mode == "firefly" and exit_code == 0 and self.progress_total > 0:
            self.progress_var.set(100.0)
        if self.progress_mode in ("indeterminate", "firefly", "optimization_evals"):
            try:
                self.progress_bar.stop()
            except Exception:
                pass
        self.progress_mode = "idle"
        self.progress_label_var.set(f"Done (exit={exit_code})")
        self.monitor_state_var.set(f"Done (exit={exit_code})")

        self.status_var.set(f"Finished: {self.current_script} (exit={exit_code})")
        self._append_log(f"\n=== DONE (exit={exit_code}) ===\n")

        out_dir = self._extract_out_dir_from_output("".join(self.output_lines))
        script = self.current_script
        if out_dir is not None and out_dir.exists():
            self.result_dir_var.set(str(out_dir))
            self.last_result_dir = out_dir
            if _script_match(script, "run_workflow.py"):
                if (out_dir / "evaluations.jsonl").exists() and (out_dir / "summary.json").exists():
                    self._load_optimization_dir(out_dir)
                elif (out_dir / "manifest.json").exists():
                    self._load_session_dir(out_dir)
                    self._show_latest_plot()
                else:
                    self._set_results_text("".join(self.output_lines))
            elif _script_match(script, "analysis/filip_article_plots.py"):
                self._load_optimization_dir(out_dir)
                self._show_current_optimization_plot()
            elif _script_match(script, "analysis/compare_legacy_filip_xlsx.py"):
                self._load_compare_dir(out_dir)
            elif _script_match(script, "run_fem_parametric_matrix.py"):
                self._load_matrix_dir(out_dir)
            elif (
                _script_match(script, "run_firefly_optimization.py")
                or _script_match(script, "run_filip_autotuning.py")
                or _script_match(script, "run_filip_original.py")
            ):
                self._load_optimization_dir(out_dir)
            else:
                # For preflight we keep raw output in results tab.
                self._set_results_text("".join(self.output_lines))
        else:
            if _script_match(script, "run_fem_parametric_preflight.py"):
                self._set_results_text("".join(self.output_lines))
            elif (
                _script_match(script, "run_all_benchmarks.py")
                or _script_match(script, "run_all_backends.py")
                or _script_match(script, "run_all_cpu_benchmarks.py")
                or _script_match(script, "run_all_gpu_benchmarks.py")
                or _script_match(script, "real_kernels/run_all_real_kernels.py")
            ):
                self._load_latest_session()
            elif _script_match(script, "analysis/report.py"):
                self._load_latest_report()
            elif _script_match(script, "analysis/filip_article_plots.py"):
                self._load_latest_optimization()
                self._show_current_optimization_plot()
            elif _script_match(script, "analysis/compare_legacy_filip_xlsx.py"):
                self._set_results_text("".join(self.output_lines))
            elif _script_match(script, "analysis/generate_plots.py"):
                self._show_latest_plot()
            elif _script_match(script, "run_filip_autotuning.py") or _script_match(script, "run_filip_original.py"):
                self._load_latest_optimization()
            else:
                self._set_results_text("".join(self.output_lines))

        self.proc = None

    def _stop_command(self) -> None:
        if self.proc is None or self.proc.poll() is not None:
            messagebox.showinfo("No process", "No running command.")
            return
        try:
            self.proc.terminate()
            self.status_var.set("Stopping process...")
            self.progress_label_var.set("Stopping...")
            self.monitor_state_var.set("Stopping...")
        except Exception as e:
            messagebox.showerror("Stop failed", str(e))

    def _reset_progress_state(self, *, script: str, args: List[str]) -> None:
        self.run_start_ts = time.monotonic()
        self.last_output_ts = self.run_start_ts
        self._reset_monitor_state()
        self.progress_current = 0
        self.progress_out_dir = None
        self.progress_eval_path = None
        self.progress_eval_offset = 0
        self.progress_eval_lines = 0
        self.progress_tick_token += 1

        self.progress_var.set(0.0)

        workflow_id = _get_arg_value(args, "--workflow", "")

        if _script_match(script, "run_firefly_optimization.py") or (
            _script_match(script, "run_workflow.py") and workflow_id == "filip_firefly"
        ):
            pop = _to_int_or_default(_get_arg_value(args, "--population", "16"), 16)
            iters = _to_int_or_default(_get_arg_value(args, "--iterations", "25"), 25)
            self.progress_total = max(1, pop * (iters + 1))
            self.progress_mode = "firefly"
            self.progress_bar.configure(mode="determinate")
            base = ROOT / "data" / "optimization"
            if base.exists():
                self.progress_known_dirs = {p.name for p in base.iterdir() if p.is_dir()}
            else:
                self.progress_known_dirs = set()
            self.progress_label_var.set(f"Firefly progress: 0/{self.progress_total} evals")
        elif _script_match(script, "run_filip_autotuning.py") or (
            _script_match(script, "run_workflow.py") and workflow_id == "filip_autotune"
        ):
            trials = _to_int_or_default(_get_arg_value(args, "--trials", "96"), 96)
            self.progress_total = max(1, trials)
            self.progress_mode = "optimization_evals"
            self.progress_bar.configure(mode="determinate")
            base = ROOT / "data" / "optimization"
            if base.exists():
                self.progress_known_dirs = {p.name for p in base.iterdir() if p.is_dir()}
            else:
                self.progress_known_dirs = set()
            self.progress_label_var.set(f"Autotuning progress: 0/{self.progress_total} evals")
        elif _script_match(script, "run_filip_original.py") or (
            _script_match(script, "run_workflow.py") and workflow_id == "filip_original"
        ):
            filip_case = _get_arg_value(args, "--benchmark-case", _get_arg_value(args, "--filip-case", "portable")).strip() or "portable"
            case_defaults = {
                "portable": (["diffusion", "diffusion_convection_mass"], ["tet4"]),
                "laplace_prism": (["laplace"], ["prism6"]),
                "test_prism": (["test"], ["prism6"]),
                "prism_pair": (["laplace", "test"], ["prism6"]),
            }
            default_operators, default_element_types = case_defaults.get(
                filip_case,
                case_defaults["portable"],
            )
            operators = _parse_csv_arg(_get_arg_value(args, "--operators", ",".join(default_operators)))
            element_types = _parse_csv_arg(_get_arg_value(args, "--element-types", ",".join(default_element_types)))
            variants = _parse_csv_arg(_get_arg_value(args, "--variants", "qss,sqs,ssq"))
            option_rows = _to_int_or_default(_get_arg_value(args, "--limit-option-rows", "80"), 80)
            self.progress_total = max(1, len(operators) * len(element_types) * len(variants) * option_rows)
            self.progress_mode = "optimization_evals"
            self.progress_bar.configure(mode="determinate")
            base = ROOT / "data" / "optimization"
            if base.exists():
                self.progress_known_dirs = {p.name for p in base.iterdir() if p.is_dir()}
            else:
                self.progress_known_dirs = set()
            self.progress_label_var.set(f"Filip original progress: 0/{self.progress_total} evals")
        else:
            self.progress_total = 0
            self.progress_mode = "indeterminate"
            self.progress_bar.configure(mode="indeterminate")
            self.progress_bar.start(12)
            self.progress_label_var.set("Running...")

    def _detect_new_firefly_out_dir(self) -> None:
        if self.progress_out_dir is not None:
            return
        base = ROOT / "data" / "optimization"
        if not base.exists():
            return
        candidates = [
            p
            for p in base.iterdir()
            if p.is_dir() and p.name not in self.progress_known_dirs and p.stat().st_mtime >= (self.run_start_ts - 2.0)
        ]
        if not candidates:
            return
        newest = max(candidates, key=lambda p: p.stat().st_mtime)
        self.progress_out_dir = newest
        self.progress_eval_path = newest / "evaluations.jsonl"
        self.result_dir_var.set(str(newest))

    def _update_firefly_eval_counter(self) -> None:
        self._detect_new_firefly_out_dir()
        path = self.progress_eval_path
        if path is None or not path.exists():
            return
        try:
            with path.open("rb") as f:
                f.seek(self.progress_eval_offset)
                chunk = f.read()
        except Exception:
            return
        if not chunk:
            return
        self.progress_eval_offset += len(chunk)
        self.progress_eval_lines += chunk.count(b"\n")
        self.progress_current = self.progress_eval_lines

    def _tick_progress(self, token: int) -> None:
        if token != self.progress_tick_token:
            return
        if self.proc is None or self.proc.poll() is not None:
            return

        now = time.monotonic()
        elapsed = now - self.run_start_ts
        since_output = now - self.last_output_ts

        if self.progress_mode in ("firefly", "optimization_evals"):
            self._update_firefly_eval_counter()
            total = max(1, self.progress_total)
            cur = min(self.progress_current, total)
            pct = 100.0 * float(cur) / float(total)
            self.progress_var.set(pct)
            prefix = "Firefly progress" if self.progress_mode == "firefly" else "Autotuning progress"
            self.progress_label_var.set(
                f"{prefix}: {cur}/{total} evals ({pct:.1f}%) | elapsed { _format_hms(elapsed) } | last output { _format_hms(since_output) } ago"
            )
        else:
            # Keep heartbeat visible even when script has sparse output.
            self.progress_label_var.set(
                f"Running | elapsed { _format_hms(elapsed) } | last output { _format_hms(since_output) } ago"
            )

        self.after(500, lambda: self._tick_progress(token))

    @staticmethod
    def _extract_out_dir_from_output(output: str) -> Path | None:
        # run_firefly_optimization summary JSON
        m = re.search(r'"out_dir"\s*:\s*"([^"]+)"', output)
        if m:
            return Path(m.group(1))
        m = re.search(r'"optimization_dir"\s*:\s*"([^"]+)"', output)
        if m:
            return Path(m.group(1))
        # run_fem_parametric_matrix human output
        m = re.search(r"out_dir:\s*([^\n]+)", output)
        if m:
            return Path(m.group(1).strip())
        return None

    def _browse_result_dir(self) -> None:
        path = filedialog.askdirectory(initialdir=str(ROOT))
        if path:
            self.result_dir_var.set(path)

    def _load_selected_optimization(self) -> None:
        path = Path(self.result_dir_var.get().strip())
        self._load_optimization_dir(path)

    def _load_selected_matrix(self) -> None:
        path = Path(self.result_dir_var.get().strip())
        self._load_matrix_dir(path)

    def _load_latest_optimization(self) -> None:
        base = ROOT / "data" / "optimization"
        if not base.exists():
            messagebox.showwarning("No results", f"No directory: {base}")
            return
        dirs = [d for d in base.iterdir() if d.is_dir()]
        if not dirs:
            messagebox.showwarning("No results", "No optimization runs found.")
            return
        latest = max(dirs, key=lambda d: d.stat().st_mtime)
        self.result_dir_var.set(str(latest))
        self._load_optimization_dir(latest)

    def _load_latest_matrix(self) -> None:
        base = ROOT / "data" / "validation"
        if not base.exists():
            messagebox.showwarning("No results", f"No directory: {base}")
            return
        dirs = [d for d in base.iterdir() if d.is_dir()]
        if not dirs:
            messagebox.showwarning("No results", "No matrix validation runs found.")
            return
        latest = max(dirs, key=lambda d: d.stat().st_mtime)
        self.result_dir_var.set(str(latest))
        self._load_matrix_dir(latest)

    @staticmethod
    def _latest_child(base: Path, pattern: str, only_dirs: bool = False) -> Path | None:
        if not base.exists():
            return None
        items = list(base.glob(pattern))
        if only_dirs:
            items = [p for p in items if p.is_dir()]
        else:
            items = [p for p in items if p.exists()]
        if not items:
            return None
        return max(items, key=lambda p: p.stat().st_mtime)

    @staticmethod
    def _preferred_plot_file(base: Path) -> Path | None:
        if not base.exists():
            return None
        for name in OPTIMIZATION_PLOT_PREFERENCE:
            candidate = base / name
            if candidate.exists():
                return candidate
        return AutotuneGui._latest_child(base, "*.png", only_dirs=False)

    @staticmethod
    def _optimization_plots_dir(base_dir: Path) -> Path | None:
        primary = base_dir / "plots"
        fallback = ROOT / "analysis" / "plots" / f"{base_dir.name}__filip_article"
        for candidate in (primary, fallback):
            if AutotuneGui._preferred_plot_file(candidate) is not None:
                return candidate
        return None

    def _show_text_file(self, path: Path) -> None:
        try:
            text = path.read_text(encoding="utf-8", errors="replace")
        except Exception as e:
            messagebox.showerror("Read failed", str(e))
            return
        self._set_results_text(text)
        self.result_dir_var.set(str(path))
        self.status_var.set(f"Loaded file: {path}")

    def _load_latest_session(self) -> None:
        base = ROOT / "data" / "runs"
        latest = self._latest_child(base, "*", only_dirs=True)
        if latest is None:
            messagebox.showwarning("No session", f"No session directories in {base}")
            return
        self._load_session_dir(latest)

    def _load_session_dir(self, path: Path) -> None:
        self.result_dir_var.set(str(path))
        manifest = path / "manifest.json"
        if manifest.exists():
            try:
                payload = _read_json(manifest)
                payload["_session_dir"] = str(path)
                self._set_results_text(payload)
            except Exception:
                self._show_text_file(manifest)
        else:
            self._set_results_text(f"Session directory: {path}\nManifest not found.")
        self.status_var.set(f"Loaded session: {path.name}")

    def _load_latest_report(self) -> None:
        base = ROOT / "reports"
        latest = self._latest_child(base, "*.md", only_dirs=False)
        if latest is None:
            messagebox.showwarning("No report", f"No markdown reports in {base}")
            return
        self._show_text_file(latest)

    def _show_latest_plot(self) -> None:
        base = ROOT / "analysis" / "plots"
        latest = self._latest_child(base, "*.png", only_dirs=False)
        if latest is None:
            messagebox.showwarning("No plot", f"No plot PNG files in {base}")
            return
        self._plot_image_file(latest)

    def _show_current_optimization_plot(self) -> None:
        base_dir = self.last_result_dir
        if base_dir is None:
            raw = self.result_dir_var.get().strip()
            if raw:
                base_dir = Path(raw)
        if base_dir is None:
            messagebox.showwarning("No optimization", "No optimization result directory is loaded.")
            return
        plots_dir = self._optimization_plots_dir(base_dir)
        if plots_dir is None:
            messagebox.showwarning("No plot", f"No PNG files for optimization run {base_dir}")
            return
        selected = self._preferred_plot_file(plots_dir)
        if selected is None:
            messagebox.showwarning("No plot", f"No PNG files in {plots_dir}")
            return
        self._plot_image_file(selected)

    def _plot_image_file(self, path: Path) -> None:
        if not HAS_MPL or self.figure is None or self.canvas is None:
            messagebox.showwarning("No matplotlib", "matplotlib backend is unavailable.")
            return
        try:
            import matplotlib.image as mpimg  # type: ignore

            img = mpimg.imread(str(path))
        except Exception as e:
            messagebox.showerror("Image load failed", str(e))
            return
        self.figure.clear()
        ax = self.figure.add_axes([0.01, 0.04, 0.98, 0.90])
        ax.imshow(img, aspect="auto")
        ax.set_axis_off()
        ax.set_title(path.name)
        self.canvas.draw_idle()
        self.status_var.set(f"Loaded plot image: {path.name}")

    @staticmethod
    def _preferred_compare_plot(base: Path) -> Path | None:
        if not base.exists():
            return None
        preferred = [
            "legacy_vs_current_internal_ns_per_elem.png",
            "legacy_vs_current_ns_per_unit.png",
            "legacy_reference_internal_ns_per_elem.png",
            "legacy_vs_current_kernel_ms.png",
            "legacy_reference_ns_per_unit.png",
            "legacy_reference_kernel_ms.png",
        ]
        for name in preferred:
            candidate = base / name
            if candidate.exists():
                return candidate
        pngs = [p for p in base.glob("*.png") if p.is_file()]
        if not pngs:
            return None
        return max(pngs, key=lambda p: p.stat().st_mtime)

    def _default_legacy_compare_operator(self) -> str:
        data = self.latest_optimization_data or {}
        summary = data.get("summary") or {}
        operators = [str(op).strip().lower() for op in summary.get("operators", []) if str(op).strip()]
        benchmark_case = str(summary.get("benchmark_case", "")).strip().lower()
        if "laplace" in operators:
            return "laplace"
        if benchmark_case == "test_prism" and "test" in operators:
            return "test"
        if operators:
            return operators[0]
        best = summary.get("best_overall") or {}
        config = best.get("config") or {}
        operator = str(config.get("operator", "")).strip().lower()
        return operator or "laplace"

    def _load_compare_dir(self, path: Path) -> None:
        summary_path = path / "summary.json"
        if not summary_path.exists():
            messagebox.showerror("Invalid compare directory", f"Missing file: {summary_path}")
            return
        try:
            summary = _read_json(summary_path)
        except Exception as exc:
            messagebox.showerror("Load failed", str(exc))
            return

        self.last_compare_dir = path
        self._set_results_text(summary)
        selected = self._preferred_compare_plot(path)
        if selected is not None:
            self._plot_image_file(selected)
        self.status_var.set(f"Loaded legacy comparison: {path.name}")

    def _run_legacy_xlsx_compare(self) -> None:
        if self.proc is not None and self.proc.poll() is None:
            messagebox.showwarning("Process running", "Finish the current command before starting legacy comparison.")
            return

        optimization_dir = self.last_result_dir
        if optimization_dir is None or not (optimization_dir / "summary.json").exists():
            messagebox.showwarning("No optimization run", "Load or run a Filip benchmark first, then compare it against legacy XLSX.")
            return

        summary = {}
        try:
            summary = _read_json(optimization_dir / "summary.json")
        except Exception:
            summary = {}
        benchmark_case = str(summary.get("benchmark_case", "")).strip().lower()
        preferred_dir = Path.home() / "Downloads"
        filip_ref_dir = ROOT / "Kod Filipa" / "mod_2022" / "work"
        if benchmark_case == "laplace_prism" and (filip_ref_dir / "diff_in_box").exists():
            preferred_dir = filip_ref_dir / "diff_in_box"
        elif benchmark_case == "test_prism" and (filip_ref_dir / "test_scalar").exists():
            preferred_dir = filip_ref_dir / "test_scalar"

        paths = filedialog.askopenfilenames(
            title="Select legacy Filip reference files",
            initialdir=str(preferred_dir),
            filetypes=[("Reference files", "*.xlsx *.csv"), ("Excel", "*.xlsx"), ("CSV", "*.csv"), ("All files", "*.*")],
        )
        if not paths:
            return

        operator = self._default_legacy_compare_operator()
        args: List[str] = []
        for path in paths:
            suffix = Path(path).suffix.lower()
            args.extend(["--csv" if suffix == ".csv" else "--xlsx", path])
        args.extend(
            [
                "--optimization-dir",
                str(optimization_dir),
                "--current-operator",
                operator,
            ]
        )
        if benchmark_case in {"laplace_prism", "test_prism", "prism_pair"}:
            args.append("--strict-fig4")
        self.script_var.set("analysis/compare_legacy_filip_xlsx.py")
        self.args_text.delete("1.0", "end")
        self.args_text.insert("1.0", " ".join(shlex.quote(x) for x in args))
        self._run_command()

    def _load_optimization_dir(self, path: Path) -> None:
        summary_path = path / "summary.json"
        if not summary_path.exists():
            messagebox.showerror("Invalid run directory", f"Missing file: {summary_path}")
            return

        try:
            summary = _read_json(summary_path)
            it_rows = _read_jsonl(path / "iterations.jsonl")
            eval_rows = _read_jsonl(path / "evaluations.jsonl")
        except Exception as e:
            messagebox.showerror("Load failed", str(e))
            return

        self.latest_optimization_data = {
            "summary": summary,
            "iterations": it_rows,
            "evaluations": eval_rows,
            "out_dir": str(path),
        }
        self.last_result_dir = path
        self.result_dir_var.set(str(path))
        self._set_results_text(self.latest_optimization_data)
        if self._optimization_plots_dir(path) is not None:
            self._show_current_optimization_plot()
        else:
            self._plot_firefly_convergence()

    def _load_matrix_dir(self, path: Path) -> None:
        summary_path = path / "summary.json"
        if not summary_path.exists():
            messagebox.showerror("Invalid run directory", f"Missing file: {summary_path}")
            return

        try:
            summary = _read_json(summary_path)
        except Exception as e:
            messagebox.showerror("Load failed", str(e))
            return

        self.latest_matrix_data = summary
        self.last_result_dir = path
        self.result_dir_var.set(str(path))
        self._set_results_text(summary)
        self._plot_matrix_comparison()

    def _plot_firefly_convergence(self) -> None:
        if not HAS_MPL or self.figure is None or self.canvas is None:
            return
        data = self.latest_optimization_data
        self.figure.clear()
        ax1 = self.figure.add_subplot(2, 1, 1)
        ax2 = self.figure.add_subplot(2, 1, 2)
        method = str((data.get("summary") or {}).get("method", "")).strip().lower()
        iter_label = "Trial" if method == "random_search" else "Iteration"
        best_label = "Best Score per Trial" if method == "random_search" else "Best Brightness per Iteration"
        gflops_label = (
            "Best Feasible gflops_mean per Trial" if method == "random_search" else "Best Feasible gflops_mean per Iteration"
        )

        it_rows = data.get("iterations", [])
        if it_rows:
            xs = [int(r.get("iteration", 0)) for r in it_rows]
            ys = [float(r.get("best_brightness", float("nan"))) for r in it_rows]
            ax1.plot(xs, ys, marker="o", linewidth=1.5)
            ax1.set_title(best_label)
            ax1.set_xlabel(iter_label)
            ax1.set_ylabel("Score" if method == "random_search" else "Brightness")
            ax1.grid(True, alpha=0.3)
        else:
            ax1.text(0.5, 0.5, "No iterations.jsonl data", ha="center", va="center")
            ax1.set_axis_off()

        eval_rows = data.get("evaluations", [])
        best_by_it: Dict[int, float] = {}
        for r in eval_rows:
            try:
                it = int(r.get("iteration", 0))
                status = str(r.get("status", ""))
                ok = int(r.get("constraints_ok", 0)) == 1
                g = float(r.get("metric_gflops_mean", float("nan")))
            except Exception:
                continue
            if status != "ok" or not ok or not math.isfinite(g):
                continue
            prev = best_by_it.get(it)
            if prev is None or g > prev:
                best_by_it[it] = g

        if best_by_it:
            xs = sorted(best_by_it.keys())
            ys = [best_by_it[i] for i in xs]
            ax2.plot(xs, ys, marker="o", linewidth=1.5, color="#0f766e")
            ax2.set_title(gflops_label)
            ax2.set_xlabel(iter_label)
            ax2.set_ylabel("gflops_mean")
            ax2.grid(True, alpha=0.3)
        else:
            ax2.text(0.5, 0.5, "No feasible gflops data in evaluations.jsonl", ha="center", va="center")
            ax2.set_axis_off()

        self.figure.tight_layout()
        self.canvas.draw_idle()

    def _plot_firefly_scatter(self) -> None:
        if not HAS_MPL or self.figure is None or self.canvas is None:
            return
        data = self.latest_optimization_data
        eval_rows = data.get("evaluations", [])

        self.figure.clear()
        ax = self.figure.add_subplot(1, 1, 1)

        x_ok: List[float] = []
        y_ok: List[float] = []
        c_ok: List[float] = []
        x_bad: List[float] = []
        y_bad: List[float] = []
        for r in eval_rows:
            try:
                g = float(r.get("metric_gflops_mean", float("nan")))
                b = float(r.get("metric_gbps_mean", float("nan")))
                br = float(r.get("brightness", float("nan")))
                status = str(r.get("status", ""))
                ok = int(r.get("constraints_ok", 0)) == 1
            except Exception:
                continue
            if not math.isfinite(g) or not math.isfinite(b):
                continue
            if status == "ok" and ok:
                x_ok.append(g)
                y_ok.append(b)
                c_ok.append(br if math.isfinite(br) else 0.0)
            else:
                x_bad.append(g)
                y_bad.append(b)

        if x_ok:
            s = ax.scatter(x_ok, y_ok, c=c_ok, cmap="viridis", alpha=0.85, label="feasible")
            self.figure.colorbar(s, ax=ax, label="brightness")
        if x_bad:
            ax.scatter(x_bad, y_bad, marker="x", color="#b91c1c", alpha=0.8, label="infeasible/error")

        if not x_ok and not x_bad:
            ax.text(0.5, 0.5, "No evaluations data", ha="center", va="center")
            ax.set_axis_off()
        else:
            ax.set_title("Evaluations Scatter (gflops_mean vs gbps_mean)")
            ax.set_xlabel("gflops_mean")
            ax.set_ylabel("gbps_mean")
            ax.grid(True, alpha=0.3)
            ax.legend()

        self.figure.tight_layout()
        self.canvas.draw_idle()

    def _plot_matrix_comparison(self) -> None:
        if not HAS_MPL or self.figure is None or self.canvas is None:
            return
        data = self.latest_matrix_data
        comps = data.get("comparisons_vs_baseline", [])

        self.figure.clear()
        ax = self.figure.add_subplot(1, 1, 1)

        labels: List[str] = []
        spearman_vals: List[float] = []
        overlap_vals: List[float] = []
        for item in comps:
            if int(item.get("available", 0)) != 1:
                continue
            cmp = item.get("compare") or {}
            s = float(cmp.get("spearman_gflops", float("nan")))
            o = float(cmp.get("topk_overlap", float("nan")))
            labels.append(str(item.get("backend", "unknown")))
            spearman_vals.append(0.0 if not math.isfinite(s) else s)
            overlap_vals.append(0.0 if not math.isfinite(o) else o)

        if not labels:
            ax.text(0.5, 0.5, "No matrix comparison data", ha="center", va="center")
            ax.set_axis_off()
        else:
            xs = list(range(len(labels)))
            w = 0.38
            ax.bar([x - w / 2 for x in xs], spearman_vals, width=w, label="spearman_gflops")
            ax.bar([x + w / 2 for x in xs], overlap_vals, width=w, label="topk_overlap")
            ax.set_xticks(xs)
            ax.set_xticklabels(labels, rotation=20, ha="right")
            ax.set_ylim(-1.0, 1.0)
            ax.set_title("Matrix Comparison vs Baseline")
            ax.set_ylabel("Score")
            ax.grid(True, axis="y", alpha=0.3)
            ax.legend()

        self.figure.tight_layout()
        self.canvas.draw_idle()

    def _save_current_figure(self) -> None:
        if not HAS_MPL or self.figure is None:
            return
        out = filedialog.asksaveasfilename(
            initialdir=str(ROOT / "analysis" / "plots"),
            defaultextension=".png",
            filetypes=[("PNG", "*.png"), ("All files", "*.*")],
        )
        if not out:
            return
        try:
            self.figure.savefig(out, dpi=160)
            self.status_var.set(f"Saved figure: {out}")
        except Exception as e:
            messagebox.showerror("Save failed", str(e))


def main() -> None:
    app = AutotuneGui()
    app.mainloop()


if __name__ == "__main__":
    main()
