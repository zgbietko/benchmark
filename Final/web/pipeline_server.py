#!/usr/bin/env python3
from __future__ import annotations

import argparse
from dataclasses import dataclass, field
from datetime import datetime
from http import HTTPStatus
from http.server import SimpleHTTPRequestHandler, ThreadingHTTPServer
import json
import mimetypes
import os
from pathlib import Path
import platform
import queue
import signal
import subprocess
import sys
import threading
import time
from typing import Any, Dict, Iterable, List
from urllib.parse import parse_qs, urlparse
import webbrowser

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.publication_style import THESIS_CORE_DIR

STATIC_DIR = ROOT / "web" / "static"
THESIS_DIR = ROOT / "data" / "thesis_full"
OPT_DIR = ROOT / "data" / "optimization"
VALIDATION_DIR = ROOT / "data" / "fem_option_validation"
ANALYSIS_PLOTS_DIR = THESIS_CORE_DIR
PYTHON = sys.executable or "python3"

MARKER = "=== WORKFLOW DONE ==="

PIPELINE_STAGES: list[dict[str, Any]] = [
    {
        "id": "platform_characterization",
        "order": 1,
        "label": "Etap 1. Charakterystyka platformy",
        "subtitle": "Mikrobenchmarki, hierarchia pamięci, roofline",
        "description": "Na tym etapie opisujemy surowe własności platformy obliczeniowej. To tutaj mieszczą się mikrobenchmarki CPU i GPU, pomiary przepustowości, roofline oraz opóźnienia pamięci, w tym strefy L1, L2, L3 i TLB/page-walk. Badanie opóźnień pamięci nie jest więc osobnym równorzędnym etapem, tylko częścią mikrobenchmarków.",
        "y": 20,
        "height": 180,
    },
    {
        "id": "real_kernel_characterization",
        "order": 2,
        "label": "Etap 2. Jądra uproszczone i real kernels",
        "subtitle": "Przejście od prymitywów do bardziej realistycznych obciążeń",
        "description": "Na tym etapie sprawdzamy, czy obserwacje z mikrobenchmarków utrzymują się na uproszczonych, ale już bardziej praktycznych jądrach obliczeniowych. To wciąż nie jest pełny kod aplikacyjny, ale etap pośredni między prymitywem a aplikacją.",
        "y": 215,
        "height": 170,
    },
    {
        "id": "fem_bridge",
        "order": 3,
        "label": "Etap 3. Most interpretacyjny FEM",
        "subtitle": "Walidacja opcji FEM na wspólnym problemie testowym",
        "description": "Ten etap łączy charakterystykę platformy z zachowaniem obliczeń zbliżonych do rzeczywistego problemu MES/FEM. To tu zaczyna się interpretacja opcji obliczeniowych w języku problemu numerycznego, a nie tylko sprzętu.",
        "y": 405,
        "height": 150,
    },
    {
        "id": "application_campaign",
        "order": 4,
        "label": "Etap 4. Kampania aplikacyjna: kod Filipa",
        "subtitle": "Pełna przestrzeń konfiguracji oraz strategie strojenia",
        "description": "To główny etap aplikacyjny. Najpierw uruchamiany jest pełny sweep wszystkich konfiguracji, a następnie strategie wyszukiwania dobrych ustawień, takie jak autotuning i Firefly. Dzięki temu można analizować zarówno pełny krajobraz konfiguracji, jak i zachowanie metod strojenia.",
        "y": 575,
        "height": 310,
    },
    {
        "id": "correctness_validation",
        "order": 5,
        "label": "Etap 5. Walidacja poprawności obliczeń",
        "subtitle": "Exact reference i replay na zamrożonych danych wejściowych",
        "description": "Ten etap nie służy przede wszystkim do pomiaru wydajności, tylko do potwierdzenia, że porównywane backendy liczą to samo. Exact OpenCL albo replay na Metalu odpowiadają na pytanie o zgodność wyniku obliczeń.",
        "y": 930,
        "height": 150,
    },
    {
        "id": "synthesis",
        "order": 6,
        "label": "Etap 6. Synteza i interpretacja",
        "subtitle": "Profiler correlation i składanie wniosków",
        "description": "Na tym etapie łączone są obserwacje ze sprzętu, mikrobenchmarków, walidacji FEM, kampanii aplikacyjnej i profilera. To końcowy etap interpretacyjny, z którego wychodzą wnioski do rozprawy.",
        "y": 1110,
        "height": 150,
    },
]

PIPELINE_STEPS: list[dict[str, Any]] = [
    {
        "id": "cpu_benchmark",
        "label": "CPU mikrobenchmarki i pamiec",
        "workflow": "cpu_benchmark",
        "description": "Charakterystyka procesora: przepustowosc, roofline CPU, opoznienia pamieci oraz analiza stref L1, L2, L3 i TLB/page-walk.",
        "inputs": ["profil kampanii", "architektura CPU", "zestaw benchmarkow CPU"],
        "outputs": ["sesja CPU", "CSV benchmarkow", "wykresy CPU", "wykresy pamieci L1/L2/L3/TLB", "roofline CPU"],
        "depends_on": [],
        "x": 40,
        "y": 80,
        "group": "micro",
        "stage_id": "platform_characterization",
    },
    {
        "id": "gpu_benchmark",
        "label": "GPU mikrobenchmarki i roofline",
        "workflow": "gpu_benchmark",
        "description": "Charakterystyka backendu GPU: przepustowosc, opoznienia, roofline GPU i pomiary pomocnicze.",
        "inputs": ["backend GPU", "device index", "zestaw benchmarkow GPU"],
        "outputs": ["sesja GPU", "CSV benchmarkow", "lokalne wykresy", "roofline GPU"],
        "depends_on": [],
        "x": 380,
        "y": 80,
        "group": "micro",
        "stage_id": "platform_characterization",
    },
    {
        "id": "cpu_real_kernels",
        "label": "CPU real kernels",
        "workflow": "cpu_real_kernels",
        "description": "Uruchomienie rzeczywistych kerneli na CPU, aby sprawdzic, jak mikrobenchmarki przekladaja sie na bardziej realistyczne obciazenie.",
        "inputs": ["sesja CPU", "profil kampanii", "zestaw real kernels CPU"],
        "outputs": ["sesja real kernels CPU", "metryki wydajnosci", "wykresy"],
        "depends_on": ["cpu_benchmark"],
        "x": 40,
        "y": 250,
        "group": "real",
        "stage_id": "real_kernel_characterization",
    },
    {
        "id": "gpu_real_kernels",
        "label": "GPU real kernels",
        "workflow": "gpu_real_kernels",
        "description": "Uruchomienie rzeczywistych kerneli na GPU, z zachowaniem tej samej architektury pomiaru co dla CPU.",
        "inputs": ["sesja GPU", "backend GPU", "zestaw real kernels GPU"],
        "outputs": ["sesja real kernels GPU", "metryki wydajnosci", "wykresy"],
        "depends_on": ["gpu_benchmark"],
        "x": 380,
        "y": 250,
        "group": "real",
        "stage_id": "real_kernel_characterization",
    },
    {
        "id": "ai_accel",
        "label": "AI acceleration paths",
        "workflow": "ai_accel",
        "description": "Testy matmul pod katem sciezek akceleracji AI (GPU i runtime AI), z oznaczeniem native/proxy/fallback/unsupported i tym samym kontraktem artefaktow.",
        "inputs": ["backend", "device index", "standard/extended", "zestaw rozmiarow i precyzji"],
        "outputs": ["sesja ai_accel", "CSV matmul/coreml probe", "podsumowanie GFLOP/s", "wykres ai_accel_overview"],
        "depends_on": ["cpu_benchmark"],
        "x": 720,
        "y": 250,
        "group": "real",
        "stage_id": "real_kernel_characterization",
    },
    {
        "id": "fem_option_validation",
        "label": "Walidacja opcji FEM",
        "workflow": "fem_option_validation",
        "description": "Walidacja wzorcow opcji FEM na wspolnym problemie testowym. Ten krok laczy mikrobenchmarki z obciazeniem bardziej zblizonym do rzeczywistego kernela.",
        "inputs": ["backend FEM", "warianty qss/sqs/ssq", "operatorzy laplace/test"],
        "outputs": ["katalog walidacji FEM", "CSV probe summary", "wykresy walidacyjne"],
        "depends_on": ["cpu_real_kernels", "gpu_real_kernels"],
        "x": 210,
        "y": 430,
        "group": "validation",
        "stage_id": "fem_bridge",
    },
    {
        "id": "filip_original_portable",
        "label": "Kod Filipa: pelny sweep",
        "workflow": "filip_original",
        "description": "Pelna kampania przenosnego sweepu dla kodu Filipa. Daje zbior wszystkich kombinacji oraz czasy dla danej platformy.",
        "inputs": ["backend obliczeniowy", "problem Filipa", "wszystkie kombinacje qss/sqs/ssq"],
        "outputs": ["katalog optimization", "wyniki wszystkich kombinacji", "wykresy artykulowe"],
        "depends_on": ["fem_option_validation"],
        "x": 210,
        "y": 600,
        "group": "filip",
        "stage_id": "application_campaign",
    },
    {
        "id": "filip_autotune",
        "label": "Kod Filipa: Autotuning",
        "workflow": "filip_autotune",
        "description": "Losowe przeszukiwanie przestrzeni ustawien w celu znalezienia dobrych konfiguracji i porownania ich z kampania pelna.",
        "inputs": ["backend obliczeniowy", "liczba prob", "problem Filipa"],
        "outputs": ["katalog optimization", "trajektoria autotuningu", "najlepsza konfiguracja"],
        "depends_on": ["filip_original_portable"],
        "x": 40,
        "y": 790,
        "group": "tuning",
        "stage_id": "application_campaign",
    },
    {
        "id": "filip_firefly",
        "label": "Kod Filipa: Firefly",
        "workflow": "filip_firefly",
        "description": "Metaheurystyka Firefly uruchomiona na tej samej przestrzeni ustawien, aby porownac sposob eksploracji i zbieznosc.",
        "inputs": ["backend obliczeniowy", "populacja", "iteracje"],
        "outputs": ["katalog optimization", "historia firefly", "najlepsza konfiguracja"],
        "depends_on": ["filip_original_portable"],
        "x": 380,
        "y": 790,
        "group": "tuning",
        "stage_id": "application_campaign",
    },
    {
        "id": "filip_exact_reference",
        "label": "Walidacja poprawnosci: exact / replay",
        "workflow": "filip_original",
        "description": "Najbardziej rygorystyczna sciezka porownawcza: exact OpenCL na Linuxie albo replay na Metalu z zamrozonymi wejsciami.",
        "inputs": ["bundle replay albo OpenCL exact", "problem Filipa", "backend exact"],
        "outputs": ["katalog exact", "dumpy lub replay", "walidacja wyniku"],
        "depends_on": ["filip_original_portable"],
        "x": 210,
        "y": 980,
        "group": "exact",
        "stage_id": "correctness_validation",
    },
    {
        "id": "profiler_correlation",
        "label": "Synteza: profiler correlation",
        "workflow": "profiler_correlation",
        "description": "Powiazanie obserwacji z mikrobenchmarkow i walidacji FEM z wynikami profilera oraz danymi z kodu Filipa.",
        "inputs": ["optimization dir", "fem option validation dir", "raporty profilera opcjonalnie"],
        "outputs": ["raport korelacji", "CSV zgodnosci", "wykresy korelacyjne"],
        "depends_on": ["fem_option_validation", "filip_exact_reference"],
        "x": 210,
        "y": 1160,
        "group": "analysis",
        "stage_id": "synthesis",
    },
]
STEP_INDEX = {step["id"]: step for step in PIPELINE_STEPS}

GLOBAL_PLOTS = [
    "cpu_memcpy_bandwidth_scaling.png",
    "cpu_stream_triad_scaling.png",
    "cpu_peak_compute_scaling.png",
    "cpu_memory_latency_hierarchy.png",
    "gpu_microbenchmark_suite.png",
    "platform_roofline_measured.png",
    "real_kernels_model_validation.png",
    "real_kernels_filip_contrast_map.png",
    "ai_accel_overview.png",
    "ai_accel_break_even.png",
    "ai_precision_scaling.png",
]

BENCHMARK_PLOT_NAMES = [
    "cpu_memcpy_bandwidth_scaling.png",
    "cpu_stream_triad_scaling.png",
    "cpu_peak_compute_scaling.png",
    "cpu_memory_latency_hierarchy.png",
    "gpu_microbenchmark_suite.png",
    "platform_roofline_measured.png",
]

REAL_PLOT_NAMES = [
    "real_kernels_filip_contrast_map.png",
    "real_kernels_model_validation.png",
    "ai_accel_overview.png",
    "ai_accel_break_even.png",
    "ai_precision_scaling.png",
]

FILIP_VARIANT_PLOT_NAMES = [
    "filip_variant_qss.png",
    "filip_variant_sqs.png",
    "filip_variant_ssq.png",
    "filip_variant_digest.png",
]

FILIP_TUNING_PLOT_NAMES = [
    "filip_autotuning_trace.png",
    "filip_best_summary.png",
    "filip_memory_compute_breakdown.png",
    "filip_best_configuration_card.png",
]

FILIP_EXACT_PLOT_NAMES = [
    "filip_variant_qss.png",
    "filip_variant_sqs.png",
    "filip_variant_ssq.png",
    "filip_variant_digest.png",
    "filip_best_summary.png",
    "filip_memory_compute_breakdown.png",
]

RUN_GROUPS: list[dict[str, Any]] = [
    {
        "id": "benchmarks",
        "label": "Benchmarki platformy",
        "description": "CPU i GPU microbenchmarki, roofline, przepustowosc oraz opoznienia pamieci.",
        "step_ids": ["cpu_benchmark", "gpu_benchmark"],
    },
    {
        "id": "real_kernels",
        "label": "Real kernels",
        "description": "Uproszczone jadra obliczeniowe + testy sciezek akceleracji AI (GPU/runtime AI).",
        "step_ids": ["cpu_real_kernels", "gpu_real_kernels", "ai_accel"],
    },
    {
        "id": "filip_test",
        "label": "Test Filipa",
        "description": "Walidacja FEM oraz kampania aplikacyjna: portable sweep, autotuning i Firefly.",
        "step_ids": ["fem_option_validation", "filip_original_portable", "filip_autotune", "filip_firefly"],
    },
]
RUN_GROUP_INDEX = {group["id"]: group for group in RUN_GROUPS}
OPTIONAL_STEP_IDS = {"gpu_benchmark", "gpu_real_kernels", "filip_exact_reference"}

@dataclass
class JobState:
    lock: threading.Lock = field(default_factory=threading.Lock)
    running: bool = False
    job_id: str = ""
    mode: str = ""
    label: str = ""
    started_at: float = 0.0
    finished_at: float = 0.0
    exit_code: int | None = None
    status: str = "idle"
    log_path: str = ""
    current_step: str = ""
    command: list[str] = field(default_factory=list)
    latest_payload: dict[str, Any] = field(default_factory=dict)
    selected_campaign_dir: str = ""
    stop_requested: bool = False
    active_pid: int | None = None
    active_process: subprocess.Popen[str] | None = field(default=None, repr=False, compare=False)

    def snapshot(self) -> dict[str, Any]:
        with self.lock:
            return {
                "running": self.running,
                "job_id": self.job_id,
                "mode": self.mode,
                "label": self.label,
                "started_at": self.started_at,
                "finished_at": self.finished_at,
                "exit_code": self.exit_code,
                "status": self.status,
                "log_path": self.log_path,
                "current_step": self.current_step,
                "command": list(self.command),
                "latest_payload": self.latest_payload,
                "selected_campaign_dir": self.selected_campaign_dir,
                "stop_requested": self.stop_requested,
                "active_pid": self.active_pid,
            }

JOB = JobState()

UI_PLATFORM_PROFILES = ["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"]
UI_BACKENDS = ["auto", "cpu", "metal", "cuda", "hip", "opencl", "amd", "intel"]
UI_FILIP_CASES = ["portable", "laplace_prism", "test_prism", "prism_pair"]
UI_BENCHMARK_MODES = ["standard", "extended"]
_UI_OPTIONS_CACHE: dict[str, Any] = {"ts": 0.0, "payload": {}}


def _now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def _recommended_platform_profile() -> str:
    system = platform.system()
    if system == "Darwin":
        return "apple"
    try:
        from run_workflow import _backend_available  # type: ignore
    except Exception:
        return "auto"
    if _backend_available("cuda"):
        return "nvidia"
    if _backend_available("hip"):
        return "amd"
    if _backend_available("opencl"):
        return "intel_arc"
    return "auto"


def _write_json(path: Path, payload: dict[str, Any]) -> None:
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False, default=_json_default), encoding="utf-8")


def _safe_slug(text: str) -> str:
    out = []
    for ch in str(text).lower().strip():
        if ch.isalnum():
            out.append(ch)
        elif ch in ("-", "_"):
            out.append(ch)
        else:
            out.append("-")
    cleaned = "".join(out).strip("-")
    while "--" in cleaned:
        cleaned = cleaned.replace("--", "-")
    return cleaned or "step"


def _json_default(value: Any) -> Any:
    if isinstance(value, Path):
        return str(value)
    return value


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _is_within(root: Path, target: Path) -> bool:
    try:
        target.resolve().relative_to(root.resolve())
        return True
    except Exception:
        return False


def _safe_path(raw: str) -> Path | None:
    if not raw:
        return None
    path = Path(raw).expanduser()
    if not path.is_absolute():
        path = (ROOT / path).resolve()
    try:
        resolved = path.resolve()
    except Exception:
        return None
    if _is_within(ROOT, resolved) or _is_within(Path.home(), resolved):
        return resolved
    return None


def _campaign_dirs() -> list[Path]:
    if not THESIS_DIR.exists():
        return []
    return sorted([d for d in THESIS_DIR.iterdir() if d.is_dir() and (d / "summary.json").exists()], key=lambda p: p.stat().st_mtime, reverse=True)


def _ui_capabilities(force: bool = False) -> dict[str, Any]:
    now = time.time()
    if not force and _UI_OPTIONS_CACHE["payload"] and (now - float(_UI_OPTIONS_CACHE["ts"])) < 20.0:
        return dict(_UI_OPTIONS_CACHE["payload"])

    backend_status: dict[str, dict[str, Any]] = {}
    device_choices: list[dict[str, Any]] = [
        {
            "value": "auto",
            "label": "Auto | automatyczny dobór backendu i urządzenia",
            "backend": "auto",
            "device_index": 0,
            "device_name": "",
        }
    ]
    available_backends: list[str] = ["cpu"]

    try:
        from device_catalog import discover_backends  # type: ignore

        discovery = discover_backends([b for b in UI_BACKENDS if b not in {"auto"}])
    except Exception:
        discovery = []

    for result in discovery:
        backend = str(result.backend).strip().lower()
        available = bool(result.available)
        backend_status[backend] = {
            "available": available,
            "error": str(result.error or ""),
            "device_count": len(result.devices or []),
        }
        if available and backend not in available_backends:
            available_backends.append(backend)
        for device in result.devices or []:
            label = str(getattr(device, "label", "") or getattr(device, "device_name", "") or backend).strip()
            device_choices.append(
                {
                    "value": f"{backend}:{int(device.device_index)}",
                    "label": label,
                    "backend": backend,
                    "device_index": int(device.device_index),
                    "device_name": str(device.device_name or ""),
                }
            )

    ordered_available = [b for b in UI_BACKENDS if b == "auto" or b in available_backends]
    backend_choices = []
    for backend in ordered_available:
        if backend == "auto":
            backend_choices.append({"value": "auto", "label": "auto | dobór automatyczny"})
            continue
        status = backend_status.get(backend, {})
        device_count = int(status.get("device_count") or 0)
        suffix = f" ({device_count} urządzeń)" if device_count > 0 else ""
        backend_choices.append({"value": backend, "label": f"{backend}{suffix}"})

    payload = {
        "backend_choices": backend_choices,
        "benchmark_mode_choices": [
            {"value": "standard", "label": "standard | sciezka porownawcza"},
            {"value": "extended", "label": "extended | diagnostyka architektury"},
        ],
        "platform_profile_choices": [
            {"value": item, "label": item + (" | zalecane" if item == _recommended_platform_profile() else "")}
            for item in UI_PLATFORM_PROFILES
        ],
        "filip_case_choices": [{"value": item, "label": item} for item in UI_FILIP_CASES],
        "device_choices": device_choices,
        "backend_status": backend_status,
        "available_backends": available_backends,
        "backend_hint": (
            "Dostępne teraz: " + ", ".join(available_backends)
            if available_backends
            else "Nie wykryto backendów GPU; pozostaje CPU."
        ),
        "recommended_platform_profile": _recommended_platform_profile(),
    }
    try:
        from cpu_utils import detect_cpu_topology  # type: ignore

        topology = detect_cpu_topology()
        payload["cpu_topology"] = topology
        logical = int(topology.get("logical_cpus", 0) or 0)
        if logical <= 0:
            logical = int(os.cpu_count() or 1)
        payload["cpu_thread_limit_max"] = max(1, logical)
    except Exception:
        payload["cpu_topology"] = {}
        payload["cpu_thread_limit_max"] = max(1, int(os.cpu_count() or 1))
    _UI_OPTIONS_CACHE["ts"] = now
    _UI_OPTIONS_CACHE["payload"] = dict(payload)
    return payload


def _latest_campaign_dir() -> Path | None:
    dirs = _campaign_dirs()
    return dirs[0] if dirs else None


def _load_campaign_summary(path: Path | None) -> dict[str, Any] | None:
    if path is None:
        return None
    summary_path = path / "summary.json"
    if not summary_path.exists():
        return None
    try:
        return _read_json(summary_path)
    except Exception:
        return None


def _find_step(summary: dict[str, Any], step_id: str) -> dict[str, Any] | None:
    for step in summary.get("steps", []) or []:
        if str(step.get("id")) == step_id:
            return step
    return None


def _path_exists_str(path_str: str) -> bool:
    return bool(path_str) and Path(path_str).expanduser().exists()


def _step_runtime_images(step: dict[str, Any]) -> list[str]:
    images: list[str] = []
    payload = step.get("payload") or {}
    if isinstance(payload, dict):
        for p in payload.get("figure_set_plots", []) or []:
            if _path_exists_str(str(p)):
                images.append(str(Path(str(p)).resolve()))
        for p in payload.get("figure_appendix_plots", []) or []:
            if _path_exists_str(str(p)):
                images.append(str(Path(str(p)).resolve()))
        for p in payload.get("article_plots", []) or []:
            if _path_exists_str(str(p)):
                images.append(str(Path(str(p)).resolve()))
        figure_set_dir = str(payload.get("figure_set_dir") or "").strip()
        if figure_set_dir:
            for png in sorted(Path(figure_set_dir).glob("*.png")):
                images.append(str(png.resolve()))
        figure_appendix_dir = str(payload.get("figure_appendix_dir") or "").strip()
        if figure_appendix_dir:
            for png in sorted(Path(figure_appendix_dir).glob("*.png")):
                images.append(str(png.resolve()))
        plots_dir = str(payload.get("article_plots_dir") or "").strip()
        if plots_dir:
            for png in sorted(Path(plots_dir).glob("*.png")):
                images.append(str(png.resolve()))
    result_dir = str(step.get("result_dir") or "").strip()
    if result_dir:
        result_path = Path(result_dir)
        candidate_dirs = [result_path / "plots", result_path / "roofline"]
        for directory in candidate_dirs:
            if directory.exists():
                for png in sorted(directory.glob("*.png")):
                    images.append(str(png.resolve()))
        for png in sorted(result_path.glob("*.png")):
            images.append(str(png.resolve()))
    dedup: list[str] = []
    seen: set[str] = set()
    for item in images:
        if item not in seen:
            seen.add(item)
            dedup.append(item)
    return dedup[:18]


def _campaign_view(summary: dict[str, Any] | None) -> dict[str, Any]:
    step_state: dict[str, dict[str, Any]] = {}
    summary_running = bool((summary or {}).get("running"))
    summary_steps_present: set[str] = set()
    exit_code_raw = (summary or {}).get("exit_code")
    summary_exit_ok = False
    if exit_code_raw is not None:
        try:
            summary_exit_ok = int(exit_code_raw) == 0
        except Exception:
            summary_exit_ok = False
    else:
        summary_exit_ok = bool((summary or {}).get("critical_success", False))
    summary_finished_ok = bool(summary) and (not summary_running) and summary_exit_ok
    if summary:
        for step in summary.get("steps", []) or []:
            step_id = str(step.get("id"))
            summary_steps_present.add(step_id)
            raw_status = str(step.get("status", "pending"))
            status = "pending" if (not summary_running and raw_status == "running") else raw_status
            step_state[step_id] = {
                "status": status,
                "elapsed_s": step.get("elapsed_s"),
                "result_dir": step.get("result_dir", ""),
                "log_path": step.get("log_path", ""),
                "reason": step.get("reason", ""),
                "payload": step.get("payload") or {},
                "images": _step_runtime_images(step),
            }

    nodes: list[dict[str, Any]] = []
    for meta in PIPELINE_STEPS:
        step_id = str(meta["id"])
        runtime = step_state.get(step_id, {})
        if not runtime and summary_finished_ok and step_id not in summary_steps_present:
            # Legacy campaign summary produced by an older pipeline version
            # where this step did not exist yet.
            runtime = {
                "status": "skipped",
                "elapsed_s": 0.0,
                "result_dir": "",
                "log_path": "",
                "reason": "Krok nieobecny w summary tej kampanii (legacy pipeline version).",
                "payload": {},
                "images": [],
            }
        nodes.append({
            **meta,
            "status": runtime.get("status", "pending"),
            "elapsed_s": runtime.get("elapsed_s"),
            "result_dir": runtime.get("result_dir", ""),
            "log_path": runtime.get("log_path", ""),
            "reason": runtime.get("reason", ""),
            "payload": runtime.get("payload", {}),
            "images": runtime.get("images", []),
        })
    return {
        "summary": summary or {},
        "nodes": nodes,
        "stages": PIPELINE_STAGES,
        "groups": RUN_GROUPS,
        "global_plots": _global_plot_entries(),
        "plot_sections": _plot_sections(summary),
        "campaigns": _campaign_entries(),
    }


def _campaign_entries() -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for directory in _campaign_dirs()[:20]:
        try:
            summary = _read_json(directory / "summary.json")
        except Exception:
            continue
        entries.append({
            "name": directory.name,
            "path": str(directory.resolve()),
            "timestamp_utc": summary.get("timestamp_utc", ""),
            "critical_success": bool(summary.get("critical_success", False)),
            "exit_code": summary.get("exit_code"),
            "required_failures": summary.get("required_failures", []),
        })
    return entries


def _global_plot_entries() -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    for name in GLOBAL_PLOTS:
        path = ANALYSIS_PLOTS_DIR / name
        if path.exists():
            entries.append({"name": name, "path": str(path.resolve())})
    return entries


def _plot_entries_from_dir(directory: Path, names: list[str]) -> list[dict[str, Any]]:
    entries: list[dict[str, Any]] = []
    seen: set[str] = set()
    for name in names:
        path = directory / name
        if path.exists() and path.is_file():
            resolved = str(path.resolve())
            if resolved not in seen:
                seen.add(resolved)
                entries.append({"name": name, "path": resolved})
    # Add any additional PNG figures not listed in the curated names.
    # This keeps the GUI flexible when new publication figures are added.
    if directory.exists():
        for png in sorted(directory.glob("*.png")):
            resolved = str(png.resolve())
            if resolved in seen:
                continue
            seen.add(resolved)
            entries.append({"name": png.name, "path": resolved})
    return entries


def _latest_filip_plots_dir(summary: dict[str, Any] | None) -> Path | None:
    if summary:
        portable = _find_step(summary, "filip_original_portable") or {}
        result_dir = str(portable.get("result_dir") or "").strip()
        if result_dir:
            plots_dir = Path(result_dir).expanduser().resolve() / "figures" / "thesis_core"
            if plots_dir.exists():
                return plots_dir
    if not OPT_DIR.exists():
        return None
    runs = [p for p in OPT_DIR.iterdir() if p.is_dir() and "__filip_original__backend-" in p.name]
    if not runs:
        return None
    latest = max(runs, key=lambda p: p.stat().st_mtime)
    plots_dir = latest / "figures" / "thesis_core"
    return plots_dir if plots_dir.exists() else None


def _latest_exact_plots_dir(summary: dict[str, Any] | None) -> Path | None:
    if summary:
        exact = _find_step(summary, "filip_exact_reference") or {}
        result_dir = str(exact.get("result_dir") or "").strip()
        if result_dir:
            plots_dir = Path(result_dir).expanduser().resolve() / "figures" / "thesis_core"
            if plots_dir.exists():
                return plots_dir
    if not OPT_DIR.exists():
        return None
    runs = [p for p in OPT_DIR.iterdir() if p.is_dir() and "__exact" in p.name and (p / "summary.json").exists()]
    if not runs:
        return None
    latest = max(runs, key=lambda p: p.stat().st_mtime)
    plots_dir = latest / "figures" / "thesis_core"
    return plots_dir if plots_dir.exists() else None


def _plot_sections(summary: dict[str, Any] | None) -> dict[str, list[dict[str, Any]]]:
    sections: dict[str, list[dict[str, Any]]] = {
        "benchmark": _plot_entries_from_dir(ANALYSIS_PLOTS_DIR, BENCHMARK_PLOT_NAMES),
        "real": _plot_entries_from_dir(ANALYSIS_PLOTS_DIR, REAL_PLOT_NAMES),
        "filip_variants": [],
        "filip_tuning": [],
        "exact": [],
    }
    filip_dir = _latest_filip_plots_dir(summary)
    if filip_dir is not None:
        sections["filip_variants"] = _plot_entries_from_dir(filip_dir, FILIP_VARIANT_PLOT_NAMES)
        sections["filip_tuning"] = _plot_entries_from_dir(filip_dir, FILIP_TUNING_PLOT_NAMES)
        appendix_dir = filip_dir.parent / "appendix"
        if appendix_dir.exists():
            appendix_entries = _plot_entries_from_dir(appendix_dir, FILIP_TUNING_PLOT_NAMES)
            seen = {entry["path"] for entry in sections["filip_tuning"]}
            for entry in appendix_entries:
                if entry["path"] not in seen:
                    sections["filip_tuning"].append(entry)
                    seen.add(entry["path"])
    exact_dir = _latest_exact_plots_dir(summary)
    if exact_dir is not None:
        sections["exact"] = _plot_entries_from_dir(exact_dir, FILIP_EXACT_PLOT_NAMES)
    return sections


def _create_campaign_dir(*, kind: str, profile: str, backend: str) -> Path:
    THESIS_DIR.mkdir(parents=True, exist_ok=True)
    name = f"{_now_tag()}__{kind}__profile-{profile}__backend-{backend}"
    path = THESIS_DIR / name
    (path / "logs").mkdir(parents=True, exist_ok=True)
    (path / "plots").mkdir(parents=True, exist_ok=True)
    (path / "artifacts").mkdir(parents=True, exist_ok=True)
    latest_link = THESIS_DIR / "latest"
    try:
        if latest_link.exists() or latest_link.is_symlink():
            latest_link.unlink()
        latest_link.symlink_to(path.name)
    except Exception:
        try:
            (THESIS_DIR / "latest.txt").write_text(path.name + "\n", encoding="utf-8")
        except Exception:
            pass
    return path


def _compact_payload(payload: dict[str, Any]) -> dict[str, Any]:
    if not isinstance(payload, dict):
        return {}
    keep = [
        "workflow",
        "experiment_class",
        "target",
        "resolved_backend",
        "optimizer",
        "filip_case",
        "filip_mode",
        "execution_mode",
        "session_dir",
        "out_dir",
        "roofline_dir",
        "article_plots_dir",
        "article_plots",
        "plots",
        "best_overall",
        "analysis",
        "validation_summary",
        "comparison_note",
        "numerical_equivalence",
        "replay_dump_root",
        "replay_dump_root_source",
        "error",
        "exit_code",
    ]
    return {key: payload.get(key) for key in keep if key in payload}


def _step_summary(step: dict[str, Any]) -> dict[str, Any]:
    return {
        "id": step["id"],
        "label": step["label"],
        "status": step["status"],
        "elapsed_s": step.get("elapsed_s"),
        "result_dir": step.get("result_dir"),
        "log_path": step.get("log_path"),
        "reason": step.get("reason", ""),
        "payload": _compact_payload(step.get("payload") or {}),
    }


def _live_campaign_summary(
    *,
    campaign_dir: Path,
    workflow: str,
    experiment_class: str,
    profile: str,
    requested_backend: str,
    benchmark_mode: str,
    platform_profile: str,
    device_index: int,
    group_id: str,
    group_label: str,
    steps: list[dict[str, Any]],
    running: bool,
) -> dict[str, Any]:
    key_results = {
        str(step.get("id", "")): str(step.get("result_dir", "") or "").strip()
        for step in steps
        if str(step.get("result_dir", "") or "").strip()
    }
    required_failures = [
        str(step.get("id", ""))
        for step in steps
        if bool(step.get("required")) and str(step.get("status", "")) == "failed"
    ]
    current_step = next((str(step.get("id", "")) for step in steps if str(step.get("status", "")) == "running"), "")
    exit_code = None if running else (0 if not required_failures else 1)
    return {
        "workflow": workflow,
        "experiment_class": experiment_class,
        "timestamp_utc": datetime.utcnow().isoformat() + "Z",
        "profile": profile,
        "requested_backend": requested_backend,
        "benchmark_mode": benchmark_mode,
        "platform_profile": platform_profile,
        "device_index": device_index,
        "campaign_dir": str(campaign_dir),
        "running": bool(running),
        "current_step": current_step,
        "group_id": group_id,
        "group_label": group_label,
        "steps": [_step_summary(step) for step in steps],
        "key_results": key_results,
        "required_failures": required_failures,
        "critical_success": (len(required_failures) == 0) if not running else False,
        "exit_code": exit_code,
    }


def _write_campaign_state(campaign_dir: Path, summary: dict[str, Any]) -> None:
    _write_json(campaign_dir / "summary.json", summary)
    _write_json(campaign_dir / "steps.json", {"steps": summary.get("steps", [])})
    lines = [
        f"# {summary.get('group_label') or summary.get('workflow')}",
        "",
        f"- Workflow: `{summary.get('workflow', '')}`",
        f"- Profil: `{summary.get('profile', '')}`",
        f"- Backend zadany: `{summary.get('requested_backend', '')}`",
        f"- Tryb benchmarkow: `{summary.get('benchmark_mode', '')}`",
        f"- Platform profile: `{summary.get('platform_profile', '')}`",
        f"- Device index: `{summary.get('device_index', '')}`",
        f"- Running: `{summary.get('running', False)}`",
        f"- Exit code: `{summary.get('exit_code')}`",
        "",
        "## Kroki",
        "",
    ]
    for step in summary.get("steps", []) or []:
        lines.append(f"- `{step.get('id', '')}`: `{step.get('status', '')}` ({step.get('elapsed_s')})")
    (campaign_dir / "campaign.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def _tail_file(path: Path, max_lines: int = 220) -> str:
    if not path.exists():
        return ""
    try:
        lines = path.read_text(encoding="utf-8", errors="replace").splitlines()
    except Exception:
        return ""
    return "\n".join(lines[-max_lines:])


def _extract_payload_from_text(text: str) -> dict[str, Any]:
    marker_idx = text.rfind(MARKER)
    if marker_idx < 0:
        return {}
    tail = text[marker_idx + len(MARKER):].strip()
    start = tail.find("{")
    end = tail.rfind("}")
    if start < 0 or end < start:
        return {}
    try:
        payload = json.loads(tail[start : end + 1])
    except Exception:
        return {}
    return payload if isinstance(payload, dict) else {}


def _build_base_args(config: dict[str, Any]) -> list[str]:
    return [
        "--profile", "full",
        "--platform-profile", str(config.get("platform_profile") or "auto"),
        "--arch", str(config.get("arch") or "auto"),
        "--backend", str(config.get("backend") or "auto"),
        "--benchmark-mode", str(config.get("benchmark_mode") or "standard"),
        "--benchmarks-max-cpu-threads", str(int(config.get("benchmarks_max_cpu_threads") or 0)),
        "--real-kernels-max-cpu-threads", str(int(config.get("real_kernels_max_cpu_threads") or 0)),
        "--filip-max-cpu-threads", str(int(config.get("filip_max_cpu_threads") or 0)),
        "--device-index", str(int(config.get("device_index") or 0)),
        "--roofline-ai", str(float(config.get("roofline_ai") or 8.0)),
        "--roofline-bytes", str(float(config.get("roofline_bytes") or 1_000_000_000.0)),
    ]


def _platform_is_macos() -> bool:
    return platform.system() == "Darwin"


def _gpu_backend_available(config: dict[str, Any]) -> bool:
    try:
        from run_workflow import _backend_available, _resolve_gpu_backend  # type: ignore
    except Exception:
        return True
    backend = str(config.get("backend") or "auto")
    if backend.strip().lower() == "cpu":
        return False
    platform_profile = str(config.get("platform_profile") or "auto")
    arch = str(config.get("arch") or "auto")
    resolved = _resolve_gpu_backend(backend, platform_profile, arch)
    return bool(resolved) and _backend_available(resolved)


def _latest_existing_dir(*candidates: str) -> str:
    for candidate in candidates:
        if candidate and Path(candidate).expanduser().exists():
            return str(Path(candidate).expanduser().resolve())
    return ""


def _step_command(step_id: str, config: dict[str, Any], selected_campaign_dir: str = "") -> tuple[list[str] | None, str]:
    base = _build_base_args(config)
    run_workflow = [PYTHON, str(ROOT / "run_workflow.py")]
    replay_root = str(config.get("replay_dump_root") or "").strip()
    filip_case = str(config.get("filip_case") or "prism_pair")
    repeats = str(int(config.get("repeats") or 5))
    trials = str(int(config.get("trials") or 256))
    population = str(int(config.get("population") or 24))
    iterations = str(int(config.get("iterations") or 40))
    validation_ops = str(config.get("validation_operators") or "laplace,test")
    validation_variants = str(config.get("validation_variants") or "qss,sqs,ssq")
    validation_n_elements = str(int(config.get("validation_n_elements") or 16384))
    validation_n_qp = str(int(config.get("validation_n_qp") or 6))
    validation_wg = str(int(config.get("validation_workgroup_size") or 64))

    if step_id == "cpu_benchmark":
        return run_workflow + ["--workflow", "cpu_benchmark", *base], ""
    if step_id == "gpu_benchmark":
        if not _gpu_backend_available(config):
            return None, "Brak dostepnego backendu GPU dla wybranej konfiguracji."
        return run_workflow + ["--workflow", "gpu_benchmark", *base], ""
    if step_id == "cpu_real_kernels":
        return run_workflow + ["--workflow", "cpu_real_kernels", *base, "--real-runs", str(int(config.get("real_runs") or 5))], ""
    if step_id == "gpu_real_kernels":
        if not _gpu_backend_available(config):
            return None, "Brak dostepnego backendu GPU dla wybranej konfiguracji."
        return run_workflow + ["--workflow", "gpu_real_kernels", *base, "--real-runs", str(int(config.get("real_runs") or 5))], ""
    if step_id == "ai_accel":
        return run_workflow + ["--workflow", "ai_accel", *base, "--real-runs", str(int(config.get("real_runs") or 5))], ""
    if step_id == "fem_option_validation":
        return run_workflow + [
            "--workflow", "fem_option_validation", *base,
            "--repeats", repeats,
            "--fem-option-validation-operators", validation_ops,
            "--fem-option-validation-variants", validation_variants,
            "--fem-option-validation-n-elements", validation_n_elements,
            "--fem-option-validation-n-qp", validation_n_qp,
            "--fem-option-validation-workgroup-size", validation_wg,
        ], ""
    if step_id == "filip_original_portable":
        return run_workflow + [
            "--workflow", "filip_original", *base,
            "--repeats", repeats,
            "--filip-case", filip_case,
            "--filip-mode", "portable_sweep",
        ], ""
    if step_id == "filip_autotune":
        return run_workflow + [
            "--workflow", "filip_autotune", *base,
            "--repeats", repeats,
            "--trials", trials,
        ], ""
    if step_id == "filip_firefly":
        return run_workflow + [
            "--workflow", "filip_firefly", *base,
            "--repeats", repeats,
            "--population", population,
            "--iterations", iterations,
        ], ""
    if step_id == "filip_exact_reference":
        cmd = run_workflow + [
            "--workflow", "filip_original", *base,
            "--filip-case", filip_case,
            "--filip-mode", "exact_reference",
        ]
        if _platform_is_macos():
            cmd += ["--backend", "metal"]
            if replay_root:
                cmd += ["--filip-replay-dump-root", replay_root]
        else:
            cmd += [
                "--backend", "opencl",
                "--repeats", repeats,
                "--filip-dump-launch-artifacts",
                "--filip-export-replay-inputs",
                "--filip-export-replay-include-expected-output",
                "--filip-export-canonical-replay-bundles",
            ]
        return cmd, ""
    if step_id == "profiler_correlation":
        summary = _load_campaign_summary(_safe_path(selected_campaign_dir) if selected_campaign_dir else _latest_campaign_dir())
        if not summary:
            return None, "Brak zakonczonej kampanii pelnej, z ktorej mozna pobrac optimization_dir i validation_dir."
        val_step = _find_step(summary, "fem_option_validation") or {}
        exact_step = _find_step(summary, "filip_exact_reference") or {}
        portable_step = _find_step(summary, "filip_original_portable") or {}
        optimization_dir = _latest_existing_dir(str(exact_step.get("result_dir") or ""), str(portable_step.get("result_dir") or ""))
        validation_dir = _latest_existing_dir(str(val_step.get("result_dir") or ""))
        if not optimization_dir or not validation_dir:
            return None, "Brak optimization_dir lub validation_dir do korelacji profilerowej."
        cmd = run_workflow + [
            "--workflow", "profiler_correlation",
            "--correlation-optimization-dir", optimization_dir,
            "--correlation-fem-option-validation-dir", validation_dir,
            "--correlation-out-dir", str((Path(optimization_dir) / "profiler_correlation").resolve()),
        ]
        reports_raw = str(config.get("correlation_profiler_reports") or "").strip()
        if reports_raw:
            for line in reports_raw.splitlines():
                report = line.strip()
                if report:
                    cmd += ["--correlation-profiler-report", report]
        return cmd, ""
    return None, f"Nieznany krok: {step_id}"


def _make_ui_log_dir() -> Path:
    path = ROOT / "data" / "web_pipeline"
    path.mkdir(parents=True, exist_ok=True)
    return path


def _write_job_log_header(handle, title: str, cmd: list[str]) -> None:
    handle.write(f"# {title}\n")
    handle.write(f"# started: {datetime.now().isoformat()}\n")
    handle.write("$ " + " ".join(cmd) + "\n\n")
    handle.flush()


def _register_active_process(proc: subprocess.Popen[str] | None) -> None:
    with JOB.lock:
        JOB.active_process = proc
        JOB.active_pid = int(proc.pid) if proc is not None else None


def _terminate_process(proc: subprocess.Popen[str], *, force: bool = False) -> None:
    try:
        if platform.system() == "Windows":
            if force:
                proc.kill()
            else:
                proc.terminate()
            return
        sig = signal.SIGKILL if force else signal.SIGTERM
        try:
            os.killpg(proc.pid, sig)
        except Exception:
            if force:
                proc.kill()
            else:
                proc.terminate()
    except Exception:
        pass


def _stop_active_job(reason: str = "manual_stop", wait_s: float = 2.0) -> tuple[bool, str]:
    with JOB.lock:
        if not JOB.running:
            return False, "Brak aktywnej kampanii do przerwania."
        JOB.stop_requested = True
        JOB.status = "stopping"
        proc = JOB.active_process

    if proc is None:
        return True, "Wyslano zadanie zatrzymania; proces glowny jest pomiedzy krokami."

    _terminate_process(proc, force=False)
    try:
        proc.wait(timeout=max(0.2, float(wait_s)))
    except Exception:
        _terminate_process(proc, force=True)

    with JOB.lock:
        JOB.latest_payload = {
            "error": f"Przerwano przez uzytkownika ({reason}).",
            **(JOB.latest_payload or {}),
        }
    return True, "Przerwano aktywna kampanie."


def _run_process(cmd: list[str], *, log_path: Path, label: str) -> tuple[int, dict[str, Any]]:
    tail: list[str] = []
    env = dict(os.environ)
    env["PYTHONUNBUFFERED"] = "1"
    rc = 1
    try:
        with log_path.open("w", encoding="utf-8") as handle:
            _write_job_log_header(handle, label, cmd)
            popen_kwargs: dict[str, Any] = {
                "cwd": ROOT,
                "env": env,
                "stdout": subprocess.PIPE,
                "stderr": subprocess.STDOUT,
                "text": True,
                "bufsize": 1,
            }
            if platform.system() == "Windows":
                flags = int(getattr(subprocess, "CREATE_NEW_PROCESS_GROUP", 0))
                if flags:
                    popen_kwargs["creationflags"] = flags
            else:
                popen_kwargs["start_new_session"] = True

            proc = subprocess.Popen(cmd, **popen_kwargs)
            _register_active_process(proc)
            assert proc.stdout is not None
            for line in proc.stdout:
                handle.write(line)
                handle.flush()
                tail.append(line)
                if len(tail) > 4000:
                    tail = tail[-4000:]
            rc = int(proc.wait())
    except Exception as exc:
        tail.append(f"[pipeline-web] _run_process exception: {type(exc).__name__}: {exc}\n")
        rc = 1
    finally:
        _register_active_process(None)
    payload = _extract_payload_from_text("".join(tail))
    return rc, payload


def _run_full_pipeline_job(config: dict[str, Any]) -> None:
    ui_log_dir = _make_ui_log_dir()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = ui_log_dir / f"{ts}__full_pipeline.log"
    cmd = [PYTHON, str(ROOT / "run_workflow.py"), "--workflow", "full_thesis_pipeline"]
    cmd += [
        "--platform-profile", str(config.get("platform_profile") or "auto"),
        "--arch", str(config.get("arch") or "auto"),
        "--backend", str(config.get("backend") or "auto"),
        "--benchmark-mode", str(config.get("benchmark_mode") or "standard"),
        "--benchmarks-max-cpu-threads", str(int(config.get("benchmarks_max_cpu_threads") or 0)),
        "--real-kernels-max-cpu-threads", str(int(config.get("real_kernels_max_cpu_threads") or 0)),
        "--filip-max-cpu-threads", str(int(config.get("filip_max_cpu_threads") or 0)),
        "--device-index", str(int(config.get("device_index") or 0)),
        "--filip-case", str(config.get("filip_case") or "prism_pair"),
    ]
    replay_root = str(config.get("replay_dump_root") or "").strip()
    if replay_root:
        cmd += ["--filip-replay-dump-root", replay_root]
    with JOB.lock:
        JOB.running = True
        JOB.job_id = ts
        JOB.mode = "full"
        JOB.label = "Pelna kampania Final"
        JOB.started_at = time.time()
        JOB.finished_at = 0.0
        JOB.exit_code = None
        JOB.status = "running"
        JOB.log_path = str(log_path)
        JOB.current_step = "full_thesis_pipeline"
        JOB.command = cmd
        JOB.latest_payload = {}
        JOB.stop_requested = False
        JOB.active_pid = None
        JOB.active_process = None
    rc, payload = _run_process(cmd, log_path=log_path, label="Pelna kampania Final")
    latest = _latest_campaign_dir()
    with JOB.lock:
        JOB.running = False
        JOB.finished_at = time.time()
        JOB.exit_code = rc
        if JOB.stop_requested:
            JOB.status = "failed"
            payload = {"error": "Przerwano przez uzytkownika.", **(payload or {})}
        else:
            JOB.status = "ok" if rc == 0 else "failed"
        JOB.current_step = ""
        JOB.latest_payload = payload
        JOB.selected_campaign_dir = str(latest.resolve()) if latest else ""
        JOB.stop_requested = False
        JOB.active_pid = None
        JOB.active_process = None


def _run_group_pipeline_job(group_id: str, config: dict[str, Any], selected_campaign_dir: str = "") -> None:
    group = RUN_GROUP_INDEX[group_id]
    profile = "full"
    requested_backend = str(config.get("backend") or "auto")
    campaign_dir = _create_campaign_dir(kind=f"suite-{group_id}", profile=profile, backend=requested_backend)
    selected_steps = set(group.get("step_ids", []))
    steps: list[dict[str, Any]] = []
    blocked_reason = ""

    def _update(running: bool = True) -> None:
        summary = _live_campaign_summary(
            campaign_dir=campaign_dir,
            workflow="group_pipeline",
            experiment_class="thesis_group_campaign",
            profile=profile,
            requested_backend=requested_backend,
            benchmark_mode=str(config.get("benchmark_mode") or "standard"),
            platform_profile=str(config.get("platform_profile") or "auto"),
            device_index=int(config.get("device_index") or 0),
            group_id=group_id,
            group_label=str(group.get("label") or group_id),
            steps=steps,
            running=running,
        )
        _write_campaign_state(campaign_dir, summary)

    with JOB.lock:
        JOB.running = True
        JOB.job_id = _now_tag()
        JOB.mode = "group"
        JOB.label = str(group.get("label") or group_id)
        JOB.started_at = time.time()
        JOB.finished_at = 0.0
        JOB.exit_code = None
        JOB.status = "running"
        JOB.log_path = str(campaign_dir / "logs")
        JOB.current_step = ""
        JOB.command = []
        JOB.latest_payload = {}
        JOB.selected_campaign_dir = str(campaign_dir.resolve())
        JOB.stop_requested = False
        JOB.active_pid = None
        JOB.active_process = None

    for meta in PIPELINE_STEPS:
        step_id = str(meta["id"])
        with JOB.lock:
            user_stop_requested = bool(JOB.stop_requested)
        if user_stop_requested and not blocked_reason:
            blocked_reason = "Kampania zostala przerwana przez uzytkownika."

        if step_id not in selected_steps:
            steps.append({
                "id": step_id,
                "label": meta["label"],
                "required": False,
                "status": "skipped",
                "elapsed_s": 0.0,
                "reason": "Krok nie nalezy do wybranego pakietu uruchomieniowego.",
                "payload": {},
            })
            _update(running=True)
            continue

        if blocked_reason:
            steps.append({
                "id": step_id,
                "label": meta["label"],
                "required": step_id not in OPTIONAL_STEP_IDS,
                "status": "skipped",
                "elapsed_s": 0.0,
                "reason": blocked_reason,
                "payload": {},
            })
            _update(running=True)
            continue

        cmd, reason = _step_command(step_id, config, selected_campaign_dir)
        step: dict[str, Any] = {
            "id": step_id,
            "label": meta["label"],
            "required": step_id not in OPTIONAL_STEP_IDS,
            "command": cmd or [],
            "reason": reason,
        }
        if cmd is None:
            step["status"] = "skipped"
            step["elapsed_s"] = 0.0
            steps.append(step)
            _update(running=True)
            continue

        log_path = campaign_dir / "logs" / f"{_safe_slug(step_id)}.log"
        step["status"] = "running"
        step["elapsed_s"] = 0.0
        step["log_path"] = str(log_path)
        steps.append(step)
        with JOB.lock:
            JOB.current_step = step_id
            JOB.log_path = str(log_path)
            JOB.command = cmd
        _update(running=True)

        started = time.time()
        rc, payload = _run_process(cmd, log_path=log_path, label=f"Pakiet {group_id}: {step_id}")
        elapsed = time.time() - started
        step["status"] = "ok" if rc == 0 else "failed"
        step["elapsed_s"] = elapsed
        step["exit_code"] = int(rc)
        step["payload"] = payload
        result_dir = ""
        if isinstance(payload, dict):
            result_dir = str(payload.get("out_dir") or payload.get("session_dir") or "").strip()
        if result_dir:
            step["result_dir"] = result_dir
        _update(running=True)
        if rc != 0 and step["required"]:
            blocked_reason = f"Kampania pakietowa zostala przerwana po bledzie kroku: {step_id}."

    _update(running=False)
    latest_summary = _load_campaign_summary(campaign_dir) or {}
    with JOB.lock:
        JOB.running = False
        JOB.finished_at = time.time()
        if JOB.stop_requested:
            JOB.exit_code = 130
            JOB.status = "failed"
            latest_summary = {**latest_summary, "error": "Przerwano przez uzytkownika."}
        else:
            JOB.exit_code = int(latest_summary.get("exit_code") or 0)
            JOB.status = "ok" if JOB.exit_code == 0 else "failed"
        JOB.current_step = ""
        JOB.latest_payload = latest_summary
        JOB.selected_campaign_dir = str(campaign_dir.resolve())
        JOB.stop_requested = False
        JOB.active_pid = None
        JOB.active_process = None


def _run_single_step_job(step_id: str, config: dict[str, Any], selected_campaign_dir: str = "") -> None:
    ui_log_dir = _make_ui_log_dir()
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    log_path = ui_log_dir / f"{ts}__{step_id}.log"
    cmd, reason = _step_command(step_id, config, selected_campaign_dir)
    with JOB.lock:
        JOB.running = True
        JOB.job_id = ts
        JOB.mode = "step"
        JOB.label = STEP_INDEX.get(step_id, {}).get("label", step_id)
        JOB.started_at = time.time()
        JOB.finished_at = 0.0
        JOB.exit_code = None
        JOB.status = "running"
        JOB.log_path = str(log_path)
        JOB.current_step = step_id
        JOB.command = cmd or []
        JOB.latest_payload = {}
        JOB.selected_campaign_dir = selected_campaign_dir
        JOB.stop_requested = False
        JOB.active_pid = None
        JOB.active_process = None
    if cmd is None:
        log_path.write_text(reason + "\n", encoding="utf-8")
        with JOB.lock:
            JOB.running = False
            JOB.finished_at = time.time()
            JOB.exit_code = 1
            JOB.status = "failed"
            JOB.latest_payload = {"error": reason}
            JOB.current_step = ""
            JOB.stop_requested = False
            JOB.active_pid = None
            JOB.active_process = None
        return
    rc, payload = _run_process(cmd, log_path=log_path, label=f"Krok: {step_id}")
    with JOB.lock:
        JOB.running = False
        JOB.finished_at = time.time()
        JOB.exit_code = rc
        if JOB.stop_requested:
            JOB.status = "failed"
            payload = {"error": "Przerwano przez uzytkownika.", **(payload or {})}
        else:
            JOB.status = "ok" if rc == 0 else "failed"
        JOB.current_step = ""
        JOB.latest_payload = payload
        JOB.stop_requested = False
        JOB.active_pid = None
        JOB.active_process = None


def _start_job(target, *args) -> tuple[bool, str]:
    with JOB.lock:
        if JOB.running:
            return False, "Inna kampania juz trwa. Poczekaj na zakonczenie albo odswiez status."
    def guarded_target() -> None:
        try:
            target(*args)
        except Exception as exc:
            with JOB.lock:
                JOB.running = False
                JOB.finished_at = time.time()
                JOB.exit_code = 1
                JOB.status = "failed"
                JOB.current_step = ""
                JOB.latest_payload = {"error": f"Wyjatek zadania: {type(exc).__name__}: {exc}"}
                JOB.stop_requested = False
                JOB.active_pid = None
                JOB.active_process = None

    thread = threading.Thread(target=guarded_target, daemon=True)
    thread.start()
    return True, "OK"


def _open_path(path: Path) -> tuple[bool, str]:
    if not path.exists():
        return False, f"Sciezka nie istnieje: {path}"
    try:
        if platform.system() == "Darwin":
            subprocess.run(["open", str(path)], check=False)
        elif platform.system() == "Windows":
            os.startfile(str(path))  # type: ignore[attr-defined]
        else:
            subprocess.run(["xdg-open", str(path)], check=False)
        return True, "OK"
    except Exception as exc:
        return False, str(exc)


class Handler(SimpleHTTPRequestHandler):
    def __init__(self, *args, **kwargs):
        super().__init__(*args, directory=str(STATIC_DIR), **kwargs)

    def log_message(self, format: str, *args) -> None:
        sys.stdout.write("[web] " + (format % args) + "\n")

    def _send_json(self, payload: Any, status: int = 200) -> None:
        body = json.dumps(payload, ensure_ascii=False, indent=2, default=_json_default).encode("utf-8")
        self.send_response(status)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.end_headers()
        self.wfile.write(body)

    def _read_json_body(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0") or 0)
        if length <= 0:
            return {}
        raw = self.rfile.read(length)
        if not raw:
            return {}
        try:
            payload = json.loads(raw.decode("utf-8"))
        except Exception:
            return {}
        return payload if isinstance(payload, dict) else {}

    def do_GET(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/state":
            self._handle_api_state(parsed)
            return
        if parsed.path == "/api/campaign":
            self._handle_api_campaign(parsed)
            return
        if parsed.path == "/api/log":
            self._handle_api_log(parsed)
            return
        if parsed.path == "/api/image":
            self._handle_api_image(parsed)
            return
        return super().do_GET()

    def do_POST(self) -> None:
        parsed = urlparse(self.path)
        if parsed.path == "/api/run/full":
            self._handle_run_full()
            return
        if parsed.path == "/api/run/group":
            self._handle_run_group()
            return
        if parsed.path == "/api/run/step":
            self._handle_run_step()
            return
        if parsed.path == "/api/open":
            self._handle_open()
            return
        if parsed.path == "/api/refresh-plots":
            self._handle_refresh_plots()
            return
        if parsed.path == "/api/build-plots-zip":
            self._handle_build_plots_zip()
            return
        if parsed.path == "/api/stop":
            self._handle_stop_job()
            return
        self._send_json({"error": "Unknown endpoint"}, status=404)

    def _handle_api_state(self, parsed) -> None:
        query = parse_qs(parsed.query)
        requested = query.get("campaign_dir", [""])[0]
        campaign_dir = _safe_path(requested) if requested else _latest_campaign_dir()
        summary = _load_campaign_summary(campaign_dir)
        payload = {
            "root": str(ROOT),
            "latest_campaign_dir": str(campaign_dir.resolve()) if campaign_dir else "",
            "campaign": _campaign_view(summary),
            "job": JOB.snapshot(),
            "available": _ui_capabilities(),
            "defaults": {
                "platform_profile": "auto",
                "arch": "auto",
                "backend": "auto",
                "benchmark_mode": "standard",
                "benchmarks_max_cpu_threads": int((_ui_capabilities().get("cpu_thread_limit_max") or 1)),
                "real_kernels_max_cpu_threads": int((_ui_capabilities().get("cpu_thread_limit_max") or 1)),
                "filip_max_cpu_threads": int((_ui_capabilities().get("cpu_thread_limit_max") or 1)),
                "device_index": 0,
                "filip_case": "prism_pair",
                "replay_dump_root": "",
                "real_runs": 5,
                "repeats": 5,
                "trials": 256,
                "population": 24,
                "iterations": 40,
                "validation_operators": "laplace,test",
                "validation_variants": "qss,sqs,ssq",
                "validation_n_elements": 16384,
                "validation_n_qp": 6,
                "validation_workgroup_size": 64,
                "correlation_profiler_reports": "",
            },
        }
        self._send_json(payload)

    def _handle_api_campaign(self, parsed) -> None:
        query = parse_qs(parsed.query)
        requested = query.get("dir", [""])[0]
        campaign_dir = _safe_path(requested) if requested else _latest_campaign_dir()
        summary = _load_campaign_summary(campaign_dir)
        if not summary:
            self._send_json({"error": "Nie znaleziono summary.json dla wskazanej kampanii."}, status=404)
            return
        self._send_json({
            "campaign_dir": str(campaign_dir.resolve()) if campaign_dir else "",
            "campaign": _campaign_view(summary),
        })

    def _handle_api_log(self, parsed) -> None:
        query = parse_qs(parsed.query)
        requested = query.get("path", [""])[0]
        path = _safe_path(requested)
        if path is None:
            self._send_json({"error": "Niepoprawna sciezka logu."}, status=400)
            return
        self._send_json({"path": str(path), "text": _tail_file(path)})

    def _handle_api_image(self, parsed) -> None:
        query = parse_qs(parsed.query)
        requested = query.get("path", [""])[0]
        path = _safe_path(requested)
        if path is None or not path.exists() or not path.is_file():
            self.send_error(404, "Image not found")
            return
        ctype = mimetypes.guess_type(str(path))[0] or "application/octet-stream"
        data = path.read_bytes()
        self.send_response(200)
        self.send_header("Content-Type", ctype)
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def _handle_run_full(self) -> None:
        payload = self._read_json_body()
        config = payload.get("config") if isinstance(payload.get("config"), dict) else {}
        ok, msg = _start_job(_run_full_pipeline_job, config)
        if not ok:
            self._send_json({"error": msg}, status=409)
            return
        self._send_json({"ok": True, "message": "Uruchomiono pelna kampanie."})

    def _handle_run_group(self) -> None:
        payload = self._read_json_body()
        group_id = str(payload.get("group_id") or "").strip()
        if group_id not in RUN_GROUP_INDEX:
            self._send_json({"error": f"Nieznany pakiet: {group_id}"}, status=400)
            return
        config = payload.get("config") if isinstance(payload.get("config"), dict) else {}
        selected_campaign_dir = str(payload.get("campaign_dir") or "").strip()
        ok, msg = _start_job(_run_group_pipeline_job, group_id, config, selected_campaign_dir)
        if not ok:
            self._send_json({"error": msg}, status=409)
            return
        self._send_json({"ok": True, "message": f"Uruchomiono pakiet: {group_id}"})

    def _handle_run_step(self) -> None:
        payload = self._read_json_body()
        step_id = str(payload.get("step_id") or "").strip()
        if step_id not in STEP_INDEX:
            self._send_json({"error": f"Nieznany krok: {step_id}"}, status=400)
            return
        config = payload.get("config") if isinstance(payload.get("config"), dict) else {}
        selected_campaign_dir = str(payload.get("campaign_dir") or "").strip()
        ok, msg = _start_job(_run_single_step_job, step_id, config, selected_campaign_dir)
        if not ok:
            self._send_json({"error": msg}, status=409)
            return
        self._send_json({"ok": True, "message": f"Uruchomiono krok: {step_id}"})

    def _handle_open(self) -> None:
        payload = self._read_json_body()
        path = _safe_path(str(payload.get("path") or "").strip())
        if path is None:
            self._send_json({"error": "Niepoprawna sciezka."}, status=400)
            return
        ok, msg = _open_path(path)
        if not ok:
            self._send_json({"error": msg}, status=400)
            return
        self._send_json({"ok": True})

    def _handle_stop_job(self) -> None:
        payload = self._read_json_body()
        reason = str(payload.get("reason") or "api_stop").strip() or "api_stop"
        ok, msg = _stop_active_job(reason=reason, wait_s=2.5)
        if not ok:
            self._send_json({"error": msg}, status=409)
            return
        self._send_json({"ok": True, "message": msg})

    def _handle_refresh_plots(self) -> None:
        payload = self._read_json_body()
        mode = str(payload.get("mode") or "session").strip()
        selected_campaign_dir = str(payload.get("campaign_dir") or "").strip()
        config = payload.get("config") if isinstance(payload.get("config"), dict) else {}
        with JOB.lock:
            if JOB.running:
                self._send_json({"error": "Inna kampania juz trwa."}, status=409)
                return
        if mode == "session":
            cmd_main = [PYTHON, str(ROOT / "analysis" / "generate_plots.py"), "--no-clean"]
            cmd_ai = [
                PYTHON,
                str(ROOT / "analysis" / "generate_ai_accel_plots.py"),
                "--scope",
                "session",
                "--session",
                "latest",
            ]
            label = "Pakiet wykresow zbiorczych"
            synthetic_step = "session_plot_bundle"
            # Reuse single-step runner style via a tiny wrapper below.
            def runner():
                ui_log_dir = _make_ui_log_dir()
                ts = datetime.now().strftime("%Y%m%d_%H%M%S")
                log_path = ui_log_dir / f"{ts}__session_plots.log"
                with JOB.lock:
                    JOB.running = True
                    JOB.job_id = ts
                    JOB.mode = "utility"
                    JOB.label = label
                    JOB.started_at = time.time()
                    JOB.finished_at = 0.0
                    JOB.exit_code = None
                    JOB.status = "running"
                    JOB.log_path = str(log_path)
                    JOB.current_step = synthetic_step
                    JOB.command = cmd_main + ["&&"] + cmd_ai
                    JOB.latest_payload = {}
                    JOB.selected_campaign_dir = selected_campaign_dir
                    JOB.stop_requested = False
                    JOB.active_pid = None
                    JOB.active_process = None
                rc_main, payload_main = _run_process(cmd_main, log_path=log_path, label=label)
                rc_ai = 0
                payload_ai: dict[str, Any] = {}
                if rc_main == 0:
                    rc_ai, payload_ai = _run_process(
                        cmd_ai,
                        log_path=log_path,
                        label=f"{label} (AI)",
                    )
                payload_inner: dict[str, Any] = {
                    "session_plots": payload_main,
                    "ai_plots": payload_ai,
                    "analysis": {
                        "generate_plots": int(rc_main),
                        "generate_ai_accel_plots": int(rc_ai if rc_main == 0 else 1),
                    },
                }
                rc = 0 if rc_main == 0 and rc_ai == 0 else 1
                with JOB.lock:
                    JOB.running = False
                    JOB.finished_at = time.time()
                    JOB.exit_code = rc
                    if JOB.stop_requested:
                        JOB.status = "failed"
                        payload_inner = {"error": "Przerwano przez uzytkownika.", **payload_inner}
                    else:
                        JOB.status = "ok" if rc == 0 else "failed"
                    JOB.current_step = ""
                    JOB.latest_payload = payload_inner
                    JOB.stop_requested = False
                    JOB.active_pid = None
                    JOB.active_process = None
            ok, msg = _start_job(runner)
        else:
            summary = _load_campaign_summary(_safe_path(selected_campaign_dir) if selected_campaign_dir else _latest_campaign_dir())
            if not summary:
                self._send_json({"error": "Brak kampanii do odswiezenia wykresow Filipa."}, status=400)
                return
            portable = _find_step(summary, "filip_original_portable") or {}
            optimization_dir = str(portable.get("result_dir") or "").strip()
            if not optimization_dir:
                self._send_json({"error": "Brak katalogu filip_original_portable w wybranej kampanii."}, status=400)
                return
            cmd = [PYTHON, str(ROOT / "analysis" / "filip_article_plots.py"), "--optimization-dir", optimization_dir]
            label = "Odswiezenie wykresow Filipa"
            def runner2():
                ui_log_dir = _make_ui_log_dir()
                ts = datetime.now().strftime("%Y%m%d_%H%M%S")
                log_path = ui_log_dir / f"{ts}__filip_plots.log"
                with JOB.lock:
                    JOB.running = True
                    JOB.job_id = ts
                    JOB.mode = "utility"
                    JOB.label = label
                    JOB.started_at = time.time()
                    JOB.finished_at = 0.0
                    JOB.exit_code = None
                    JOB.status = "running"
                    JOB.log_path = str(log_path)
                    JOB.current_step = "filip_plot_bundle"
                    JOB.command = cmd
                    JOB.latest_payload = {}
                    JOB.selected_campaign_dir = selected_campaign_dir
                    JOB.stop_requested = False
                    JOB.active_pid = None
                    JOB.active_process = None
                rc, payload_inner = _run_process(cmd, log_path=log_path, label=label)
                with JOB.lock:
                    JOB.running = False
                    JOB.finished_at = time.time()
                    JOB.exit_code = rc
                    if JOB.stop_requested:
                        JOB.status = "failed"
                        payload_inner = {"error": "Przerwano przez uzytkownika.", **(payload_inner or {})}
                    else:
                        JOB.status = "ok" if rc == 0 else "failed"
                    JOB.current_step = ""
                    JOB.latest_payload = payload_inner
                    JOB.stop_requested = False
                    JOB.active_pid = None
                    JOB.active_process = None
            ok, msg = _start_job(runner2)
        if not ok:
            self._send_json({"error": msg}, status=409)
            return
        self._send_json({"ok": True, "message": "Uruchomiono odswiezenie wykresow."})

    def _handle_build_plots_zip(self) -> None:
        payload = self._read_json_body()
        selected_campaign_dir = str(payload.get("campaign_dir") or "").strip()
        requested_out_zip = str(payload.get("out_zip") or "").strip()
        with JOB.lock:
            if JOB.running:
                self._send_json({"error": "Poczekaj na zakonczenie biezacej kampanii przed budowaniem paczki ZIP."}, status=409)
                return
        try:
            from analysis.build_plot_zip import build_plot_zip  # type: ignore
        except Exception as exc:
            self._send_json({"error": f"Nie udalo sie zaladowac buildera ZIP: {exc}"}, status=500)
            return

        campaign_dir = _safe_path(selected_campaign_dir) if selected_campaign_dir else _latest_campaign_dir()
        if campaign_dir is not None and not campaign_dir.exists():
            campaign_dir = None
        out_zip = _safe_path(requested_out_zip) if requested_out_zip else None
        if requested_out_zip and out_zip is None:
            self._send_json({"error": "Nieprawidlowa sciezka zapisu ZIP."}, status=400)
            return

        try:
            result = build_plot_zip(campaign_dir=campaign_dir, out_zip=out_zip)
        except Exception as exc:
            self._send_json({"error": f"Nie udalo sie zbudowac paczki ZIP: {exc}"}, status=500)
            return
        self._send_json(result)


def main() -> None:
    ap = argparse.ArgumentParser(description="Local graphical pipeline for Final.")
    ap.add_argument("--host", default="127.0.0.1")
    ap.add_argument("--port", type=int, default=8765)
    ap.add_argument("--no-browser", action="store_true")
    args = ap.parse_args()

    if not STATIC_DIR.exists():
        raise SystemExit(f"Missing static dir: {STATIC_DIR}")

    requested_port = int(args.port)
    httpd = None
    bound_port = requested_port
    last_exc: Exception | None = None
    for offset in range(0, 25):
        candidate_port = requested_port + offset
        try:
            httpd = ThreadingHTTPServer((args.host, candidate_port), Handler)
            bound_port = candidate_port
            break
        except OSError as exc:
            last_exc = exc
            if getattr(exc, "errno", None) not in (48, 98):
                raise
    if httpd is None:
        raise SystemExit(
            f"Nie udalo sie uruchomic panelu WWW. Port {requested_port} i kolejne 24 porty sa zajete."
        ) from last_exc

    url = f"http://{args.host}:{bound_port}/"
    print(f"[pipeline-web] root: {ROOT}")
    if bound_port != requested_port:
        print(f"[pipeline-web] port {requested_port} zajety, uzywam {bound_port}")
    print(f"[pipeline-web] serving: {url}")
    if not args.no_browser:
        try:
            webbrowser.open(url)
        except Exception:
            pass
    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\n[pipeline-web] stopping")
        if JOB.snapshot().get("running"):
            _stop_active_job(reason="server_shutdown", wait_s=2.5)
    finally:
        httpd.server_close()


if __name__ == "__main__":
    main()
