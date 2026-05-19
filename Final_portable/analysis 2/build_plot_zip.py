#!/usr/bin/env python3
from __future__ import annotations

import argparse
import json
import sys
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
    return {
        "global_core": sorted(p for p in THESIS_CORE_DIR.glob("*.png") if p.is_file()),
        "global_appendix": sorted(p for p in APPENDIX_DIR.glob("*.png") if p.is_file()) if include_appendix else [],
        "global_manifest": (MANIFEST_DIR / "thesis_core_manifest.json") if (MANIFEST_DIR / "thesis_core_manifest.json").exists() else None,
        "filip_core": sorted(p for p in (filip_core_dir.glob("*.png") if filip_core_dir else []) if p.is_file()),
        "filip_appendix": sorted(p for p in (filip_appendix_dir.glob("*.png") if include_appendix and filip_appendix_dir else []) if p.is_file()),
        "filip_manifest": filip_manifest,
        "filip_run_dir": filip_run_dir,
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

    if not global_core and not filip_core and not global_appendix and not filip_appendix:
        raise SystemExit("Brak figur do spakowania.")

    output_dir = _artifact_dir_for_campaign(campaign_dir)
    timestamp = datetime.now().strftime("%Y%m%d_%H%M%S")
    if out_zip is None:
        stem = campaign_dir.name if campaign_dir is not None else "latest"
        out_zip = output_dir / f"plots_bundle__{stem}__{timestamp}.zip"
    out_zip.parent.mkdir(parents=True, exist_ok=True)

    manifest = {
        "created_at_utc": datetime.now(timezone.utc).isoformat(),
        "campaign_dir": str(campaign_dir.resolve()) if campaign_dir else "",
        "global_thesis_core_dir": str(THESIS_CORE_DIR.resolve()),
        "global_appendix_dir": str(APPENDIX_DIR.resolve()),
        "filip_run_dir": str(filip_run_dir.resolve()) if isinstance(filip_run_dir, Path) else "",
        "include_appendix": include_appendix,
        "global_core_count": len(global_core),
        "global_appendix_count": len(global_appendix),
        "filip_core_count": len(filip_core),
        "filip_appendix_count": len(filip_appendix),
        "global_core": [p.name for p in global_core],
        "global_appendix": [p.name for p in global_appendix],
        "filip_core": [p.name for p in filip_core],
        "filip_appendix": [p.name for p in filip_appendix],
    }

    with ZipFile(out_zip, "w", compression=ZIP_DEFLATED) as zf:
        for plot in global_core:
            zf.write(plot, arcname=f"thesis_core/global/{plot.name}")
        for plot in global_appendix:
            zf.write(plot, arcname=f"appendix/global/{plot.name}")
        for plot in filip_core:
            zf.write(plot, arcname=f"thesis_core/filip/{plot.name}")
        for plot in filip_appendix:
            zf.write(plot, arcname=f"appendix/filip/{plot.name}")
        if isinstance(global_manifest, Path) and global_manifest.exists():
            zf.write(global_manifest, arcname="manifests/thesis_core_manifest.json")
        if isinstance(filip_manifest, Path) and filip_manifest.exists():
            zf.write(filip_manifest, arcname="manifests/filip_figures_manifest.json")
        if campaign_dir is not None:
            for extra_name in ("summary.json", "campaign.md", "steps.json"):
                extra_path = campaign_dir / extra_name
                if extra_path.exists():
                    zf.write(extra_path, arcname=f"campaign/{extra_name}")
        zf.writestr("manifest.json", json.dumps(manifest, ensure_ascii=False, indent=2))

    return {
        "ok": True,
        "zip_path": str(out_zip.resolve()),
        "global_core_count": len(global_core),
        "global_appendix_count": len(global_appendix),
        "filip_core_count": len(filip_core),
        "filip_appendix_count": len(filip_appendix),
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
