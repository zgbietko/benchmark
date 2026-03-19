#!/usr/bin/env python3
from __future__ import annotations

import argparse
from datetime import datetime
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
from typing import Any


ROOT = Path(__file__).resolve().parents[1]
VALIDATION_ROOT = ROOT / "data" / "validation"
OPTIMIZATION_ROOT = ROOT / "data" / "optimization"


DEFAULT_BACKENDS = ("cpu", "cuda", "intel")


def _parse_backends(raw: str) -> list[str]:
    out: list[str] = []
    for chunk in str(raw).split(","):
        backend = chunk.strip().lower()
        if not backend:
            continue
        if backend not in ("cpu", "cuda", "intel", "opencl"):
            raise ValueError(f"Unsupported backend in validation set: {backend}")
        if backend not in out:
            out.append(backend)
    return out or list(DEFAULT_BACKENDS)


def _new_validation_dir() -> Path:
    VALIDATION_ROOT.mkdir(parents=True, exist_ok=True)
    ts = datetime.now().strftime("%Y%m%d_%H%M%S")
    out = VALIDATION_ROOT / f"{ts}__ubuntu_filip_validation"
    out.mkdir(parents=True, exist_ok=True)
    return out


def _run_capture(cmd: list[str], *, log_path: Path) -> int:
    with log_path.open("w", encoding="utf-8") as log:
        proc = subprocess.run(
            cmd,
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            check=False,
        )
        log.write(proc.stdout or "")
    return int(proc.returncode)


def _capture_command(name: str, cmd: list[str], out_dir: Path) -> dict[str, Any]:
    log_path = out_dir / f"{name}.log"
    if shutil.which(cmd[0]) is None:
        log_path.write_text(f"[missing command] {' '.join(cmd)}\n", encoding="utf-8")
        return {"name": name, "cmd": cmd, "returncode": 127, "log": str(log_path)}
    rc = _run_capture(cmd, log_path=log_path)
    return {"name": name, "cmd": cmd, "returncode": rc, "log": str(log_path)}


def _optimization_dirs() -> set[str]:
    if not OPTIMIZATION_ROOT.exists():
        return set()
    return {p.name for p in OPTIMIZATION_ROOT.iterdir() if p.is_dir()}


def _latest_new_optimization_dir(before: set[str]) -> Path | None:
    if not OPTIMIZATION_ROOT.exists():
        return None
    new_dirs = [p for p in OPTIMIZATION_ROOT.iterdir() if p.is_dir() and p.name not in before]
    if not new_dirs:
        return None
    return max(new_dirs, key=lambda p: p.stat().st_mtime)


def _load_json(path: Path) -> dict[str, Any]:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except Exception:
        return {}


def _write_report(out_dir: Path, manifest: dict[str, Any]) -> None:
    lines: list[str] = []
    lines.append(f"# Ubuntu Filip Validation")
    lines.append("")
    lines.append(f"- created_at: `{manifest.get('created_at', '')}`")
    lines.append(f"- requested_backends: `{','.join(manifest.get('requested_backends', []))}`")
    lines.append(f"- profile: `{manifest.get('profile', '')}`")
    lines.append(f"- repeats: `{manifest.get('repeats', 0)}`")
    lines.append(f"- limit_option_rows: `{manifest.get('limit_option_rows', 0)}`")
    lines.append("")
    lines.append("## Preflight")
    preflight = manifest.get("preflight", {})
    lines.append(f"- returncode: `{preflight.get('returncode', '')}`")
    lines.append(f"- log: `{preflight.get('log', '')}`")
    lines.append("")
    lines.append("## Runs")
    for run in manifest.get("runs", []):
        lines.append(f"### {str(run.get('backend', '')).upper()}")
        lines.append(f"- returncode: `{run.get('returncode', '')}`")
        lines.append(f"- log: `{run.get('log', '')}`")
        lines.append(f"- optimization_dir: `{run.get('optimization_dir', '')}`")
        summary = run.get("summary", {}) or {}
        if summary:
            lines.append(f"- device: `{summary.get('device', '')}`")
            lines.append(f"- resolved_backend: `{summary.get('resolved_backend', '')}`")
            lines.append(f"- execution_mode: `{summary.get('execution_mode', '')}`")
            lines.append(f"- device_index_used: `{summary.get('device_index_used', '')}`")
            lines.append(f"- total_evaluations: `{summary.get('total_evaluations', '')}`")
            lines.append(f"- feasible_evaluations: `{summary.get('feasible_evaluations', '')}`")
            best = summary.get("best_overall", {}) or {}
            lines.append(f"- best_ns_per_unit: `{best.get('ns_per_unit', '')}`")
            lines.append(f"- article_plots_dir: `{summary.get('article_plots_dir', '')}`")
            lines.append(f"- combined_csv: `{summary.get('combined_csv', '')}`")
        lines.append("")

    lines.append("## System Snapshot")
    for snap in manifest.get("system_snapshot", []):
        lines.append(f"- `{snap.get('name', '')}`: `{snap.get('log', '')}` (rc={snap.get('returncode', '')})")

    (out_dir / "validation_report.md").write_text("\n".join(lines) + "\n", encoding="utf-8")


def main() -> None:
    ap = argparse.ArgumentParser(description="Ubuntu validation runner for Filip_original on CPU/CUDA/Intel OpenCL.")
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="paper")
    ap.add_argument("--repeats", type=int, default=3)
    ap.add_argument("--backends", default="cpu,cuda,intel")
    ap.add_argument("--limit-option-rows", type=int, default=0)
    ap.add_argument("--memory-budget-mb-intel", type=int, default=768)
    ap.add_argument("--skip-missing", action="store_true", help="Skip unavailable backends instead of failing.")
    ap.add_argument("--skip-system-snapshot", action="store_true")
    args = ap.parse_args()

    backends = _parse_backends(args.backends)
    out_dir = _new_validation_dir()
    logs_dir = out_dir / "logs"
    logs_dir.mkdir(parents=True, exist_ok=True)

    manifest: dict[str, Any] = {
        "created_at": datetime.now().astimezone().isoformat(),
        "profile": args.profile,
        "repeats": int(args.repeats),
        "requested_backends": backends,
        "limit_option_rows": int(args.limit_option_rows),
        "out_dir": str(out_dir),
        "system_snapshot": [],
        "runs": [],
    }

    if not args.skip_system_snapshot:
        snapshot_cmds = [
            ("uname", ["uname", "-a"]),
            ("python", [sys.executable, "--version"]),
            ("lspci", ["bash", "-lc", "lspci | grep -Ei 'vga|3d|display' || true"]),
            ("nvidia_smi", ["bash", "-lc", "nvidia-smi -L || true"]),
            ("clinfo", ["bash", "-lc", "clinfo || true"]),
        ]
        for name, cmd in snapshot_cmds:
            manifest["system_snapshot"].append(_capture_command(name, cmd, logs_dir))

    preflight_cmd = [
        sys.executable,
        str(ROOT / "run_fem_parametric_preflight.py"),
        "--backend",
        ",".join(backends),
        "--platform-profile",
        "auto",
        "--json",
    ]
    preflight_log = logs_dir / "preflight.log"
    preflight_rc = _run_capture(preflight_cmd, log_path=preflight_log)
    preflight_payload = {}
    try:
        preflight_payload = json.loads(preflight_log.read_text(encoding="utf-8"))
    except Exception:
        preflight_payload = {}
    manifest["preflight"] = {
        "returncode": preflight_rc,
        "log": str(preflight_log),
        "payload": preflight_payload,
    }

    availability: dict[str, bool] = {}
    for item in preflight_payload.get("checks", []):
        availability[str(item.get("backend", ""))] = bool(item.get("available"))

    failures: list[str] = []
    for backend in backends:
        if not availability.get(backend, False):
            msg = f"Backend unavailable in preflight: {backend}"
            if args.skip_missing:
                manifest["runs"].append(
                    {
                        "backend": backend,
                        "returncode": 0,
                        "skipped": True,
                        "reason": msg,
                    }
                )
                continue
            failures.append(msg)
            continue

        before = _optimization_dirs()
        cmd = [
            sys.executable,
            str(ROOT / "run_filip_original.py"),
            "--backend",
            backend,
            "--profile",
            args.profile,
            "--repeats",
            str(args.repeats),
            "--operators",
            "diffusion,diffusion_convection_mass",
            "--element-types",
            "tet4",
            "--variants",
            "qss,sqs,ssq",
            "--dtype",
            "float32",
            "--workgroup-size",
            "0",
        ]
        if backend == "intel":
            cmd += ["--memory-budget-mb", str(args.memory_budget_mb_intel)]
        if int(args.limit_option_rows) > 0:
            cmd += ["--limit-option-rows", str(args.limit_option_rows)]

        log_path = logs_dir / f"run_{backend}.log"
        rc = _run_capture(cmd, log_path=log_path)
        optimization_dir = _latest_new_optimization_dir(before)
        summary = {}
        if optimization_dir is not None:
            summary = _load_json(optimization_dir / "summary.json")
        run_payload = {
            "backend": backend,
            "returncode": rc,
            "log": str(log_path),
            "optimization_dir": str(optimization_dir) if optimization_dir is not None else "",
            "summary": summary,
        }
        manifest["runs"].append(run_payload)
        if rc != 0:
            failures.append(f"Backend run failed: {backend}")

    (out_dir / "manifest.json").write_text(json.dumps(manifest, indent=2, ensure_ascii=True), encoding="utf-8")
    _write_report(out_dir, manifest)

    print(json.dumps(manifest, indent=2, ensure_ascii=True))
    if failures:
        for failure in failures:
            print(f"[ERROR] {failure}", file=sys.stderr)
        raise SystemExit(1)


if __name__ == "__main__":
    main()
