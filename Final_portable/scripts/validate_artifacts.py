#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path
import sys
from typing import Any

ROOT = Path(__file__).resolve().parents[1]

REQUIRED_BUNDLE_FILES = [
    'raw_results.csv',
    'summary.csv',
    'metadata.json',
    'environment.json',
    'validation.json',
    'model_metrics.json',
    'figures_manifest.json',
]
REQUIRED_SUMMARY_COLUMNS = {
    'platform', 'backend', 'benchmark_name', 'kernel_name', 'problem_size', 'threads', 'block_size',
    'time_mean', 'time_median', 'time_std', 'time_min', 'time_max', 'repetitions', 'warmup_repetitions',
    'bandwidth_GBps', 'gflops', 'energy_J', 'power_W', 'arithmetic_intensity', 'status'
}
REQUIRED_TOP_LEVEL = ['run_manifest.json', 'environment_manifest.json', 'figures_manifest.json']


def _read_csv_header(path: Path) -> list[str]:
    with path.open('r', encoding='utf-8', newline='') as handle:
        reader = csv.reader(handle)
        return next(reader, [])


def validate_run(path: Path) -> tuple[bool, list[str]]:
    issues: list[str] = []
    for name in REQUIRED_TOP_LEVEL:
        if not (path / name).exists():
            issues.append(f'Brak pliku top-level: {name}')
    run_manifest: dict[str, Any] = {}
    run_manifest_path = path / 'run_manifest.json'
    if run_manifest_path.exists():
        try:
            run_manifest = json.loads(run_manifest_path.read_text(encoding='utf-8'))
        except Exception as exc:
            issues.append(f'Nie można odczytać run_manifest.json: {exc}')
    contracts_root = path / 'contracts'
    if not contracts_root.exists():
        issues.append('Brak katalogu contracts/')
        return False, issues
    bundle_dirs = [p for p in contracts_root.rglob('*') if p.is_dir() and (p / 'summary.csv').exists()]
    if not bundle_dirs:
        issues.append('Brak bundle contracts z summary.csv')
        return False, issues
    for bundle in bundle_dirs:
        for name in REQUIRED_BUNDLE_FILES:
            if not (bundle / name).exists():
                issues.append(f'{bundle}: brak {name}')
        summary_path = bundle / 'summary.csv'
        if summary_path.exists():
            header = set(_read_csv_header(summary_path))
            missing = sorted(REQUIRED_SUMMARY_COLUMNS - header)
            if missing:
                issues.append(f'{bundle}: brak kolumn summary.csv -> {", ".join(missing)}')
    derived_artifacts = run_manifest.get('derived_artifacts', {}) if isinstance(run_manifest, dict) else {}
    if isinstance(derived_artifacts, dict):
        for key, raw_path in derived_artifacts.items():
            if not raw_path:
                continue
            artifact_path = Path(str(raw_path)).expanduser()
            if not artifact_path.is_absolute():
                artifact_path = (path / artifact_path).resolve()
            if not artifact_path.exists():
                issues.append(f'Brak derived artifact wskazanego w run_manifest: {key} -> {artifact_path}')
    return len(issues) == 0, issues


def main() -> None:
    ap = argparse.ArgumentParser(description='Validate Final artifact contracts.')
    ap.add_argument('--path', required=True, help='Run/session/optimization/campaign directory')
    args = ap.parse_args()
    path = Path(args.path).expanduser().resolve()
    ok, issues = validate_run(path)
    print(json.dumps({'path': str(path), 'ok': ok, 'issues': issues}, indent=2, ensure_ascii=False))
    raise SystemExit(0 if ok else 1)


if __name__ == '__main__':
    main()
