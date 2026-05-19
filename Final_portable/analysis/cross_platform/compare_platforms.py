#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
from pathlib import Path


def _read_csv(path: Path) -> list[dict[str, str]]:
    with path.open('r', encoding='utf-8', newline='') as handle:
        return list(csv.DictReader(handle))


def _to_float(value: object) -> float | None:
    try:
        if value is None:
            return None
        text = str(value).strip()
        if not text or text.lower() == 'nan':
            return None
        return float(text)
    except Exception:
        return None


def _normalized_row(row: dict[str, str]) -> dict[str, str | float]:
    measured = _to_float(row.get('measured_gflops') or row.get('gflops'))
    peak = _to_float(row.get('peak_gflops'))
    roof = _to_float(row.get('roofline_predicted_gflops'))
    bw = _to_float(row.get('measured_bandwidth') or row.get('bandwidth_GBps'))
    peak_bw = _to_float(row.get('peak_bw_gbps'))
    out = {
        'platform': row.get('platform', ''),
        'backend': row.get('backend', ''),
        'kernel': row.get('kernel') or row.get('kernel_name', ''),
        'benchmark_name': row.get('benchmark_name', ''),
        'workload_class': row.get('workload_class', ''),
        'problem_size': row.get('problem_size', ''),
        'threads': row.get('threads', ''),
        'measured_gflops': '' if measured is None else measured,
        'measured_bandwidth': '' if bw is None else bw,
        'roofline_predicted_gflops': '' if roof is None else roof,
        'measured_percent_of_peak_compute': '' if peak in (None, 0.0) or measured is None else (measured / peak * 100.0),
        'measured_percent_of_peak_bw': '' if peak_bw in (None, 0.0) or bw is None else (bw / peak_bw * 100.0),
        'relative_to_measured_roofline': '' if roof in (None, 0.0) or measured is None else (measured / roof * 100.0),
        'model_error_percent': row.get('model_error_percent', ''),
        'predicted_bottleneck': row.get('predicted_bottleneck', ''),
        'measured_bottleneck': row.get('measured_bottleneck', ''),
        'classification_match': row.get('classification_match', ''),
        'classification_match_exact': row.get('classification_match_exact', ''),
        'energy_J': row.get('energy_J', ''),
        'power_W': row.get('power_W', ''),
    }
    return out


def main() -> None:
    ap = argparse.ArgumentParser(description='Cross-platform comparison from Final summary/model-validation CSV files.')
    ap.add_argument('--summary', action='append', required=True, help='Path to summary.csv or model_validation_summary.csv (repeatable)')
    ap.add_argument('--out', required=True)
    args = ap.parse_args()

    rows = []
    for raw in args.summary:
        rows.extend(_read_csv(Path(raw).expanduser().resolve()))
    normalized = [_normalized_row(row) for row in rows]

    out = Path(args.out).expanduser().resolve()
    out.parent.mkdir(parents=True, exist_ok=True)
    if not normalized:
        out.write_text('', encoding='utf-8')
        return
    with out.open('w', encoding='utf-8', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=list(normalized[0].keys()))
        writer.writeheader()
        writer.writerows(normalized)
    print(str(out))


if __name__ == '__main__':
    main()
