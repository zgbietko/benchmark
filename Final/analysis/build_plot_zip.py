#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import io
import json
import re
import sys
import unicodedata
from datetime import datetime, timezone
from pathlib import Path
from zipfile import ZIP_DEFLATED, ZipFile

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in sys.path:
    sys.path.insert(0, str(ROOT))

from analysis.publication_style import APPENDIX_DIR, MANIFEST_DIR, THESIS_CORE_DIR

OPT_DIR = ROOT / "data" / "optimization"
THESIS_FULL_DIR = ROOT / "data" / "thesis_full"
GLOBAL_ARTIFACTS_DIR = ROOT / "data" / "artifacts" / "plot_bundles"
RUN_TS_RE = re.compile(r"(\d{8}_\d{6})")
CSV_SKIP_DIRS = {
    ".git",
    ".hg",
    ".svn",
    "__pycache__",
    "venv",
    ".venv",
    "node_modules",
    "figures",
    "plots",
    "artifacts",
    "contracts",
}
MAX_LOG_LINES_PER_FILE = 2500
CSV_SKIP_BASENAMES = {
    "el_data_out.csv",
}


def _read_json(path: Path) -> dict:
    return json.loads(path.read_text(encoding="utf-8"))


def _latest_stable_campaign_dir() -> Path | None:
    if not THESIS_FULL_DIR.exists():
        return None
    candidates = sorted((p for p in THESIS_FULL_DIR.iterdir() if p.is_dir()), key=lambda p: p.stat().st_mtime, reverse=True)
    for candidate in candidates:
        summary_path = candidate / "summary.json"
        if not summary_path.exists():
            continue
        try:
            summary = _read_json(summary_path)
        except Exception:
            continue
        if bool(summary.get("running", False)):
            continue
        return candidate
    return candidates[0] if candidates else None


def _find_step(summary: dict, step_id: str) -> dict | None:
    for step in summary.get("steps", []) or []:
        if str(step.get("id", "")) == step_id:
            return step
    return None


def _latest_filip_run_dir() -> Path | None:
    if not OPT_DIR.exists():
        return None
    runs = [p for p in OPT_DIR.iterdir() if p.is_dir() and "__filip_original__backend-" in p.name]
    if not runs:
        return None
    return max(runs, key=lambda p: p.stat().st_mtime)


def _filip_run_dir_for_campaign(campaign_dir: Path | None) -> Path | None:
    if campaign_dir is not None:
        summary_path = campaign_dir / "summary.json"
        if summary_path.exists():
            try:
                summary = _read_json(summary_path)
            except Exception:
                summary = {}
            portable = _find_step(summary, "filip_original_portable") or {}
            result_dir = str(portable.get("result_dir") or "").strip()
            if result_dir:
                run_dir = Path(result_dir).expanduser().resolve()
                if run_dir.exists():
                    return run_dir
    return _latest_filip_run_dir()


def _artifact_dir_for_campaign(campaign_dir: Path | None) -> Path:
    if campaign_dir is not None:
        artifacts = campaign_dir / "artifacts"
        artifacts.mkdir(parents=True, exist_ok=True)
        return artifacts
    GLOBAL_ARTIFACTS_DIR.mkdir(parents=True, exist_ok=True)
    return GLOBAL_ARTIFACTS_DIR


def _read_campaign_summary(campaign_dir: Path | None) -> dict:
    if campaign_dir is None:
        return {}
    summary_path = campaign_dir / "summary.json"
    if not summary_path.exists():
        return {}
    try:
        return _read_json(summary_path)
    except Exception:
        return {}


def _coerce_dir(path_like: object) -> Path | None:
    if not isinstance(path_like, str):
        return None
    value = path_like.strip()
    if not value:
        return None
    path = Path(value).expanduser().resolve()
    return path if path.exists() and path.is_dir() else None


def _collect_result_dirs(campaign_dir: Path | None) -> list[Path]:
    summary = _read_campaign_summary(campaign_dir)
    dirs: list[Path] = []
    for step in summary.get("steps", []) or []:
        if not isinstance(step, dict):
            continue
        direct = _coerce_dir(step.get("result_dir"))
        if direct is not None:
            dirs.append(direct)
        payload = step.get("payload")
        if isinstance(payload, dict):
            for key in ("session_dir", "result_dir", "out_dir", "output_dir"):
                nested = _coerce_dir(payload.get(key))
                if nested is not None:
                    dirs.append(nested)
    key_results = summary.get("key_results")
    if isinstance(key_results, dict):
        for value in key_results.values():
            nested = _coerce_dir(value)
            if nested is not None:
                dirs.append(nested)
    unique: list[Path] = []
    seen: set[str] = set()
    for path in dirs:
        resolved = str(path.resolve())
        if resolved in seen:
            continue
        seen.add(resolved)
        unique.append(path)
    return unique


def _slugify(text: str, *, default: str) -> str:
    normalized = unicodedata.normalize("NFKD", text)
    ascii_text = normalized.encode("ascii", "ignore").decode("ascii")
    slug = re.sub(r"[^A-Za-z0-9]+", "_", ascii_text).strip("_").lower()
    return slug or default


def _run_tokens(paths: list[Path]) -> list[str]:
    tokens: set[str] = set()
    for path in paths:
        for part in path.parts:
            for token in RUN_TS_RE.findall(part):
                tokens.add(token)
    return sorted(tokens)


def _platform_token(campaign_dir: Path | None, result_dirs: list[Path]) -> str:
    summary = _read_campaign_summary(campaign_dir)
    preferred: list[str] = []
    for root in result_dirs:
        env_path = root / "environment_manifest.json"
        if not env_path.exists():
            continue
        try:
            env = _read_json(env_path)
        except Exception:
            continue
        cpu = str(env.get("cpu_model", "")).strip()
        gpu = str(env.get("gpu_model", "")).strip()
        if cpu:
            preferred.append(cpu)
        if gpu:
            preferred.append(gpu)
        if preferred:
            break
    raw = ""
    if preferred:
        raw = "_".join(preferred[:2])
    if not raw:
        raw = str(summary.get("platform_profile") or "").strip()
    if not raw:
        raw = str(summary.get("resolved_gpu_backend") or summary.get("resolved_fem_backend") or "").strip()
    return _slugify(raw, default="unknown_platform")


def _flatten_name(value: str) -> str:
    cleaned = re.sub(r"[^A-Za-z0-9._-]+", "_", value.strip())
    return cleaned.strip("_") or "file"


def _flat_rel_path(root: Path, file_path: Path) -> str:
    rel = file_path.relative_to(root)
    parts = [_flatten_name(part) for part in rel.parts]
    return "__".join(parts)


def _collect_result_csv_files(result_dirs: list[Path]) -> list[tuple[Path, Path]]:
    csv_files: list[tuple[Path, Path]] = []
    for root in result_dirs:
        for file_path in root.rglob("*.csv"):
            if file_path.name.lower() in CSV_SKIP_BASENAMES:
                continue
            rel_parts = file_path.relative_to(root).parts
            if any(part in CSV_SKIP_DIRS for part in rel_parts):
                continue
            csv_files.append((root, file_path))
    return csv_files


def _collect_campaign_logs(campaign_dir: Path | None) -> list[Path]:
    if campaign_dir is None:
        return []
    logs_dir = campaign_dir / "logs"
    if not logs_dir.exists():
        return []
    return sorted(p for p in logs_dir.glob("*.log") if p.is_file())


def _build_combined_log(campaign_dir: Path | None, summary: dict) -> str:
    lines: list[str] = []
    lines.append("# Pipeline campaign log (combined)")
    lines.append(f"created_utc: {datetime.now(timezone.utc).isoformat()}")
    lines.append(f"campaign_dir: {campaign_dir.resolve() if campaign_dir else ''}")
    lines.append("")
    steps = summary.get("steps", []) if isinstance(summary, dict) else []
    if isinstance(steps, list):
        lines.append("## Steps summary")
        for step in steps:
            if not isinstance(step, dict):
                continue
            sid = str(step.get("id", ""))
            status = str(step.get("status", ""))
            elapsed = step.get("elapsed_s", "")
            lines.append(f"- {sid}: {status}, elapsed_s={elapsed}")
        lines.append("")

    logs = _collect_campaign_logs(campaign_dir)
    if not logs:
        lines.append("No campaign log files found.")
        return "\n".join(lines) + "\n"

    lines.append("## Step logs")
    for log_path in logs:
        lines.append("")
        lines.append(f"===== {log_path.name} =====")
        try:
            text = log_path.read_text(encoding="utf-8", errors="replace")
        except Exception as exc:
            lines.append(f"[read_error] {type(exc).__name__}: {exc}")
            continue
        log_lines = text.splitlines()
        if len(log_lines) > MAX_LOG_LINES_PER_FILE:
            lines.append(
                f"[truncated] keeping first {MAX_LOG_LINES_PER_FILE} lines out of {len(log_lines)} total lines."
            )
            log_lines = log_lines[:MAX_LOG_LINES_PER_FILE]
        lines.extend(log_lines)
    return "\n".join(lines) + "\n"


def _unique_arcname(used: set[str], desired: str) -> str:
    if desired not in used:
        used.add(desired)
        return desired
    base, dot, ext = desired.partition(".")
    idx = 2
    while True:
        candidate = f"{base}__{idx}.{ext}" if dot else f"{base}__{idx}"
        if candidate not in used:
            used.add(candidate)
            return candidate
        idx += 1


def _render_csv(rows: list[dict[str, object]], fieldnames: list[str]) -> str:
    buf = io.StringIO()
    writer = csv.DictWriter(buf, fieldnames=fieldnames)
    writer.writeheader()
    for row in rows:
        writer.writerow({k: row.get(k, "") for k in fieldnames})
    return buf.getvalue()


def _render_manifest_txt(payload: dict[str, object]) -> str:
    lines = [
        "ZIP MANIFEST",
        "",
        f"created_at_utc: {payload.get('created_at_utc', '')}",
        f"campaign_dir: {payload.get('campaign_dir', '')}",
        f"platform: {payload.get('platform', '')}",
        f"include_appendix: {payload.get('include_appendix', False)}",
        "",
        "COUNTS",
        f"analysis_plot_count: {payload.get('analysis_plot_count', 0)}",
        f"filip_plot_count: {payload.get('filip_plot_count', 0)}",
        f"result_csv_count: {payload.get('result_csv_count', 0)}",
        "",
        "LAYOUT",
        "figures/ -> wszystkie PNG",
        "csv/results_all.csv -> wszystkie wyniki CSV w formie skonsolidowanej",
        "csv/results_sources.csv -> mapa zrodlowych CSV i liczby wierszy",
        "logs/pipeline.log -> jeden scalony log",
        "INDEX.csv -> mapa wszystkich plikow w ZIP",
        "MANIFEST.csv -> metadane kampanii",
    ]
    return "\n".join(lines) + "\n"


def _render_readme_md(payload: dict[str, object]) -> str:
    created = str(payload.get("created_at_utc", ""))
    campaign_name = str(payload.get("campaign_name", ""))
    platform = str(payload.get("platform", ""))
    runs_label = str(payload.get("runs_label", ""))
    analysis_plot_count = int(payload.get("analysis_plot_count", 0))
    filip_plot_count = int(payload.get("filip_plot_count", 0))
    result_csv_count = int(payload.get("result_csv_count", 0))
    total_plots = analysis_plot_count + filip_plot_count
    return "\n".join(
        [
            "# AI Analysis Pack",
            "",
            "This ZIP is prepared for cross-model AI analysis (NotebookLM, ChatGPT, Claude, Gemini, etc.).",
            "",
            "## Summary",
            f"- created_at_utc: {created}",
            f"- campaign_name: {campaign_name}",
            f"- platform: {platform}",
            f"- runs_label: {runs_label}",
            f"- plots_total: {total_plots}",
            f"- csv_total: {result_csv_count}",
            "",
            "## Folder layout",
            "- `figures/` -> PNG charts",
            "- `csv/results_all.csv` -> all source CSV rows merged into one table",
            "- `csv/results_sources.csv` -> source CSV map (file-level metadata)",
            "- `logs/pipeline.log` -> single merged log",
            "- `INDEX.csv` -> complete map of files in this ZIP",
            "- `MANIFEST.csv` -> one-row metadata summary",
            "- `MANIFEST.txt` -> human-readable manifest",
            "",
            "## Recommended AI workflow",
            "1. Read `MANIFEST.csv` first (campaign metadata).",
            "2. Read `INDEX.csv` and build a file map by `section` and `kind`.",
            "3. Analyze `csv/results_all.csv` for metrics/statistics/trends.",
            "4. Use `figures/` as visual confirmation of numeric findings.",
            "5. Use `logs/pipeline.log` to detect warnings, skipped steps, or runtime anomalies.",
            "",
            "## Universal prompt template",
            "```text",
            "Analyze this benchmark package using MANIFEST.csv and INDEX.csv first.",
            "Then:",
            "1) build a table of all files: path, role, key metrics;",
            "2) summarize top performance findings per backend/platform;",
            "3) detect anomalies and inconsistencies;",
            "4) list potential methodological risks;",
            "5) propose 5 publication-grade conclusions supported by specific csv/figure evidence.",
            "```",
            "",
        ]
    ) + "\n"


def _build_result_dir_step_map(summary: dict) -> dict[str, str]:
    mapping: dict[str, str] = {}
    steps = summary.get("steps", []) if isinstance(summary, dict) else []
    if not isinstance(steps, list):
        return mapping
    for step in steps:
        if not isinstance(step, dict):
            continue
        step_id = str(step.get("id", "")).strip()
        if not step_id:
            continue
        direct = _coerce_dir(step.get("result_dir"))
        if direct is not None:
            mapping[str(direct.resolve())] = step_id
        payload = step.get("payload")
        if isinstance(payload, dict):
            for key in ("session_dir", "result_dir", "out_dir", "output_dir"):
                nested = _coerce_dir(payload.get(key))
                if nested is not None:
                    mapping[str(nested.resolve())] = step_id
    return mapping


def _detect_section(rel_path: str) -> str:
    rel = rel_path.replace("\\", "/").strip("/")
    if not rel:
        return "root"
    return rel.split("/", 1)[0]


def _build_consolidated_csv_payloads(
    result_csv: list[tuple[Path, Path]],
    step_map: dict[str, str],
) -> tuple[str, str, int, int, int]:
    all_rows: list[dict[str, object]] = []
    src_rows: list[dict[str, object]] = []
    total_data_rows = 0
    parse_errors = 0

    for root, file_path in sorted(result_csv, key=lambda item: (item[0].name, str(item[1]))):
        rel = str(file_path.relative_to(root))
        section = _detect_section(rel)
        root_resolved = str(root.resolve())
        step_id = step_map.get(root_resolved, "")
        row_count = 0
        columns = ""
        status = "ok"
        error_msg = ""

        try:
            text = file_path.read_text(encoding="utf-8", errors="replace")
            reader = csv.DictReader(io.StringIO(text))
            if reader.fieldnames:
                fieldnames = [str(name) if name is not None else "" for name in reader.fieldnames]
                columns = "|".join(fieldnames)
                for idx, row in enumerate(reader, start=1):
                    normalized: dict[str, str] = {}
                    for key, value in row.items():
                        k = str(key).strip() if key is not None else ""
                        if not k:
                            k = "_unnamed"
                        normalized[k] = "" if value is None else str(value)
                    all_rows.append(
                        {
                            "step_id": step_id,
                            "source_run": root.name,
                            "section": section,
                            "source_rel_path": rel,
                            "source_file": file_path.name,
                            "row_index": idx,
                            "columns": columns,
                            "row_json": json.dumps(normalized, ensure_ascii=False),
                        }
                    )
                    row_count += 1
            else:
                plain_reader = csv.reader(io.StringIO(text))
                for idx, row in enumerate(plain_reader, start=1):
                    if not row:
                        continue
                    normalized = {f"c{i + 1}": str(cell) for i, cell in enumerate(row)}
                    if not columns:
                        columns = "|".join(normalized.keys())
                    all_rows.append(
                        {
                            "step_id": step_id,
                            "source_run": root.name,
                            "section": section,
                            "source_rel_path": rel,
                            "source_file": file_path.name,
                            "row_index": idx,
                            "columns": columns,
                            "row_json": json.dumps(normalized, ensure_ascii=False),
                        }
                    )
                    row_count += 1
        except Exception as exc:
            status = "error"
            error_msg = f"{type(exc).__name__}: {exc}"
            parse_errors += 1

        total_data_rows += row_count
        src_rows.append(
            {
                "step_id": step_id,
                "source_run": root.name,
                "section": section,
                "source_rel_path": rel,
                "source_file": file_path.name,
                "file_size_bytes": file_path.stat().st_size,
                "rows": row_count,
                "status": status,
                "error": error_msg,
                "source_path": str(file_path.resolve()),
            }
        )

    all_csv = _render_csv(
        all_rows,
        [
            "step_id",
            "source_run",
            "section",
            "source_rel_path",
            "source_file",
            "row_index",
            "columns",
            "row_json",
        ],
    )
    src_csv = _render_csv(
        src_rows,
        [
            "step_id",
            "source_run",
            "section",
            "source_rel_path",
            "source_file",
            "file_size_bytes",
            "rows",
            "status",
            "error",
            "source_path",
        ],
    )
    return all_csv, src_csv, len(src_rows), total_data_rows, parse_errors


def _run_figure_dirs(run_dir: Path | None) -> tuple[Path | None, Path | None, Path | None]:
    if run_dir is None:
        return None, None, None
    thesis_core = run_dir / "figures" / "thesis_core"
    appendix = run_dir / "figures" / "appendix"
    manifest_dir = run_dir / "figures" / "manifests"
    manifest = manifest_dir / "filip_figures_manifest.json"
    if not manifest.exists():
        manifest = manifest_dir / "article_plots_summary.json"
    return (
        thesis_core if thesis_core.exists() else None,
        appendix if appendix.exists() else None,
        manifest if manifest.exists() else None,
    )


def _collect_files(campaign_dir: Path | None, *, include_appendix: bool) -> dict[str, list[Path] | Path | None]:
    filip_run_dir = _filip_run_dir_for_campaign(campaign_dir)
    filip_core_dir, filip_appendix_dir, filip_manifest = _run_figure_dirs(filip_run_dir)
    result_dirs = _collect_result_dirs(campaign_dir)
    if filip_run_dir is not None and filip_run_dir.exists():
        run_resolved = str(filip_run_dir.resolve())
        existing = {str(p.resolve()) for p in result_dirs}
        if run_resolved not in existing:
            result_dirs.append(filip_run_dir)
    csv_files = _collect_result_csv_files(result_dirs)
    return {
        "global_core": sorted(p for p in THESIS_CORE_DIR.glob("*.png") if p.is_file()),
        "global_appendix": sorted(p for p in APPENDIX_DIR.glob("*.png") if p.is_file()) if include_appendix else [],
        "global_manifest": (MANIFEST_DIR / "thesis_core_manifest.json") if (MANIFEST_DIR / "thesis_core_manifest.json").exists() else None,
        "filip_core": sorted(p for p in (filip_core_dir.glob("*.png") if filip_core_dir else []) if p.is_file()),
        "filip_appendix": sorted(p for p in (filip_appendix_dir.glob("*.png") if include_appendix and filip_appendix_dir else []) if p.is_file()),
        "filip_manifest": filip_manifest,
        "filip_run_dir": filip_run_dir,
        "result_dirs": result_dirs,
        "result_csv": csv_files,
    }


def build_plot_zip(*, campaign_dir: Path | None = None, out_zip: Path | None = None, include_appendix: bool = True) -> dict[str, object]:
    files = _collect_files(campaign_dir, include_appendix=include_appendix)
    global_core = list(files["global_core"])  # type: ignore[arg-type]
    global_appendix = list(files["global_appendix"])  # type: ignore[arg-type]
    filip_core = list(files["filip_core"])  # type: ignore[arg-type]
    filip_appendix = list(files["filip_appendix"])  # type: ignore[arg-type]
    global_manifest = files["global_manifest"]
    filip_manifest = files["filip_manifest"]
    filip_run_dir = files["filip_run_dir"]
    result_dirs = list(files["result_dirs"])  # type: ignore[arg-type]
    result_csv = list(files["result_csv"])  # type: ignore[arg-type]

    if not global_core and not filip_core and not global_appendix and not filip_appendix:
        raise SystemExit("Brak figur do spakowania.")

    output_dir = _artifact_dir_for_campaign(campaign_dir)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    if out_zip is None:
        stem = campaign_dir.name if campaign_dir is not None else "latest"
        run_tokens = _run_tokens(result_dirs + ([campaign_dir] if campaign_dir is not None else []))
        if run_tokens:
            run_label = run_tokens[0] if len(run_tokens) == 1 else f"{run_tokens[0]}-{run_tokens[-1]}"
        else:
            run_label = "unknown"
        platform = _platform_token(campaign_dir, result_dirs)
        out_zip = output_dir / f"plots_bundle__runs-{run_label}__platform-{platform}__{stem}__{timestamp}.zip"
    out_zip.parent.mkdir(parents=True, exist_ok=True)
    summary = _read_campaign_summary(campaign_dir)
    combined_log = _build_combined_log(campaign_dir, summary)
    run_tokens = _run_tokens(result_dirs + ([campaign_dir] if campaign_dir is not None else []))
    run_label = "unknown"
    if run_tokens:
        run_label = run_tokens[0] if len(run_tokens) == 1 else f"{run_tokens[0]}-{run_tokens[-1]}"
    platform = _platform_token(campaign_dir, result_dirs)
    campaign_name = campaign_dir.name if campaign_dir is not None else "latest"
    step_map = _build_result_dir_step_map(summary)
    results_all_csv, results_sources_csv, raw_csv_count, raw_row_count, raw_parse_errors = _build_consolidated_csv_payloads(
        result_csv,
        step_map,
    )
    bundled_csv_count = 2

    manifest = {
        "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "campaign_dir": str(campaign_dir.resolve()) if campaign_dir else "",
        "campaign_name": campaign_name,
        "platform": platform,
        "runs_label": run_label,
        "global_thesis_core_dir": str(THESIS_CORE_DIR.resolve()),
        "global_appendix_dir": str(APPENDIX_DIR.resolve()),
        "filip_run_dir": str(filip_run_dir.resolve()) if isinstance(filip_run_dir, Path) else "",
        "include_appendix": include_appendix,
        "global_core_count": len(global_core),
        "global_appendix_count": len(global_appendix),
        "filip_core_count": len(filip_core),
        "filip_appendix_count": len(filip_appendix),
        "result_dirs_count": len(result_dirs),
        "result_csv_count": bundled_csv_count,
        "result_csv_raw_count": raw_csv_count,
        "result_csv_raw_rows": raw_row_count,
        "result_csv_parse_errors": raw_parse_errors,
        "result_dirs": [str(p.resolve()) for p in result_dirs],
        "zip_layout": {
            "figures": "figures/*.png",
            "csv": "csv/results_all.csv + csv/results_sources.csv",
            "logs": "logs/pipeline.log",
            "index": "INDEX.csv",
            "manifest_csv": "MANIFEST.csv",
            "manifest_txt": "MANIFEST.txt",
            "readme": "README.md",
        },
        "global_core": [p.name for p in global_core],
        "global_appendix": [p.name for p in global_appendix],
        "filip_core": [p.name for p in filip_core],
        "filip_appendix": [p.name for p in filip_appendix],
    }

    index_rows: list[dict[str, object]] = []

    with ZipFile(out_zip, "w", compression=ZIP_DEFLATED) as zf:
        used_arcnames: set[str] = set()
        for plot in global_core:
            arc = _unique_arcname(used_arcnames, f"figures/{_flatten_name(plot.name)}")
            zf.write(plot, arcname=arc)
            index_rows.append(
                {
                    "zip_path": arc,
                    "section": "figures",
                    "kind": "analysis_plot",
                    "source_run": "global",
                    "source_rel_path": plot.name,
                    "source_path": str(plot.resolve()),
                }
            )
        for plot in global_appendix:
            arc = _unique_arcname(used_arcnames, f"figures/{_flatten_name(plot.name)}")
            zf.write(plot, arcname=arc)
            index_rows.append(
                {
                    "zip_path": arc,
                    "section": "figures",
                    "kind": "analysis_plot_appendix",
                    "source_run": "global",
                    "source_rel_path": plot.name,
                    "source_path": str(plot.resolve()),
                }
            )
        for plot in filip_core:
            arc = _unique_arcname(used_arcnames, f"figures/{_flatten_name(plot.name)}")
            zf.write(plot, arcname=arc)
            index_rows.append(
                {
                    "zip_path": arc,
                    "section": "figures",
                    "kind": "filip_plot",
                    "source_run": str(filip_run_dir.name) if isinstance(filip_run_dir, Path) else "",
                    "source_rel_path": plot.name,
                    "source_path": str(plot.resolve()),
                }
            )
        for plot in filip_appendix:
            arc = _unique_arcname(used_arcnames, f"figures/{_flatten_name(plot.name)}")
            zf.write(plot, arcname=arc)
            index_rows.append(
                {
                    "zip_path": arc,
                    "section": "figures",
                    "kind": "filip_plot_appendix",
                    "source_run": str(filip_run_dir.name) if isinstance(filip_run_dir, Path) else "",
                    "source_rel_path": plot.name,
                    "source_path": str(plot.resolve()),
                }
            )

        if isinstance(global_manifest, Path) and global_manifest.exists():
            arc = _unique_arcname(used_arcnames, "figures/thesis_core_manifest.json")
            zf.write(global_manifest, arcname=arc)
            index_rows.append(
                {
                    "zip_path": arc,
                    "section": "figures",
                    "kind": "figure_manifest",
                    "source_run": "global",
                    "source_rel_path": global_manifest.name,
                    "source_path": str(global_manifest.resolve()),
                }
            )
        if isinstance(filip_manifest, Path) and filip_manifest.exists():
            arc = _unique_arcname(used_arcnames, "figures/filip_figures_manifest.json")
            zf.write(filip_manifest, arcname=arc)
            index_rows.append(
                {
                    "zip_path": arc,
                    "section": "figures",
                    "kind": "figure_manifest",
                    "source_run": str(filip_run_dir.name) if isinstance(filip_run_dir, Path) else "",
                    "source_rel_path": filip_manifest.name,
                    "source_path": str(filip_manifest.resolve()),
                }
            )

        zf.writestr("csv/results_all.csv", results_all_csv)
        zf.writestr("csv/results_sources.csv", results_sources_csv)
        index_rows.append(
            {
                "zip_path": "csv/results_all.csv",
                "section": "csv",
                "kind": "result_csv_consolidated",
                "source_run": campaign_name,
                "source_rel_path": "generated from all source csv files",
                "source_path": "",
            }
        )
        index_rows.append(
            {
                "zip_path": "csv/results_sources.csv",
                "section": "csv",
                "kind": "result_csv_sources_map",
                "source_run": campaign_name,
                "source_rel_path": "generated source file map",
                "source_path": "",
            }
        )
        zf.writestr("logs/pipeline.log", combined_log)
        index_rows.append(
            {
                "zip_path": "logs/pipeline.log",
                "section": "logs",
                "kind": "combined_log",
                "source_run": campaign_name,
                "source_rel_path": "logs/*.log (combined)",
                "source_path": str((campaign_dir / "logs").resolve()) if campaign_dir is not None else "",
            }
        )

        manifest_row = {
            "created_at_utc": manifest.get("created_at_utc", ""),
            "campaign_name": campaign_name,
            "campaign_dir": manifest.get("campaign_dir", ""),
            "platform": platform,
            "runs_label": run_label,
            "include_appendix": include_appendix,
            "analysis_plot_count": len(global_core) + len(global_appendix),
            "filip_plot_count": len(filip_core) + len(filip_appendix),
            "result_csv_count": bundled_csv_count,
            "result_csv_raw_count": raw_csv_count,
            "result_csv_raw_rows": raw_row_count,
            "result_csv_parse_errors": raw_parse_errors,
            "result_dirs_count": len(result_dirs),
        }
        readme_payload = dict(manifest_row)

        zf.writestr(
            "MANIFEST.csv",
            _render_csv(
                [manifest_row],
                [
                    "created_at_utc",
                    "campaign_name",
                    "campaign_dir",
                    "platform",
                    "runs_label",
                    "include_appendix",
                    "analysis_plot_count",
                    "filip_plot_count",
                    "result_csv_count",
                    "result_csv_raw_count",
                    "result_csv_raw_rows",
                    "result_csv_parse_errors",
                    "result_dirs_count",
                ],
            ),
        )
        zf.writestr("MANIFEST.txt", _render_manifest_txt({**manifest_row}))
        zf.writestr("README.md", _render_readme_md(readme_payload))

        index_rows.append(
            {
                "zip_path": "MANIFEST.csv",
                "section": "meta",
                "kind": "manifest_csv",
                "source_run": campaign_name,
                "source_rel_path": "generated",
                "source_path": "",
            }
        )
        index_rows.append(
            {
                "zip_path": "MANIFEST.txt",
                "section": "meta",
                "kind": "manifest_txt",
                "source_run": campaign_name,
                "source_rel_path": "generated",
                "source_path": "",
            }
        )
        index_rows.append(
            {
                "zip_path": "README.md",
                "section": "meta",
                "kind": "readme",
                "source_run": campaign_name,
                "source_rel_path": "generated",
                "source_path": "",
            }
        )
        zf.writestr(
            "INDEX.csv",
            _render_csv(
                index_rows,
                ["zip_path", "section", "kind", "source_run", "source_rel_path", "source_path"],
            ),
        )

    analysis_count = len(global_core) + len(global_appendix)
    filip_count = len(filip_core) + len(filip_appendix)
    return {
        "ok": True,
        "zip_path": str(out_zip.resolve()),
        "analysis_plot_count": analysis_count,
        "filip_plot_count": filip_count,
        "global_core_count": len(global_core),
        "global_appendix_count": len(global_appendix),
        "filip_core_count": len(filip_core),
        "filip_appendix_count": len(filip_appendix),
        "result_dirs_count": len(result_dirs),
        "result_csv_count": bundled_csv_count,
        "result_csv_raw_count": raw_csv_count,
        "result_csv_raw_rows": raw_row_count,
        "result_csv_parse_errors": raw_parse_errors,
        "combined_log": "logs/pipeline.log",
        "campaign_dir": str(campaign_dir.resolve()) if campaign_dir else "",
        "filip_run_dir": str(filip_run_dir.resolve()) if isinstance(filip_run_dir, Path) else "",
    }


def main() -> None:
    ap = argparse.ArgumentParser(description="Build a ZIP bundle with thesis-core and appendix figures.")
    ap.add_argument("--campaign-dir", default="", help="Optional thesis campaign directory used to pick Filip figures and store the bundle.")
    ap.add_argument("--out", default="", help="Optional output zip path.")
    ap.add_argument("--no-appendix", action="store_true", help="Only include thesis-core figures.")
    args = ap.parse_args()

    campaign_dir = Path(args.campaign_dir).expanduser().resolve() if args.campaign_dir else _latest_stable_campaign_dir()
    if args.campaign_dir and not campaign_dir.exists():
        raise SystemExit(f"Campaign directory does not exist: {campaign_dir}")
    out_zip = Path(args.out).expanduser().resolve() if args.out else None
    result = build_plot_zip(campaign_dir=campaign_dir, out_zip=out_zip, include_appendix=not args.no_appendix)
    print(json.dumps(result, ensure_ascii=False, indent=2))


if __name__ == "__main__":
    main()
