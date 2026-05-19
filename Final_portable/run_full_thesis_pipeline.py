#!/usr/bin/env python3
from __future__ import annotations

import argparse
from collections import deque
from datetime import datetime, timezone
import json
import math
import os
from pathlib import Path
import subprocess
import sys
import time
from typing import Any, Dict, List

ROOT = Path(__file__).resolve().parent
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.provenance import collect_runtime_provenance, sha256_json
from analysis.contract_utils import standardize_campaign_artifacts
from analysis.google_drive_sync import (
    default_google_drive_dir,
    default_google_drive_subdir,
    default_rclone_remote,
    default_sync_mode,
    sync_artifacts_to_google_drive,
)
from profiles.loader import load_profile
from run_session import manifest_base, write_manifest
from run_workflow import _backend_available, _resolve_fem_backend_token, _resolve_gpu_backend

THESIS_ROOT = ROOT / "data" / "thesis_full"
MARKER = "=== WORKFLOW DONE ==="


def _now_tag() -> str:
    return datetime.now().strftime("%Y%m%d_%H%M%S")


def _json_default(value: Any) -> Any:
    if isinstance(value, Path):
        return str(value)
    return str(value)


def _write_json(path: Path, payload: Dict[str, Any]) -> None:
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=True, default=_json_default), encoding="utf-8")


def _append_pipeline_log(path: Path, message: str) -> None:
    timestamp = datetime.now(timezone.utc).isoformat()
    path.parent.mkdir(parents=True, exist_ok=True)
    with path.open("a", encoding="utf-8") as handle:
        handle.write(f"[{timestamp}] {message}\n")


def _append_pipeline_event(path: Path, payload: Dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    line = json.dumps(payload, ensure_ascii=True, default=_json_default)
    with path.open("a", encoding="utf-8") as handle:
        handle.write(line + "\n")


def _apply_experiment_profile(args: argparse.Namespace) -> argparse.Namespace:
    name = str(getattr(args, "experiment_profile", "") or "").strip()
    if not name:
        return args
    profile = load_profile(name)
    if "benchmark_mode" in profile:
        args.benchmark_mode = str(profile.get("benchmark_mode") or args.benchmark_mode)
    if "repetitions" in profile:
        args.repeats = int(profile.get("repetitions", args.repeats))
    if "warmups" in profile:
        args.warmups = int(profile.get("warmups", args.warmups))
    if "real_runs" in profile:
        args.real_runs = int(profile.get("real_runs", args.real_runs))
    if "trials" in profile:
        args.trials = int(profile.get("trials", args.trials))
    if "population" in profile:
        args.population = int(profile.get("population", args.population))
    if "iterations" in profile:
        args.iterations = int(profile.get("iterations", args.iterations))
    return args


def _safe_slug(text: str) -> str:
    out = []
    for ch in text.lower().strip():
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


def _create_campaign_dir(*, profile: str, backend: str) -> Path:
    THESIS_ROOT.mkdir(parents=True, exist_ok=True)
    name = f"{_now_tag()}__full_thesis__profile-{profile}__backend-{backend}"
    path = THESIS_ROOT / name
    (path / "logs").mkdir(parents=True, exist_ok=True)
    (path / "plots").mkdir(parents=True, exist_ok=True)
    (path / "artifacts").mkdir(parents=True, exist_ok=True)
    latest_link = THESIS_ROOT / "latest"
    try:
        if latest_link.exists() or latest_link.is_symlink():
            latest_link.unlink()
        latest_link.symlink_to(path.name)
    except Exception:
        try:
            (THESIS_ROOT / "latest.txt").write_text(path.name + "\n", encoding="utf-8")
        except Exception:
            pass
    return path


def _extract_payload_from_text(text: str) -> Dict[str, Any]:
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


def _symlink_or_note(link_path: Path, target: Path) -> None:
    try:
        if link_path.exists() or link_path.is_symlink():
            link_path.unlink()
        link_path.symlink_to(target)
    except Exception:
        link_path.with_suffix(".txt").write_text(str(target) + "\n", encoding="utf-8")


def _run_logged(cmd: List[str], *, log_path: Path) -> tuple[int, Dict[str, Any]]:
    tail: deque[str] = deque(maxlen=4000)
    with log_path.open("w", encoding="utf-8") as log_handle:
        log_handle.write("$ " + " ".join(cmd) + "\n\n")
        log_handle.flush()
        env = dict(os.environ)
        env["PYTHONUNBUFFERED"] = "1"
        proc = subprocess.Popen(
            cmd,
            cwd=ROOT,
            env=env,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            bufsize=1,
        )
        assert proc.stdout is not None
        for line in proc.stdout:
            sys.stdout.write(line)
            log_handle.write(line)
            tail.append(line)
        rc = proc.wait()
    payload = _extract_payload_from_text("".join(tail))
    return rc, payload


def _compact_payload(payload: Dict[str, Any]) -> Dict[str, Any]:
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
        "figure_set_dir",
        "figure_set_plots",
        "figure_appendix_dir",
        "figure_appendix_plots",
        "figure_manifest_path",
        "plots",
        "best_overall",
        "analysis",
        "error",
        "exit_code",
    ]
    return {key: payload.get(key) for key in keep if key in payload}


def _step_summary(step: Dict[str, Any]) -> Dict[str, Any]:
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


def _live_summary(
    *,
    campaign_dir: Path,
    forced_profile: str,
    args: argparse.Namespace,
    resolved_gpu_backend: str | None,
    resolved_fem_backend: str,
    steps: List[Dict[str, Any]],
    running: bool,
) -> Dict[str, Any]:
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
    summary: Dict[str, Any] = {
        "workflow": "full_thesis_pipeline",
        "experiment_class": "thesis_full_campaign",
        "timestamp_utc": datetime.now(timezone.utc).isoformat(),
        "profile": forced_profile,
        "requested_profile": args.profile,
        "requested_backend": args.backend,
        "benchmark_mode": args.benchmark_mode,
        "experiment_profile": str(getattr(args, "experiment_profile", "") or ""),
        "warmups": int(getattr(args, "warmups", 0) or 0),
        "resolved_gpu_backend": resolved_gpu_backend or "",
        "resolved_fem_backend": resolved_fem_backend,
        "platform_profile": args.platform_profile,
        "arch_request": args.arch,
        "device_index": int(args.device_index),
        "benchmarks_max_cpu_threads": int(args.benchmarks_max_cpu_threads),
        "real_kernels_max_cpu_threads": int(args.real_kernels_max_cpu_threads),
        "filip_max_cpu_threads": int(args.filip_max_cpu_threads),
        "campaign_dir": str(campaign_dir),
        "running": bool(running),
        "current_step": current_step,
        "steps": [_step_summary(step) for step in steps],
        "key_results": key_results,
        "required_failures": required_failures,
        "critical_success": (len(required_failures) == 0) if not running else False,
        "provenance": collect_runtime_provenance(ROOT),
    }
    summary["summary_hash"] = sha256_json(summary)
    summary["exit_code"] = 0 if (not running and not required_failures) else (1 if not running else None)
    return summary


def _build_campaign_markdown(summary: Dict[str, Any]) -> str:
    lines: List[str] = []
    lines.append("# Kampania pelna Final")
    lines.append("")
    lines.append(f"- Czas utworzenia (UTC): `{summary.get('timestamp_utc', '')}`")
    lines.append(f"- Profil: `{summary.get('profile', '')}`")
    lines.append(f"- Backend zadany: `{summary.get('requested_backend', '')}`")
    lines.append(f"- Tryb benchmarkow: `{summary.get('benchmark_mode', '')}`")
    lines.append(f"- CPU threads | benchmarki: `{summary.get('benchmarks_max_cpu_threads', '')}`")
    lines.append(f"- CPU threads | real kernels: `{summary.get('real_kernels_max_cpu_threads', '')}`")
    lines.append(f"- CPU threads | Filip: `{summary.get('filip_max_cpu_threads', '')}`")
    lines.append(f"- Backend GPU rozwiazany: `{summary.get('resolved_gpu_backend', '')}`")
    lines.append(f"- Backend FEM rozwiazany: `{summary.get('resolved_fem_backend', '')}`")
    lines.append(f"- Kod wyjscia kampanii: `{summary.get('exit_code', '')}`")
    lines.append("")
    lines.append("## Kroki")
    lines.append("")
    for step in summary.get("steps", []):
        lines.append(f"### {step.get('label', step.get('id', 'krok'))}")
        lines.append("")
        lines.append(f"- Status: `{step.get('status', '')}`")
        lines.append(f"- Czas: `{step.get('elapsed_s', '')}` s")
        if step.get("result_dir"):
            lines.append(f"- Wynik: `{step.get('result_dir')}`")
        if step.get("log_path"):
            lines.append(f"- Log: `{step.get('log_path')}`")
        if step.get("reason"):
            lines.append(f"- Uwaga: {step.get('reason')}")
        payload = step.get("payload") or {}
        if isinstance(payload, dict) and payload:
            workflow = str(payload.get("workflow", "")).strip()
            if workflow:
                lines.append(f"- Workflow: `{workflow}`")
            optimizer = str(payload.get("optimizer", "")).strip()
            if optimizer:
                lines.append(f"- Optymalizator: `{optimizer}`")
            best = payload.get("best_overall") or {}
            if isinstance(best, dict) and best:
                if "ns_per_unit" in best:
                    lines.append(f"- Najlepsze `ns_per_unit`: `{best.get('ns_per_unit')}`")
                if "variant" in best:
                    lines.append(f"- Najlepszy wariant: `{best.get('variant')}`")
                if "option_index" in best:
                    lines.append(f"- Najlepsza opcja: `{best.get('option_index')}`")
        lines.append("")
    lines.append("## Kluczowe artefakty")
    lines.append("")
    for key, value in sorted((summary.get("key_results") or {}).items()):
        lines.append(f"- `{key}`: `{value}`")
    lines.append("")
    return "\n".join(lines)


def _generate_campaign_plot(summary: Dict[str, Any], out_dir: Path) -> Dict[str, str]:
    try:
        import matplotlib
        matplotlib.use("Agg")
        import matplotlib.pyplot as plt  # type: ignore
    except Exception:
        return {}

    steps = summary.get("steps", []) or []
    if not steps:
        return {}

    labels = [str(step.get("label", step.get("id", ""))) for step in steps]
    elapsed = []
    colors = []
    status_texts = []
    for step in steps:
        status = str(step.get("status", "pending"))
        status_texts.append(status)
        try:
            value = float(step.get("elapsed_s", 0.0) or 0.0)
        except Exception:
            value = 0.0
        elapsed.append(max(value, 0.0))
        if status == "ok":
            colors.append("#15803d")
        elif status == "skipped":
            colors.append("#94a3b8")
        else:
            colors.append("#b91c1c")

    fig, axes = plt.subplots(2, 1, figsize=(14, 8), gridspec_kw={"height_ratios": [2.0, 1.2]})
    ax1, ax2 = axes

    ys = list(range(len(labels)))
    ax1.barh(ys, elapsed, color=colors, alpha=0.9)
    ax1.set_yticks(ys)
    ax1.set_yticklabels(labels, fontsize=9)
    ax1.invert_yaxis()
    ax1.set_xlabel("Czas kroku [s]")
    ax1.set_title("Pelna kampania Final: czasy wykonania krokow")
    ax1.grid(True, axis="x", alpha=0.25)
    for y, value, status in zip(ys, elapsed, status_texts):
        ax1.text(value + max(max(elapsed) * 0.01, 0.02), y, status, va="center", fontsize=8)

    counts = {
        "ok": sum(1 for s in status_texts if s == "ok"),
        "skipped": sum(1 for s in status_texts if s == "skipped"),
        "failed": sum(1 for s in status_texts if s not in {"ok", "skipped"}),
    }
    ax2.bar(list(counts.keys()), list(counts.values()), color=["#15803d", "#94a3b8", "#b91c1c"], alpha=0.9)
    ax2.set_title("Status kampanii")
    ax2.set_ylabel("Liczba krokow")
    ax2.grid(True, axis="y", alpha=0.25)

    fig.tight_layout()
    plot_path = out_dir / "plots" / "full_thesis_overview.png"
    fig.savefig(plot_path, dpi=180)
    plt.close(fig)
    return {"campaign_overview": str(plot_path)}


def _workflow_cmd(*args: str) -> List[str]:
    return [sys.executable, str(ROOT / "run_workflow.py"), *args]


def _gpu_backend_or_none(requested_backend: str, platform_profile: str, arch: str) -> str | None:
    req = requested_backend.strip().lower()
    if req == "cpu":
        return None
    resolved = _resolve_gpu_backend(requested_backend, platform_profile, arch)
    if not _backend_available(resolved):
        return None
    return resolved


def _run_step(
    *,
    campaign_dir: Path,
    steps: List[Dict[str, Any]],
    step_id: str,
    label: str,
    cmd: List[str] | None,
    reason: str = "",
    required: bool = False,
    on_update=None,
    pipeline_log_path: Path | None = None,
    pipeline_events_path: Path | None = None,
) -> Dict[str, Any]:
    step: Dict[str, Any] = {
        "id": step_id,
        "label": label,
        "required": bool(required),
        "command": cmd or [],
        "reason": reason,
    }
    if cmd is None:
        step["status"] = "skipped"
        step["elapsed_s"] = 0.0
        steps.append(step)
        if pipeline_log_path is not None:
            _append_pipeline_log(pipeline_log_path, f"{step_id}: skipped ({reason})")
        if pipeline_events_path is not None:
            _append_pipeline_event(
                pipeline_events_path,
                {
                    "step_name": step_id,
                    "label": label,
                    "status": "skipped",
                    "reason": reason,
                    "start_time": None,
                    "end_time": None,
                    "duration": 0.0,
                    "command": [],
                    "exit_code": None,
                    "stdout_path": None,
                    "stderr_path": None,
                    "artifacts_created": [],
                },
            )
        if on_update is not None:
            on_update()
        return step

    log_path = campaign_dir / "logs" / f"{_safe_slug(step_id)}.log"
    step["status"] = "running"
    step["elapsed_s"] = 0.0
    step["log_path"] = str(log_path)
    steps.append(step)
    if on_update is not None:
        on_update()
    started = time.time()
    started_iso = datetime.now(timezone.utc).isoformat()
    if pipeline_log_path is not None:
        _append_pipeline_log(pipeline_log_path, f"{step_id}: start -> {' '.join(cmd)}")
    rc, payload = _run_logged(cmd, log_path=log_path)
    elapsed = time.time() - started
    ended_iso = datetime.now(timezone.utc).isoformat()
    step["status"] = "ok" if rc == 0 else "failed"
    step["elapsed_s"] = elapsed
    step["exit_code"] = int(rc)
    step["payload"] = payload

    result_dir = ""
    if isinstance(payload, dict):
        result_dir = str(payload.get("out_dir") or payload.get("session_dir") or "").strip()
    if result_dir:
        step["result_dir"] = result_dir
        target = Path(result_dir).expanduser().resolve()
        if target.exists():
            _symlink_or_note(campaign_dir / "artifacts" / _safe_slug(step_id), target)
    if pipeline_log_path is not None:
        _append_pipeline_log(pipeline_log_path, f"{step_id}: {step['status']} rc={rc} elapsed={elapsed:.3f}s")
    if pipeline_events_path is not None:
        artifacts_created = [result_dir] if result_dir else []
        _append_pipeline_event(
            pipeline_events_path,
            {
                "step_name": step_id,
                "label": label,
                "status": step["status"],
                "reason": reason,
                "start_time": started_iso,
                "end_time": ended_iso,
                "duration": elapsed,
                "command": cmd,
                "exit_code": int(rc),
                "stdout_path": str(log_path),
                "stderr_path": str(log_path),
                "artifacts_created": artifacts_created,
            },
        )
    if on_update is not None:
        on_update()
    return step


def main() -> None:
    ap = argparse.ArgumentParser(description="Full thesis pipeline for Final.")
    ap.add_argument("--profile", choices=["quick", "paper", "full"], default="full")
    ap.add_argument("--experiment-profile", default="")
    ap.add_argument("--platform-profile", choices=["auto", "apple", "nvidia", "amd", "intel_arc", "intel_igpu"], default="auto")
    ap.add_argument("--arch", choices=["auto", "apple", "x86", "intel", "amd", "generic"], default="auto")
    ap.add_argument("--backend", choices=["auto", "cpu", "metal", "cuda", "hip", "opencl", "amd", "intel"], default="auto")
    ap.add_argument("--benchmark-mode", choices=["standard", "extended"], default="standard")
    ap.add_argument("--benchmarks-max-cpu-threads", type=int, default=0)
    ap.add_argument("--real-kernels-max-cpu-threads", type=int, default=0)
    ap.add_argument("--filip-max-cpu-threads", type=int, default=0)
    ap.add_argument("--device-index", type=int, default=0)
    ap.add_argument("--roofline-ai", type=float, default=8.0)
    ap.add_argument("--roofline-bytes", type=float, default=1_000_000_000.0)
    ap.add_argument("--real-runs", type=int, default=5)
    ap.add_argument("--repeats", type=int, default=5)
    ap.add_argument("--warmups", type=int, default=0)
    ap.add_argument("--trials", type=int, default=256)
    ap.add_argument("--population", type=int, default=24)
    ap.add_argument("--iterations", type=int, default=40)
    ap.add_argument("--filip-case", choices=["portable", "laplace_prism", "test_prism", "prism_pair"], default="prism_pair")
    ap.add_argument("--filip-modfem-dir", default="")
    ap.add_argument("--filip-input-override", default="")
    ap.add_argument("--filip-replay-dump-root", default="")
    ap.add_argument("--filip-limit-option-rows", type=int, default=0, help=argparse.SUPPRESS)
    ap.add_argument("--fem-option-validation-operators", default="laplace,test")
    ap.add_argument("--fem-option-validation-variants", default="qss,sqs,ssq")
    ap.add_argument("--fem-option-validation-n-elements", type=int, default=16384)
    ap.add_argument("--fem-option-validation-n-qp", type=int, default=6)
    ap.add_argument("--fem-option-validation-workgroup-size", type=int, default=64)
    ap.add_argument("--correlation-profiler-report", action="append", default=[])
    ap.add_argument("--google-drive-sync", choices=["off", "auto", "folder", "rclone"], default=default_sync_mode())
    ap.add_argument("--google-drive-dir", default=default_google_drive_dir())
    ap.add_argument("--google-drive-rclone-remote", default=default_rclone_remote())
    ap.add_argument("--google-drive-subdir", default=default_google_drive_subdir())
    ap.add_argument("--smoke", action="store_true", help=argparse.SUPPRESS)
    args = ap.parse_args()
    args = _apply_experiment_profile(args)

    forced_profile = "full"
    full_repeats = max(int(args.repeats), 5)
    full_real_runs = max(int(args.real_runs), 5)
    full_trials = max(int(args.trials), 256)
    full_population = max(int(args.population), 24)
    full_iterations = max(int(args.iterations), 40)
    validation_n_elements = max(int(args.fem_option_validation_n_elements), 16384)
    validation_n_qp = max(int(args.fem_option_validation_n_qp), 6)
    validation_workgroup_size = max(int(args.fem_option_validation_workgroup_size), 64)

    if bool(args.smoke):
        forced_profile = "quick"
        full_repeats = 1
        args.warmups = 1
        full_real_runs = 1
        full_trials = 4
        full_population = 4
        full_iterations = 2
        validation_n_elements = 64
        validation_n_qp = 2
        validation_workgroup_size = 32

    resolved_gpu_backend = _gpu_backend_or_none(args.backend, args.platform_profile, args.arch)
    resolved_fem_backend = _resolve_fem_backend_token(args.backend, args.platform_profile, args.arch)
    campaign_dir = _create_campaign_dir(profile=forced_profile, backend=(resolved_fem_backend or args.backend or "auto"))

    manifest = manifest_base(forced_profile)
    manifest.update(
        {
            "launcher": "run_full_thesis_pipeline.py",
            "workflow": "full_thesis_pipeline",
            "experiment_class": "thesis_full_campaign",
            "requested_profile": args.profile,
            "effective_profile": forced_profile,
            "requested_backend": args.backend,
            "resolved_gpu_backend": resolved_gpu_backend or "",
            "resolved_fem_backend": resolved_fem_backend,
            "benchmark_mode": args.benchmark_mode,
            "experiment_profile": str(getattr(args, "experiment_profile", "") or ""),
            "warmups": int(getattr(args, "warmups", 0) or 0),
            "platform_profile": args.platform_profile,
            "arch_request": args.arch,
            "device_index": int(args.device_index),
            "campaign_dir": str(campaign_dir),
        }
    )
    write_manifest(campaign_dir, manifest)
    pipeline_log_path = campaign_dir / "pipeline.log"
    pipeline_events_path = campaign_dir / "pipeline_events.jsonl"
    _append_pipeline_log(pipeline_log_path, f"campaign_start: {campaign_dir.name}")
    _append_pipeline_event(
        pipeline_events_path,
        {
            "event": "campaign_start",
            "run_id": campaign_dir.name,
            "profile": forced_profile,
            "experiment_profile": str(getattr(args, "experiment_profile", "") or ""),
            "benchmark_mode": args.benchmark_mode,
            "warmups": int(getattr(args, "warmups", 0) or 0),
        },
    )

    steps: List[Dict[str, Any]] = []
    def _write_live_state() -> None:
        summary = _live_summary(
            campaign_dir=campaign_dir,
            forced_profile=forced_profile,
            args=args,
            resolved_gpu_backend=resolved_gpu_backend,
            resolved_fem_backend=resolved_fem_backend,
            steps=steps,
            running=True,
        )
        _write_json(campaign_dir / "summary.json", summary)
        _write_json(campaign_dir / "steps.json", {"steps": summary.get("steps", [])})

    base_args = [
        "--profile", forced_profile,
        "--platform-profile", args.platform_profile,
        "--arch", args.arch,
        "--backend", args.backend,
        "--benchmark-mode", args.benchmark_mode,
        "--benchmarks-max-cpu-threads", str(int(args.benchmarks_max_cpu_threads)),
        "--real-kernels-max-cpu-threads", str(int(args.real_kernels_max_cpu_threads)),
        "--filip-max-cpu-threads", str(int(args.filip_max_cpu_threads)),
        "--device-index", str(int(args.device_index)),
        "--roofline-ai", str(float(args.roofline_ai)),
        "--roofline-bytes", str(float(args.roofline_bytes)),
        "--experiment-profile", str(getattr(args, "experiment_profile", "") or ""),
        "--warmups", str(max(int(getattr(args, "warmups", 0) or 0), 0)),
    ]

    _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="cpu_benchmark",
        label="CPU microbenchmarks",
        cmd=_workflow_cmd("--workflow", "cpu_benchmark", *base_args),
        required=True,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="gpu_benchmark",
        label="GPU microbenchmarks",
        cmd=_workflow_cmd("--workflow", "gpu_benchmark", *base_args) if resolved_gpu_backend else None,
        reason="Brak dostepnego backendu GPU na tej platformie." if resolved_gpu_backend is None else "",
        required=False,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="cpu_real_kernels",
        label="CPU real kernels",
        cmd=_workflow_cmd(
            "--workflow", "cpu_real_kernels", *base_args,
            "--real-runs", str(full_real_runs),
        ),
        required=True,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="gpu_real_kernels",
        label="GPU real kernels",
        cmd=_workflow_cmd(
            "--workflow", "gpu_real_kernels", *base_args,
            "--real-runs", str(full_real_runs),
        ) if resolved_gpu_backend else None,
        reason="Brak dostepnego backendu GPU na tej platformie." if resolved_gpu_backend is None else "",
        required=False,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="ai_accel",
        label="AI acceleration paths",
        cmd=_workflow_cmd(
            "--workflow", "ai_accel", *base_args,
            "--real-runs", str(full_real_runs),
        ),
        required=False,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    validation_step = _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="fem_option_validation",
        label="FEM option validation",
        cmd=_workflow_cmd(
            "--workflow", "fem_option_validation", *base_args,
            "--repeats", str(full_repeats),
            "--fem-option-validation-operators", args.fem_option_validation_operators,
            "--fem-option-validation-variants", args.fem_option_validation_variants,
            "--fem-option-validation-n-elements", str(validation_n_elements),
            "--fem-option-validation-n-qp", str(validation_n_qp),
            "--fem-option-validation-workgroup-size", str(validation_workgroup_size),
        ),
        required=True,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    portable_step = _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="filip_original_portable",
        label="Filip original portable sweep",
        cmd=_workflow_cmd(
            "--workflow", "filip_original", *base_args,
            "--repeats", str(full_repeats),
            "--filip-case", args.filip_case,
            "--filip-mode", "portable_sweep",
        ),
        required=True,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    autotune_step = _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="filip_autotune",
        label="Filip autotuning (random search)",
        cmd=_workflow_cmd(
            "--workflow", "filip_autotune", *base_args,
            "--repeats", str(full_repeats),
            "--trials", str(full_trials),
        ),
        required=True,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    firefly_step = _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="filip_firefly",
        label="Filip autotuning (firefly)",
        cmd=_workflow_cmd(
            "--workflow", "filip_firefly", *base_args,
            "--repeats", str(full_repeats),
            "--population", str(full_population),
            "--iterations", str(full_iterations),
        ),
        required=True,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    exact_cmd: List[str] | None = None
    exact_reason = ""
    replay_root = str(args.filip_replay_dump_root).strip()
    if sys.platform == "darwin":
        exact_cmd = _workflow_cmd(
            "--workflow", "filip_original", *base_args,
            "--filip-case", args.filip_case,
            "--filip-mode", "exact_reference",
            "--backend", "metal",
        )
        if replay_root:
            exact_cmd += ["--filip-replay-dump-root", replay_root]
        if int(getattr(args, "filip_limit_option_rows", 0) or 0) > 0:
            exact_cmd += ["--filip-limit-option-rows", str(int(args.filip_limit_option_rows))]
    else:
        if _backend_available("opencl"):
            exact_cmd = _workflow_cmd(
                "--workflow", "filip_original", *base_args,
                "--backend", "opencl",
                "--repeats", str(full_repeats),
                "--filip-case", args.filip_case,
                "--filip-mode", "exact_reference",
                "--filip-dump-launch-artifacts",
                "--filip-export-replay-inputs",
                "--filip-export-replay-include-expected-output",
                "--filip-export-canonical-replay-bundles",
            )
            if str(args.filip_modfem_dir).strip():
                exact_cmd += ["--filip-modfem-dir", str(args.filip_modfem_dir).strip()]
            if str(args.filip_input_override).strip():
                exact_cmd += ["--filip-input-override", str(args.filip_input_override).strip()]
            if int(getattr(args, "filip_limit_option_rows", 0) or 0) > 0:
                exact_cmd += ["--filip-limit-option-rows", str(int(args.filip_limit_option_rows))]
        else:
            exact_reason = "Brak dostepnego backendu OpenCL dla exact reference na tej platformie."

    exact_step = _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="filip_exact_reference",
        label="Filip exact reference / replay",
        cmd=exact_cmd,
        reason=exact_reason,
        required=False,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    correlation_optimization_dir = ""
    for candidate in (exact_step, portable_step):
        result_dir = str(candidate.get("result_dir", "")).strip()
        if result_dir:
            correlation_optimization_dir = result_dir
            break
    correlation_validation_dir = str(validation_step.get("result_dir", "")).strip()
    correlation_cmd: List[str] | None = None
    correlation_reason = ""
    if correlation_optimization_dir and correlation_validation_dir:
        correlation_cmd = _workflow_cmd(
            "--workflow", "profiler_correlation",
            "--correlation-optimization-dir", correlation_optimization_dir,
            "--correlation-fem-option-validation-dir", correlation_validation_dir,
            "--correlation-out-dir", str((Path(correlation_optimization_dir) / "profiler_correlation").resolve()),
        )
        for report in args.correlation_profiler_report:
            report_str = str(report).strip()
            if report_str:
                correlation_cmd += ["--correlation-profiler-report", report_str]
    else:
        correlation_reason = "Brak optimization_dir lub fem_option_validation_dir do zbudowania korelacji profilerowej."

    correlation_step = _run_step(
        campaign_dir=campaign_dir,
        steps=steps,
        step_id="profiler_correlation",
        label="Profiler correlation",
        cmd=correlation_cmd,
        reason=correlation_reason,
        required=False,
        on_update=_write_live_state,
        pipeline_log_path=pipeline_log_path,
        pipeline_events_path=pipeline_events_path,
    )

    required_failures = [step.get("id", "") for step in steps if bool(step.get("required")) and step.get("status") != "ok"]
    summary: Dict[str, Any] = _live_summary(
        campaign_dir=campaign_dir,
        forced_profile=forced_profile,
        args=args,
        resolved_gpu_backend=resolved_gpu_backend,
        resolved_fem_backend=resolved_fem_backend,
        steps=steps,
        running=False,
    )
    summary["plots"] = _generate_campaign_plot(summary, campaign_dir)
    try:
        contract_info = standardize_campaign_artifacts(campaign_dir=campaign_dir, summary=summary)
        summary["contracts"] = contract_info
    except Exception as exc:
        summary["contracts_error"] = str(exc)
    summary["summary_hash"] = sha256_json(summary)
    summary["exit_code"] = 0 if not required_failures else 1

    _write_json(campaign_dir / "summary.json", summary)
    _write_json(campaign_dir / "steps.json", {"steps": summary.get("steps", [])})
    (campaign_dir / "campaign.md").write_text(_build_campaign_markdown(summary), encoding="utf-8")

    sync_mode = str(getattr(args, "google_drive_sync", "") or default_sync_mode()).strip().lower()
    sync_info: dict[str, Any] = {}
    if sync_mode not in {"", "off", "disabled", "none"}:
        sync_info = sync_artifacts_to_google_drive(
            source_dir=campaign_dir,
            mode=sync_mode,
            google_drive_dir=str(getattr(args, "google_drive_dir", "") or default_google_drive_dir()),
            rclone_remote=str(getattr(args, "google_drive_rclone_remote", "") or default_rclone_remote()),
            subdir=str(getattr(args, "google_drive_subdir", "") or default_google_drive_subdir()),
            root=ROOT,
        )
    if sync_info:
        summary["google_drive_sync"] = sync_info
        contracts = summary.get("contracts")
        if isinstance(contracts, dict):
            contracts["google_drive_sync"] = sync_info
        _write_json(campaign_dir / "summary.json", summary)

    _append_pipeline_log(pipeline_log_path, f"campaign_end: exit_code={summary['exit_code']}")
    _append_pipeline_event(
        pipeline_events_path,
        {
            "event": "campaign_end",
            "run_id": campaign_dir.name,
            "exit_code": int(summary["exit_code"]),
            "required_failures": list(required_failures),
            "contracts": summary.get("contracts", {}),
            "google_drive_sync": sync_info,
        },
    )

    print("\n=== WORKFLOW DONE ===")
    print(json.dumps(summary, indent=2, ensure_ascii=True, default=_json_default))
    raise SystemExit(int(summary["exit_code"]))


if __name__ == "__main__":
    main()
