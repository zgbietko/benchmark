#!/usr/bin/env python3
from __future__ import annotations

import argparse
import csv
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def _read_csv(path: Path) -> list[dict[str, str]]:
    with path.open('r', encoding='utf-8', newline='') as handle:
        return list(csv.DictReader(handle))


def _write_csv(path: Path, rows: list[dict[str, str]]) -> None:
    if not rows:
        return
    path.parent.mkdir(parents=True, exist_ok=True)
    fieldnames: list[str] = []
    seen: set[str] = set()
    for row in rows:
        for key in row.keys():
            if key not in seen:
                seen.add(key)
                fieldnames.append(key)
    with path.open('w', encoding='utf-8', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=fieldnames)
        writer.writeheader()
        writer.writerows(rows)


def _write_markdown_table(path: Path, title: str, rows: list[dict[str, str]], columns: list[str]) -> None:
    if not rows:
        return
    lines = [f'# {title}', '', '| ' + ' | '.join(columns) + ' |', '| ' + ' | '.join(['---'] * len(columns)) + ' |']
    for row in rows:
        lines.append('| ' + ' | '.join(str(row.get(col, '')) for col in columns) + ' |')
    path.write_text('\n'.join(lines) + '\n', encoding='utf-8')


def _write_latex_table(path: Path, caption: str, label: str, rows: list[dict[str, str]], columns: list[str]) -> None:
    if not rows:
        return
    colspec = ' | '.join(['l'] * len(columns))
    lines = [
        '\\begin{table}[ht]',
        '\\centering',
        f'\\caption{{{caption}}}',
        f'\\label{{{label}}}',
        f'\\begin{{tabular}}{{{colspec}}}',
        '\\hline',
        ' & '.join(columns) + ' \\\\',
        '\\hline',
    ]
    for row in rows:
        values = [str(row.get(col, '')).replace('_', '\\_') for col in columns]
        lines.append(' & '.join(values) + ' \\\\')
    lines.extend(['\\hline', '\\end{tabular}', '\\end{table}', ''])
    path.write_text('\n'.join(lines), encoding='utf-8')


def main() -> None:
    ap = argparse.ArgumentParser(description='Generate thesis-ready tables from Final contracts.')
    ap.add_argument('--run-dir', required=True)
    args = ap.parse_args()
    run_dir = Path(args.run_dir).expanduser().resolve()
    out_dir = run_dir / 'tables'
    out_dir.mkdir(parents=True, exist_ok=True)

    env_path = run_dir / 'environment_manifest.json'
    if env_path.exists():
        env = json.loads(env_path.read_text(encoding='utf-8'))
        platform_rows = [{
            'hostname': env.get('hostname', ''),
            'system': env.get('system', ''),
            'architecture': env.get('architecture', ''),
            'cpu_model': env.get('cpu_model', ''),
            'gpu_model': env.get('gpu_model', ''),
            'backend': env.get('backend', ''),
            'ram_bytes': env.get('ram_bytes', ''),
        }]
        _write_csv(out_dir / 'platform_specification.csv', platform_rows)
        cols = list(platform_rows[0].keys())
        _write_markdown_table(out_dir / 'platform_specification.md', 'Specyfikacja platformy', platform_rows, cols)
        _write_latex_table(out_dir / 'platform_specification.tex', 'Specyfikacja platformy testowej', 'tab:platform_spec', platform_rows, cols)

    summary_rows: list[dict[str, str]] = []
    for summary_path in sorted((run_dir / 'contracts').rglob('summary.csv')):
        summary_rows.extend(_read_csv(summary_path))
    if summary_rows:
        _write_csv(out_dir / 'benchmark_summary.csv', summary_rows)
        summary_cols = [
            'platform', 'backend', 'benchmark_name', 'kernel_name', 'problem_size', 'threads',
            'time_mean', 'gflops', 'bandwidth_GBps', 'arithmetic_intensity', 'status'
        ]
        _write_markdown_table(out_dir / 'benchmark_summary.md', 'Podsumowanie benchmarków', summary_rows[:60], summary_cols)
        _write_latex_table(out_dir / 'benchmark_summary.tex', 'Podsumowanie benchmarków', 'tab:benchmark_summary', summary_rows[:25], summary_cols)

    model_validation = run_dir / 'model_validation_summary.csv'
    if model_validation.exists():
        mv_rows = _read_csv(model_validation)
        _write_csv(out_dir / 'model_validation_summary.csv', mv_rows)
        mv_cols = [
            'platform', 'backend', 'kernel', 'problem_size', 'threads', 'workload_class', 'arithmetic_intensity',
            'measured_gflops', 'roofline_predicted_gflops', 'model_error_percent',
            'predicted_bottleneck', 'measured_bottleneck', 'classification_match'
        ]
        _write_markdown_table(out_dir / 'model_validation_summary.md', 'Walidacja modelu roofline', mv_rows[:60], mv_cols)
        _write_latex_table(out_dir / 'model_validation_summary.tex', 'Walidacja modelu roofline', 'tab:model_validation', mv_rows[:25], mv_cols)

    bottleneck_matrix = run_dir / 'bottleneck_classification_matrix.csv'
    if bottleneck_matrix.exists():
        bm_rows = _read_csv(bottleneck_matrix)
        _write_csv(out_dir / 'bottleneck_classification_matrix.csv', bm_rows)
        bm_cols = [
            'platform', 'backend', 'workload_class', 'kernel', 'dominant_predicted_bottleneck',
            'dominant_measured_bottleneck', 'mean_model_error_percent', 'classification_accuracy'
        ]
        _write_markdown_table(out_dir / 'bottleneck_classification_matrix.md', 'Macierz klasyfikacji bottlenecków', bm_rows, bm_cols)
        _write_latex_table(out_dir / 'bottleneck_classification_matrix.tex', 'Macierz klasyfikacji bottlenecków', 'tab:bottleneck_matrix', bm_rows[:25], bm_cols)

    report_path = run_dir / 'model_validation_report.json'
    if report_path.exists():
        report = json.loads(report_path.read_text(encoding='utf-8'))
        lines = ['# Raport walidacji modelu', '']
        lines.append(f"- Liczba wierszy: {report.get('row_count', 0)}")
        overall = report.get('overall_classification_accuracy', {})
        lines.append(f"- Zgodność rodzin bottlenecków: {overall.get('matches', 0)}/{overall.get('considered', 0)}")
        real_acc = report.get('real_kernel_classification_accuracy', {})
        lines.append(f"- Real kernels: {real_acc.get('matches', 0)}/{real_acc.get('considered', 0)}")
        lines.append('')
        lines.append('## Backendy')
        lines.append('')
        for backend, payload in sorted((report.get('by_backend') or {}).items()):
            acc = (payload.get('family_accuracy') or {}).get('accuracy')
            lines.append(f"- {backend}: rows={payload.get('rows', 0)}, accuracy={'' if acc is None else round(float(acc), 4)}")
        (out_dir / 'model_validation_overview.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
