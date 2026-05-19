#!/usr/bin/env python3
from __future__ import annotations

import argparse
import importlib
import json
import os
from pathlib import Path
import platform
import shutil
import socket
import subprocess
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.provenance import git_provenance, sha256_json
from monitoring.power_monitor import capture_power_snapshot
from monitoring.thermal_monitor import capture_thermal_snapshot


def _safe_run(cmd: list[str]) -> str:
    try:
        proc = subprocess.run(cmd, text=True, capture_output=True, check=False)
    except Exception:
        return ""
    return proc.stdout.strip() if proc.returncode == 0 else ""


def _cpu_model() -> str:
    system = platform.system()
    if system == "Darwin":
        return _safe_run(["sysctl", "-n", "machdep.cpu.brand_string"]) or _safe_run(["sysctl", "-n", "machdep.cpu.brand_string"]) or _safe_run(["sysctl", "-n", "hw.model"])
    if system == "Linux":
        info = Path("/proc/cpuinfo")
        if info.exists():
            for line in info.read_text(errors="ignore").splitlines():
                if ":" in line and line.lower().startswith("model name"):
                    return line.split(":", 1)[1].strip()
        lscpu = _safe_run(["lscpu"])
        for line in lscpu.splitlines():
            if "Model name:" in line:
                return line.split(":", 1)[1].strip()
    return platform.processor() or platform.machine()


def _memory_bytes() -> int | None:
    system = platform.system()
    if system == "Darwin":
        raw = _safe_run(["sysctl", "-n", "hw.memsize"])
        try:
            return int(raw)
        except Exception:
            return None
    if system == "Linux":
        meminfo = Path("/proc/meminfo")
        if meminfo.exists():
            for line in meminfo.read_text(errors="ignore").splitlines():
                if line.startswith("MemTotal:"):
                    try:
                        kb = int(line.split()[1])
                        return kb * 1024
                    except Exception:
                        return None
    return None


def _library_versions(names: list[str]) -> dict[str, str]:
    out: dict[str, str] = {}
    for name in names:
        try:
            mod = importlib.import_module(name)
            out[name] = str(getattr(mod, "__version__", "unknown"))
        except Exception:
            continue
    return out


def _compiler_versions() -> dict[str, str]:
    checks = {
        "clang": ["clang", "--version"],
        "gcc": ["gcc", "--version"],
        "nvcc": ["nvcc", "--version"],
        "hipcc": ["hipcc", "--version"],
        "python": [sys.executable, "--version"],
    }
    out: dict[str, str] = {}
    for key, cmd in checks.items():
        text = _safe_run(cmd)
        if text:
            out[key] = text.splitlines()[0]
    return out


def capture_environment_manifest(*, backend: str = "", device_name: str = "", command_args: list[str] | None = None, root: Path | None = None) -> dict[str, Any]:
    root = root or ROOT
    manifest: dict[str, Any] = {
        "timestamp": __import__("datetime").datetime.now().astimezone().isoformat(),
        "hostname": socket.gethostname(),
        "system": platform.system(),
        "kernel_version": platform.release(),
        "platform_version": platform.version(),
        "architecture": platform.machine(),
        "cpu_model": _cpu_model(),
        "gpu_model": device_name,
        "backend": backend,
        "ram_bytes": _memory_bytes(),
        "python_version": platform.python_version(),
        "python_executable": sys.executable,
        "compiler_versions": _compiler_versions(),
        "library_versions": _library_versions(["numpy", "matplotlib", "yaml"]),
        "power": capture_power_snapshot(),
        "thermal": capture_thermal_snapshot(),
        "git": git_provenance(root),
        "command_line_arguments": list(command_args or []),
        "user": os.environ.get("USER") or os.environ.get("USERNAME") or "unknown",
    }
    manifest["environment_hash"] = sha256_json({k: v for k, v in manifest.items() if k != "environment_hash"})
    return manifest


def main() -> None:
    ap = argparse.ArgumentParser(description="Capture environment manifest for Final runs.")
    ap.add_argument("--backend", default="")
    ap.add_argument("--device-name", default="")
    ap.add_argument("--out", required=True)
    ap.add_argument("args", nargs="*")
    args = ap.parse_args()
    payload = capture_environment_manifest(backend=args.backend, device_name=args.device_name, command_args=args.args)
    out = Path(args.out).expanduser().resolve()
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding="utf-8")
    print(str(out))


if __name__ == "__main__":
    main()
