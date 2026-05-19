#!/usr/bin/env python3
from __future__ import annotations

import json
from datetime import datetime
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parents[1]
THESIS_DIR = ROOT / "data" / "thesis_full"
OPT_DIR = ROOT / "data" / "optimization"
PLOTS_DIR = ROOT / "analysis" / "figures" / "thesis_core"
DOCS_DIR = ROOT / "docs"


def _read_json(path: Path) -> dict[str, Any]:
    return json.loads(path.read_text(encoding="utf-8"))


def _latest_dir(root: Path, pattern: str) -> Path | None:
    if not root.exists():
        return None
    dirs = sorted([p for p in root.glob(pattern) if p.is_dir()], key=lambda p: p.stat().st_mtime, reverse=True)
    return dirs[0] if dirs else None


def _latest_stable_thesis_dir() -> Path | None:
    if not THESIS_DIR.exists():
        return None
    dirs = sorted([p for p in THESIS_DIR.glob("*__full_thesis__*") if p.is_dir()], key=lambda p: p.stat().st_mtime, reverse=True)
    for path in dirs:
        summary_path = path / "summary.json"
        if not summary_path.exists():
            continue
        try:
            summary = _read_json(summary_path)
        except Exception:
            continue
        if not bool(summary.get("running", False)):
            return path
    return None


def _status_line(ok: bool, text: str) -> str:
    return f"- [{'OK' if ok else 'BRAK'}] {text}"


def main() -> None:
    report_lines: list[str] = []
    report_lines.append("# Raport zdrowia v4")
    report_lines.append("")
    report_lines.append(f"- Czas: `{datetime.now().isoformat(timespec='seconds')}`")
    report_lines.append(f"- Root: `{ROOT}`")
    report_lines.append("")

    docs_required = [
        DOCS_DIR / "PIPELINE_BADAWCZY.md",
        DOCS_DIR / "GRAPHICAL_PIPELINE.md",
        DOCS_DIR / "V4_FULL_PIPELINE.md",
        DOCS_DIR / "THESIS_RESEARCH_PLAN.md",
    ]
    report_lines.append("## Dokumentacja")
    for path in docs_required:
        report_lines.append(_status_line(path.exists(), str(path.relative_to(ROOT))))
    report_lines.append("")

    latest_campaign = _latest_dir(THESIS_DIR, "*__full_thesis__*")
    stable_campaign = _latest_stable_thesis_dir()
    report_lines.append("## Ostatnia pelna kampania")
    if latest_campaign is None:
        report_lines.append("- [BRAK] Nie znaleziono pelnej kampanii `thesis_full`.")
    else:
        report_lines.append(f"- Ostatni katalog: `{latest_campaign.name}`")
        if stable_campaign is not None:
            report_lines.append(f"- Ostatnia stabilna kampania: `{stable_campaign.name}`")
        summary_path = latest_campaign / "summary.json"
        steps_path = latest_campaign / "steps.json"
        campaign_md = latest_campaign / "campaign.md"
        report_lines.append(_status_line(summary_path.exists(), "summary.json"))
        report_lines.append(_status_line(steps_path.exists(), "steps.json"))
        if summary_path.exists():
            try:
                summary = _read_json(summary_path)
                is_running = bool(summary.get("running", False))
            except Exception:
                is_running = False
        else:
            is_running = False
        campaign_md_ok = campaign_md.exists() or is_running
        report_lines.append(_status_line(campaign_md_ok, "campaign.md" + (" (moze pojawic sie po zakonczeniu kampanii)" if is_running else "")))
        if summary_path.exists():
            try:
                summary = _read_json(summary_path)
                report_lines.append(f"- Running: `{summary.get('running')}`")
                report_lines.append(f"- Critical success: `{summary.get('critical_success')}`")
                report_lines.append(f"- Exit code: `{summary.get('exit_code')}`")
                steps = summary.get("steps", []) or []
                report_lines.append(f"- Liczba krokow zapisanych w summary: `{len(steps)}`")
                for step in steps:
                    report_lines.append(
                        f"  - `{step.get('id')}` -> `{step.get('status')}`"
                    )
            except Exception as exc:
                report_lines.append(f"- [BRAK] Nie udalo sie odczytac summary: `{exc}`")
    report_lines.append("")

    report_lines.append("## Wykresy zbiorcze")
    key_plots = [
        "cpu_memcpy_bandwidth_scaling.png",
        "cpu_stream_triad_scaling.png",
        "cpu_peak_compute_scaling.png",
        "cpu_memory_latency_hierarchy.png",
        "gpu_microbenchmark_suite.png",
        "platform_roofline_measured.png",
        "real_kernels_model_validation.png",
        "real_kernels_filip_contrast_map.png",
    ]
    for name in key_plots:
        report_lines.append(_status_line((PLOTS_DIR / name).exists(), f"analysis/figures/thesis_core/{name}"))
    report_lines.append(_status_line((ROOT / "analysis" / "figures" / "manifests" / "thesis_core_manifest.json").exists(), "analysis/figures/manifests/thesis_core_manifest.json"))
    report_lines.append("")

    latest_filip = _latest_dir(OPT_DIR, "*__filip_original__backend-*")
    report_lines.append("## Ostatni run kodu Filipa")
    if latest_filip is None:
        report_lines.append("- [BRAK] Nie znaleziono runu `filip_original`.")
    else:
        report_lines.append(f"- Katalog: `{latest_filip.name}`")
        plots_dir = latest_filip / "figures" / "thesis_core"
        appendix_dir = latest_filip / "figures" / "appendix"
        expected = [
            "filip_variant_qss.png",
            "filip_variant_sqs.png",
            "filip_variant_ssq.png",
            "filip_autotuning_trace.png",
            "filip_best_summary.png",
            "filip_memory_compute_breakdown.png",
        ]
        for name in expected:
            report_lines.append(_status_line((plots_dir / name).exists(), f"{latest_filip.name}/figures/thesis_core/{name}"))
        report_lines.append(_status_line((appendix_dir / "filip_best_configuration_card.png").exists(), f"{latest_filip.name}/figures/appendix/filip_best_configuration_card.png"))
        report_lines.append(_status_line((latest_filip / "figures" / "manifests" / "filip_figures_manifest.json").exists(), f"{latest_filip.name}/figures/manifests/filip_figures_manifest.json"))
    report_lines.append("")

    report = "\n".join(report_lines) + "\n"
    out_dir = ROOT / "data" / "health"
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / "v4_health_report.md"
    out_path.write_text(report, encoding="utf-8")
    print(report)
    print(f"[OK] Zapisano raport: {out_path}")


if __name__ == "__main__":
    main()
