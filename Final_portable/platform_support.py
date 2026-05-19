from __future__ import annotations

import json
import os
import platform
import shutil
import sys
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from cpu_utils import detect_cpu_model, detect_cpu_topology
from device_resolution import list_opencl_devices
from run_workflow import _backend_available, _resolve_fem_backend_token, _resolve_gpu_backend


GPU_BACKENDS = ("metal", "cuda", "hip", "opencl")
UI_PROFILES = ("apple", "nvidia", "amd", "intel_arc", "intel_igpu")


def _cmd(name: str) -> bool:
    return shutil.which(name) is not None


def _import_ok(name: str) -> tuple[bool, str]:
    try:
        __import__(name)
        return True, ""
    except Exception as exc:
        return False, f"{type(exc).__name__}: {exc}"


def _backend_status(name: str) -> dict[str, Any]:
    try:
        available = bool(_backend_available(name))
        return {"available": available, "reason": ""}
    except Exception as exc:
        return {"available": False, "reason": f"{type(exc).__name__}: {exc}"}


def _recommended_platform_profile() -> str:
    system = platform.system()
    if system == "Darwin":
        return "apple"
    if _backend_available("cuda"):
        return "nvidia"
    if _backend_available("hip"):
        return "amd"
    if _backend_available("opencl"):
        return "intel_arc"
    return "auto"


def _candidate_exact_dirs(root: Path) -> list[Path]:
    return [
        root / "Kod Filipa" / "mod_2022",
        root / "legacy" / "filip_exact_bundle" / "mod_2022",
    ]


def _oneapi_state() -> dict[str, Any]:
    compiler_candidates = [
        Path("/opt/intel/oneapi/compiler/latest/linux/bin/icx"),
        Path("/opt/intel/oneapi/compiler/latest/bin/icx"),
    ]
    mkl_candidates = [
        Path("/opt/intel/oneapi/mkl/latest/include"),
        Path("/opt/intel/oneapi/mkl/latest/lib/intel64"),
    ]
    compiler_path = next((path for path in compiler_candidates if path.exists()), None)
    mkl_ready = any(path.exists() for path in mkl_candidates)
    return {
        "compiler_ready": compiler_path is not None or _cmd("icx"),
        "compiler_path": str(compiler_path) if compiler_path else (shutil.which("icx") or ""),
        "mkl_ready": mkl_ready,
    }


def _status_payload(status: str, reason: str, **extra: Any) -> dict[str, Any]:
    payload = {"status": status, "reason": reason}
    payload.update(extra)
    return payload


def _exact_reference_support(root: Path, backend_status: dict[str, dict[str, Any]]) -> dict[str, Any]:
    system = platform.system()
    if system == "Darwin":
        metal_ok = bool(backend_status.get("metal", {}).get("available"))
        reason = (
            "Metal exact-style port jest dostepny. Replay 1:1 wymaga dumpow OpenCL z kampanii Linux/OpenCL."
            if metal_ok
            else "Brak aktywnego backendu Metal."
        )
        return {
            "available_now": metal_ok,
            "status": "supported" if metal_ok else "unsupported",
            "mode": "exact_reference_metal_port",
            "replay_supported": metal_ok,
            "requires_external_assets_for_replay": True,
            "candidate_mod_dirs": [],
            "reason": reason,
        }

    if system == "Linux":
        opencl_ok = bool(backend_status.get("opencl", {}).get("available"))
        oneapi = _oneapi_state()
        candidate_dirs = [path for path in _candidate_exact_dirs(root) if path.exists()]
        assets_ready = bool(candidate_dirs)
        ready_now = opencl_ok and oneapi["compiler_ready"] and oneapi["mkl_ready"] and assets_ready
        conditional = opencl_ok and oneapi["compiler_ready"] and oneapi["mkl_ready"]
        if ready_now:
            reason = "OpenCL + oneAPI + lokalne assets mod_2022 sa gotowe."
            status = "supported"
        elif conditional:
            reason = "OpenCL i oneAPI sa gotowe, ale mod_2022 trzeba dostarczyc lokalnie albo wskazac przez --filip-modfem-dir."
            status = "conditional"
        else:
            missing = []
            if not opencl_ok:
                missing.append("OpenCL runtime / pyopencl")
            if not oneapi["compiler_ready"]:
                missing.append("icx / oneAPI compiler")
            if not oneapi["mkl_ready"]:
                missing.append("MKL")
            reason = "Brakuje: " + ", ".join(missing)
            status = "unsupported"
        return {
            "available_now": ready_now,
            "status": status,
            "mode": "exact_reference_opencl_oneapi",
            "replay_supported": False,
            "requires_external_assets_for_replay": False,
            "candidate_mod_dirs": [str(path) for path in candidate_dirs],
            "reason": reason,
            "oneapi": oneapi,
            "supported_if_assets_provided": conditional,
        }

    return {
        "available_now": False,
        "status": "unsupported",
        "mode": "unsupported",
        "replay_supported": False,
        "requires_external_assets_for_replay": False,
        "candidate_mod_dirs": [],
        "reason": "Exact reference jest zaimplementowany dla macOS/Metal oraz Linux/OpenCL.",
    }


def _energy_support() -> dict[str, Any]:
    system = platform.system()
    py_nvml_ok, py_nvml_err = _import_ok("pynvml")
    support: list[dict[str, Any]] = []
    warnings: list[str] = []

    if system == "Darwin":
        if _cmd("powermetrics"):
            support.append(
                {
                    "name": "powermetrics",
                    "status": "conditional",
                    "admin_required": True,
                    "scope": "cpu+gpu",
                    "reason": "Na macOS zwykle wymaga sudo; bez tego pomiar przechodzi w best-effort / unsupported.",
                }
            )
        else:
            warnings.append("Brak powermetrics.")
    elif system == "Linux":
        rapl = Path("/sys/class/powercap")
        if rapl.exists():
            support.append(
                {
                    "name": "intel_rapl",
                    "status": "supported",
                    "admin_required": False,
                    "scope": "cpu",
                    "reason": "Wykryto sysfs powercap.",
                }
            )
        if py_nvml_ok or _cmd("nvidia-smi"):
            support.append(
                {
                    "name": "nvidia_nvml",
                    "status": "supported" if py_nvml_ok else "conditional",
                    "admin_required": False,
                    "scope": "gpu",
                    "reason": "NVML / nvidia-smi dostepne." if py_nvml_ok else "nvidia-smi dostepne, ale modul pynvml nie jest zaladowany.",
                }
            )
        if _cmd("rocm-smi"):
            support.append(
                {
                    "name": "rocm_smi",
                    "status": "conditional",
                    "admin_required": False,
                    "scope": "gpu",
                    "reason": "ROCm SMI widoczne; jakosc pomiaru zalezy od hosta i sterownika.",
                }
            )
    else:
        warnings.append(f"Brak dedykowanej warstwy energii dla systemu {system}.")

    if not support:
        support.append(
            {
                "name": "unsupported",
                "status": "unsupported",
                "admin_required": False,
                "scope": "n/a",
                "reason": "Nie wykryto zadnego sensownego zrodla mocy / energii.",
            }
        )

    return {
        "entries": support,
        "warnings": warnings,
    }


def _desktop_gui_support() -> dict[str, Any]:
    try:
        import tkinter  # noqa: F401
    except Exception as exc:
        return _status_payload("unsupported", f"tkinter niedostepny: {type(exc).__name__}: {exc}")
    try:
        import matplotlib
        matplotlib.use("TkAgg")
        from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg  # noqa: F401
    except Exception as exc:
        return _status_payload("conditional", f"GUI uruchomi sie bez podgladu wykresow lub wymaga dopiecia TkAgg: {type(exc).__name__}: {exc}")
    return _status_payload("supported", "tkinter + TkAgg sa dostepne.")


def _web_gui_support(root: Path) -> dict[str, Any]:
    static_dir = root / "web" / "static"
    if static_dir.exists() and (static_dir / "index.html").exists():
        return _status_payload("supported", "Statyczny frontend i lokalny serwer HTTP sa dostepne.")
    return _status_payload("unsupported", "Brak katalogu web/static.")


def _declared_platform_matrix(root: Path) -> list[dict[str, Any]]:
    config_path = root / "configs" / "platform_profiles.json"
    if not config_path.exists():
        return []
    payload = json.loads(config_path.read_text(encoding="utf-8"))
    notes_map = {
        "apple": {
            "gpu_path": "metal -> opencl fallback",
            "exact_path": "metal exact-style port; replay 1:1 po dostarczeniu dumpow OpenCL",
            "ai_path": "metal + Core ML probe",
        },
        "nvidia": {
            "gpu_path": "cuda -> opencl fallback",
            "exact_path": "exact_reference przez OpenCL/oneAPI, nie przez CUDA",
            "ai_path": "cuda / cupy; INT8 i FP16 najlepsze przy vendor runtime",
        },
        "amd": {
            "gpu_path": "hip -> opencl fallback",
            "exact_path": "exact_reference przez OpenCL/oneAPI, nie przez HIP",
            "ai_path": "HIP/OpenCL w tej wersji glownie sciezki diagnostyczne / proxy",
        },
        "intel_arc": {
            "gpu_path": "opencl",
            "exact_path": "OpenCL + oneAPI jest naturalna sciezka exact",
            "ai_path": "OpenCL / CPU fallback; vendor-native AI nie jest glowna sciezka tej wersji",
        },
        "intel_igpu": {
            "gpu_path": "opencl",
            "exact_path": "OpenCL + oneAPI",
            "ai_path": "OpenCL / CPU fallback",
        },
    }
    out: list[dict[str, Any]] = []
    for key, value in payload.items():
        item = {
            "profile": key,
            "description": str(value.get("description", "")),
            "arch": str(value.get("arch", "")),
            "backend_preference": list(value.get("backend_preference", [])),
            "all_backends": list(value.get("all_backends", [])),
        }
        item.update(notes_map.get(key, {}))
        out.append(item)
    return out


def build_host_support_report(root: Path | None = None) -> dict[str, Any]:
    repo_root = Path(root or ROOT).resolve()
    system_name = platform.system()
    cpu_model = detect_cpu_model()
    topology = detect_cpu_topology()

    python_modules = {}
    for name in ("numpy", "matplotlib", "pyopencl", "pynvml", "cupy", "coremltools", "Metal"):
        ok, err = _import_ok(name)
        python_modules[name] = {"ok": ok, "error": err}

    commands = {
        "python3": _cmd("python3"),
        "gcc": _cmd("gcc"),
        "clang": _cmd("clang"),
        "nvcc": _cmd("nvcc"),
        "hipcc": _cmd("hipcc"),
        "nvidia-smi": _cmd("nvidia-smi"),
        "rocm-smi": _cmd("rocm-smi"),
        "clinfo": _cmd("clinfo"),
        "powermetrics": _cmd("powermetrics"),
        "icx": _cmd("icx"),
    }

    backend_status = {name: _backend_status(name) for name in GPU_BACKENDS}
    available_backends = [name for name, payload in backend_status.items() if payload.get("available")]

    try:
        opencl_devices = list_opencl_devices()
    except Exception:
        opencl_devices = []

    recommended_backend = _resolve_gpu_backend("auto", _recommended_platform_profile(), "auto")
    fem_backend = _resolve_fem_backend_token("auto", _recommended_platform_profile(), "auto")
    exact_reference = _exact_reference_support(repo_root, backend_status)
    energy = _energy_support()
    desktop_gui = _desktop_gui_support()
    web_gui = _web_gui_support(repo_root)

    cpu_compiler_ready = bool(commands["gcc"] or commands["clang"])
    gpu_ready = bool(available_backends)
    portable_supported = system_name == "Linux"

    workflow_support = {
        "cpu_benchmark": _status_payload(
            "supported" if cpu_compiler_ready else "conditional",
            "CPU benchmarki moga budowac biblioteki natywne." if cpu_compiler_ready else "Brak gcc/clang; uruchomienie zalezy od hosta.",
        ),
        "gpu_benchmark": _status_payload(
            "supported" if gpu_ready else "unsupported",
            f"Dostepne backendy GPU: {', '.join(available_backends)}." if gpu_ready else "Nie wykryto zadnego backendu GPU.",
        ),
        "cpu_real_kernels": _status_payload("supported", "CPU real kernels korzystaja z tego samego hosta i numpy/C backendu."),
        "gpu_real_kernels": _status_payload(
            "supported" if gpu_ready else "unsupported",
            f"GPU real kernels pojda przez: {recommended_backend}." if gpu_ready and recommended_backend else "Brak backendu GPU do real kernels.",
        ),
        "ai_accel": _status_payload(
            "supported",
            "CPU fallback jest dostepny wszedzie; sciezki akcelerowane zalezne od backendu i runtime vendorowego.",
            accelerated_backends=available_backends,
        ),
        "fem_option_validation": _status_payload(
            "supported",
            f"Walidacja FEM ma resolved backend: {fem_backend}.",
        ),
        "filip_original": _status_payload("supported", "Portable sweep dziala na CPU i wspieranych backendach GPU."),
        "filip_exact_reference": _status_payload(
            exact_reference["status"],
            exact_reference["reason"],
            mode=exact_reference.get("mode"),
        ),
        "filip_autotune": _status_payload("supported", "Autotuning korzysta z tego samego problemu FEM-like / Filip co portable sweep."),
        "filip_firefly": _status_payload("supported", "Firefly korzysta z tej samej warstwy wykonawczej co autotuning."),
        "profiler_correlation": _status_payload("supported", "Analiza korelacyjna jest lokalna, zalezy od obecnosci artefaktow kampanii."),
        "full_thesis_pipeline": _status_payload(
            "supported" if gpu_ready and exact_reference["status"] in {"supported", "conditional"} else "conditional",
            (
                "Pelny pipeline jest dostepny; exact_reference moze byc lokalny albo warunkowy."
                if gpu_ready
                else "Pelny pipeline uruchomi CPU path, ale kroki GPU beda pominiete albo unsupported."
            ),
        ),
        "web_gui": web_gui,
        "desktop_gui": desktop_gui,
    }

    package_support = {
        "benchmarks": {
            "cpu": cpu_compiler_ready,
            "gpu": gpu_ready,
        },
        "real_kernels": {
            "cpu": True,
            "gpu": gpu_ready,
        },
        "filip": {
            "portable_case": True,
            "exact_reference": exact_reference.get("available_now", False),
            "exact_reference_conditional": exact_reference.get("status") == "conditional",
        },
        "full": {
            "cpu_path": True,
            "gpu_path": gpu_ready,
            "exact_reference": exact_reference.get("available_now", False),
            "exact_reference_conditional": exact_reference.get("status") == "conditional",
        },
    }

    warnings: list[str] = []
    if system_name != "Linux":
        warnings.append("Portable launcher jest przygotowany na host Linux; na tym hoście mozna go zbudowac, ale nie uruchomic w trybie pendrive-run.")
    if not gpu_ready:
        warnings.append("Brak wykrytego backendu GPU; workflowy GPU beda niedostepne albo zredukowane do CPU-only.")
    if exact_reference.get("status") == "conditional":
        warnings.append("Exact reference nie jest kompletne lokalnie; wymaga dodatkowych assets lub toolchainu.")
    if any(item.get("status") == "conditional" and item.get("name") == "powermetrics" for item in energy["entries"]):
        warnings.append("Na macOS energia/moc wymaga uruchomienia z uprawnieniami administratora dla wiarygodnych danych.")

    report = {
        "bundle_root": str(repo_root),
        "system": {
            "os": system_name,
            "release": platform.release(),
            "machine": platform.machine(),
            "hostname": platform.node(),
            "python": sys.version.split()[0],
        },
        "cpu": {
            "model": cpu_model,
            "topology": topology,
        },
        "commands": commands,
        "python_modules": python_modules,
        "gpu": {
            "available_backends": {key: bool(payload.get("available")) for key, payload in backend_status.items()},
            "backend_status": backend_status,
            "recommended_backend": recommended_backend,
            "recommended_fem_backend": fem_backend,
            "opencl_devices": opencl_devices,
        },
        "exact_reference": exact_reference,
        "energy": energy,
        "portable_bundle": {
            "build_supported": (repo_root / "scripts" / "build_portable_bundle.py").exists(),
            "run_supported_on_host": portable_supported,
            "run_reason": "Host Linux - launcher portable powinien dzialac lokalnie." if portable_supported else "Launcher portable jest linuxowy; ten host nie jest Linuxem.",
        },
        "workflow_support": workflow_support,
        "package_support": package_support,
        "gui": {
            "web": web_gui,
            "desktop": desktop_gui,
        },
        "platform_matrix": _declared_platform_matrix(repo_root),
        "warnings": warnings,
        "limitations": {
            "drivers": "Projekt nie przenosi sterownikow GPU, CUDA, ROCm ani runtime systemowego OpenCL. Host musi je miec lokalnie.",
            "exact_reference": "Apple: exact-style Metal port dziala lokalnie, replay wymaga dumpow OpenCL. Linux: exact_reference wymaga OpenCL + oneAPI + assets mod_2022.",
            "energy": "Warstwa energii jest wieloplatformowa tylko best-effort; jakosc pomiaru zalezy od licznikow i uprawnien hosta.",
        },
        "summary": {
            "multiplatform_core": True,
            "portable_same_workflows": True,
            "portable_same_runtime_prereqs": False,
            "recommended_platform_profile": _recommended_platform_profile(),
        },
    }
    return report


def render_host_support_markdown(report: dict[str, Any]) -> str:
    lines: list[str] = []
    system = report.get("system", {})
    cpu = report.get("cpu", {})
    topology = cpu.get("topology", {}) or {}
    gpu = report.get("gpu", {}) or {}
    lines.append("# Raport kompatybilnosci i wieloplatformowosci")
    lines.append("")
    lines.append(f"- Host: `{system.get('hostname', '')}`")
    lines.append(f"- System: `{system.get('os', '')} {system.get('release', '')} {system.get('machine', '')}`")
    lines.append(f"- Python: `{system.get('python', '')}`")
    lines.append(f"- CPU: `{cpu.get('model', '')}`")
    lines.append(f"- Watki logiczne: `{topology.get('logical_cpus', 'n/a')}` | fizyczne: `{topology.get('physical_cpus', 'n/a')}`")
    if topology.get("perf_logical_cpus") or topology.get("eff_logical_cpus"):
        lines.append(f"- Podzial rdzeni: `{topology.get('perf_logical_cpus', 0)}P + {topology.get('eff_logical_cpus', 0)}E`")
    lines.append("")
    lines.append("## Backendy GPU")
    lines.append("")
    lines.append(f"- Zalecany backend GPU: `{gpu.get('recommended_backend') or 'none'}`")
    lines.append(f"- Zalecany backend FEM: `{gpu.get('recommended_fem_backend') or 'cpu'}`")
    for backend, payload in sorted((gpu.get("backend_status") or {}).items()):
        status = "available" if payload.get("available") else "not available"
        reason = str(payload.get("reason") or "")
        suffix = f" | {reason}" if reason else ""
        lines.append(f"- `{backend}`: `{status}`{suffix}")
    devices = gpu.get("opencl_devices") or []
    if devices:
        lines.append("")
        lines.append("### OpenCL devices")
        lines.append("")
        for info in devices:
            lines.append(
                f"- `[{info['index']}] {info['device_name']}` | vendor=`{info['device_vendor']}` | type=`{info['device_type']}` | platform=`{info['platform_name']}`"
            )
    lines.append("")
    lines.append("## Workflowy")
    lines.append("")
    for name, payload in sorted((report.get("workflow_support") or {}).items()):
        lines.append(f"- `{name}`: `{payload.get('status')}` - {payload.get('reason', '')}")
    lines.append("")
    lines.append("## Exact reference")
    lines.append("")
    exact = report.get("exact_reference") or {}
    lines.append(f"- Status: `{exact.get('status', 'unknown')}`")
    lines.append(f"- Tryb: `{exact.get('mode', 'unknown')}`")
    lines.append(f"- Opis: {exact.get('reason', '')}")
    if exact.get("candidate_mod_dirs"):
        for path in exact.get("candidate_mod_dirs", []):
            lines.append(f"- assets: `{path}`")
    lines.append("")
    lines.append("## Energia i moc")
    lines.append("")
    for entry in (report.get("energy", {}) or {}).get("entries", []):
        lines.append(
            f"- `{entry.get('name')}`: `{entry.get('status')}` | scope=`{entry.get('scope')}` | admin=`{entry.get('admin_required')}` | {entry.get('reason')}"
        )
    lines.append("")
    lines.append("## Portable bundle")
    lines.append("")
    portable = report.get("portable_bundle", {}) or {}
    lines.append(f"- build_supported: `{portable.get('build_supported')}`")
    lines.append(f"- run_supported_on_host: `{portable.get('run_supported_on_host')}`")
    lines.append(f"- reason: {portable.get('run_reason', '')}")
    lines.append("")
    lines.append("## Zadeklarowane profile platform")
    lines.append("")
    for item in report.get("platform_matrix", []) or []:
        lines.append(
            f"- `{item.get('profile')}`: {item.get('description')} | backends=`{','.join(item.get('backend_preference', []))}` | exact=`{item.get('exact_path', '')}` | ai=`{item.get('ai_path', '')}`"
        )
    warnings = report.get("warnings") or []
    if warnings:
        lines.append("")
        lines.append("## Ostrzezenia")
        lines.append("")
        for item in warnings:
            lines.append(f"- {item}")
    lines.append("")
    return "\n".join(lines)
