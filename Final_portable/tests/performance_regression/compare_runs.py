#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path


def _load_summary(path: Path) -> dict[tuple[str, str], dict[str, str]]:
    with path.open('r', encoding='utf-8', newline='') as handle:
        rows = list(csv.DictReader(handle))
    return {(row.get('benchmark_name', ''), row.get('kernel_name', '')): row for row in rows}


def main() -> None:
    ap = argparse.ArgumentParser(description='Compare two Final summary.csv files for regressions.')
    ap.add_argument('--baseline', required=True)
    ap.add_argument('--candidate', required=True)
    ap.add_argument('--threshold-percent', type=float, default=5.0)
    args = ap.parse_args()
    baseline = _load_summary(Path(args.baseline).expanduser().resolve())
    candidate = _load_summary(Path(args.candidate).expanduser().resolve())
    for key, base_row in baseline.items():
        cand_row = candidate.get(key)
        if not cand_row:
            print(f'{key[0]}/{key[1]}: MISSING')
            continue
        try:
            base_time = float(base_row.get('time_mean', 'nan'))
            cand_time = float(cand_row.get('time_mean', 'nan'))
        except Exception:
            print(f'{key[0]}/{key[1]}: INVALID')
            continue
        delta = (cand_time - base_time) / max(base_time, 1e-12) * 100.0
        if delta > args.threshold_percent:
            print(f'{key[0]}/{key[1]}: REGRESSION {delta:+.1f}%')
        elif delta > 0.0:
            print(f'{key[0]}/{key[1]}: WARNING {delta:+.1f}%')
        else:
            print(f'{key[0]}/{key[1]}: OK {delta:+.1f}%')


if __name__ == '__main__':
    main()
