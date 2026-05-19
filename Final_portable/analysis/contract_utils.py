from __future__ import annotations

import csv
import json
import os
from pathlib import Path
import shutil
from typing import Any
import math

ROOT = Path(__file__).resolve().parents[1]
if str(ROOT) not in __import__('sys').path:
    __import__('sys').path.insert(0, str(ROOT))

from analysis.bottleneck_classifier import classify_bottleneck
from analysis.model_validation import build_model_validation_summary
from analysis.provenance import file_manifest, sha256_json, sha256_file
from analysis.measurement_stats import build_stability_report, compute_sample_statistics
from scripts.capture_environment import capture_environment_manifest
from validation.correctness.contract import aggregate_validation_records, default_validation_payload
from validation.reference_outputs.catalog import summarize_reference_outputs
from validation.replay.summary import summarize_replay_validation

METRIC_FIELDS = {
    'elapsed_s', 'latency_ns', 'gbps', 'throughput_gbps', 'gflops', 'throughput_gflops',
    'gflops_peak', 'energy_j', 'energy_joule', 'avg_power_w', 'avg_power_watt', 'power_w', 'ai_flop_per_byte',
}
NON_GROUP_FIELDS = {
    'timestamp', 'run_id', 'run_idx', 'error', 'status', 'energy_source', 'energy_supported',
    'energy_samples', 'energy_nan_samples', 'energy_confidence', 'sample_interval_s',
    'sample_kind', 'is_warmup'
} | METRIC_FIELDS
THREAD_FIELDS = ('threads', 'num_threads', 'thread_count', 'logical_threads')
BLOCK_FIELDS = ('block_size', 'workgroup_size', 'vector_len')


def _to_float(value: object) -> float | None:
    try:
        if value is None:
            return None
        s = str(value).strip()
        if not s:
            return None
        out = float(s)
        if not math.isfinite(out):
            return None
        return out
    except Exception:
        return None


def _to_int(value: object) -> int | None:
    try:
        if value is None:
            return None
        s = str(value).strip()
        if not s:
            return None
        return int(float(s))
    except Exception:
        return None


def _read_csv(path: Path) -> list[dict[str, str]]:
    with path.open('r', encoding='utf-8', newline='') as handle:
        return list(csv.DictReader(handle))


def _bundle_slug(path: Path) -> str:
    return path.stem.replace('__', '_')


def _copy_raw_csv(src: Path, dst: Path) -> None:
    dst.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(src, dst)


def _problem_size(row: dict[str, Any]) -> str:
    if row.get('size_bytes'):
        return str(row.get('size_bytes'))
    if row.get('working_set_bytes'):
        return str(row.get('working_set_bytes'))
    for keys in (('m', 'n', 'k'), ('h', 'w'), ('d', 'h', 'w')):
        vals = [row.get(k) for k in keys]
        if all(v not in (None, '') for v in vals):
            return 'x'.join(str(v) for v in vals)
    for key in ('n_elements', 'n', 'pages_touched', 'size_mb'):
        if row.get(key) not in (None, ''):
            return str(row.get(key))
    return ''


def _thread_value(row: dict[str, Any]) -> int | str:
    for key in THREAD_FIELDS:
        value = _to_int(row.get(key))
        if value is not None:
            return value
    return ''


def _block_value(row: dict[str, Any]) -> int | str:
    for key in BLOCK_FIELDS:
        value = _to_int(row.get(key))
        if value is not None:
            return value
    return ''


def _group_key(row: dict[str, str]) -> tuple[tuple[str, str], ...]:
    items = []
    for key, value in row.items():
        if key in NON_GROUP_FIELDS:
            continue
        items.append((key, str(value)))
    return tuple(sorted(items))


def _group_to_meta(key: tuple[tuple[str, str], ...]) -> dict[str, str]:
    return {k: v for k, v in key}


def _metric_series(rows: list[dict[str, str]], field_names: tuple[str, ...]) -> list[float]:
    out: list[float] = []
    for row in rows:
        for name in field_names:
            value = _to_float(row.get(name))
            if value is not None:
                out.append(value)
                break
    return out


def _recorded_warmup_rows(rows: list[dict[str, str]]) -> int:
    count = 0
    for row in rows:
        sample_kind = str(row.get('sample_kind', '')).strip().lower()
        is_warmup = str(row.get('is_warmup', '')).strip().lower()
        if sample_kind == 'warmup' or is_warmup in {'1', 'true', 'yes', 'y'}:
            count += 1
    return count


def _figure_manifest(figure_paths: list[str]) -> dict[str, Any]:
    entries = []
    for path in figure_paths:
        p = Path(path)
        if p.exists() and p.is_file():
            entries.append(file_manifest(p))
    return {
        'count': len(entries),
        'figures': entries,
    }


def _default_model_metrics(summary_row: dict[str, Any]) -> dict[str, Any]:
    pred = _to_float(summary_row.get('gflops'))
    classification = classify_bottleneck(
        benchmark_name=str(summary_row.get('benchmark_name', '')),
        kernel_name=str(summary_row.get('kernel_name', '')),
        arithmetic_intensity=_to_float(summary_row.get('arithmetic_intensity')),
        measured_gflops=_to_float(summary_row.get('gflops')),
        measured_bandwidth=_to_float(summary_row.get('bandwidth_GBps')),
        predicted_gflops=pred,
        latency_ns=_to_float(summary_row.get('latency_mean_ns')),
    )
    return {
        'predicted_bottleneck': classification.get('bottleneck', 'mixed'),
        'measured_bottleneck': classification.get('bottleneck', 'mixed'),
        'measured_bottleneck_family': classification.get('family', ''),
        'classifier_confidence': classification.get('confidence', 0.0),
        'classifier_reason': classification.get('reason', ''),
    }


def _summarize_bundle_rows(
    *,
    rows: list[dict[str, str]],
    csv_path: Path,
    workflow: str,
    warmups: int,
    environment_manifest: dict[str, Any],
) -> tuple[list[dict[str, Any]], dict[str, Any], dict[str, Any]]:
    grouped: dict[tuple[tuple[str, str], ...], list[dict[str, str]]] = {}
    for row in rows:
        grouped.setdefault(_group_key(row), []).append(row)
    summary_rows: list[dict[str, Any]] = []
    for group_key, group_rows in grouped.items():
        meta = _group_to_meta(group_key)
        effective_warmups = _recorded_warmup_rows(group_rows)
        time_stats = compute_sample_statistics(_metric_series(group_rows, ('elapsed_s',)), warmup_repetitions=effective_warmups)
        bw_stats = compute_sample_statistics(_metric_series(group_rows, ('gbps', 'throughput_gbps')), warmup_repetitions=effective_warmups) if _metric_series(group_rows, ('gbps', 'throughput_gbps')) else None
        gf_stats = compute_sample_statistics(_metric_series(group_rows, ('gflops', 'throughput_gflops', 'gflops_peak')), warmup_repetitions=effective_warmups) if _metric_series(group_rows, ('gflops', 'throughput_gflops', 'gflops_peak')) else None
        energy_stats = compute_sample_statistics(_metric_series(group_rows, ('energy_j', 'energy_joule')), warmup_repetitions=effective_warmups) if _metric_series(group_rows, ('energy_j', 'energy_joule')) else None
        power_stats = compute_sample_statistics(_metric_series(group_rows, ('avg_power_w', 'power_w', 'avg_power_watt')), warmup_repetitions=effective_warmups) if _metric_series(group_rows, ('avg_power_w', 'power_w', 'avg_power_watt')) else None
        latency_stats = compute_sample_statistics(_metric_series(group_rows, ('latency_ns',)), warmup_repetitions=effective_warmups) if _metric_series(group_rows, ('latency_ns',)) else None
        exemplar = group_rows[0]
        stability = build_stability_report(
            coefficient_of_variation=time_stats.coefficient_of_variation,
            outliers_removed=time_stats.outliers_removed,
            valid_repetitions=time_stats.valid_repetitions,
        )
        status_values = {str(r.get('status', 'ok')).strip().lower() or 'ok' for r in group_rows}
        status = 'ok' if status_values == {'ok'} else ('warning' if 'ok' in status_values else 'fail')
        summary_row = {
            'platform': str(exemplar.get('cpu_model') or exemplar.get('gpu_model') or exemplar.get('device_name') or environment_manifest.get('cpu_model', 'unknown')),
            'backend': str(exemplar.get('backend', 'cpu')),
            'benchmark_name': str(exemplar.get('benchmark') or csv_path.stem.split('__backend')[0]),
            'kernel_name': str(exemplar.get('kernel') or exemplar.get('benchmark') or csv_path.stem.split('__backend')[0]),
            'problem_size': _problem_size(exemplar),
            'threads': _thread_value(exemplar),
            'block_size': _block_value(exemplar),
            'time_mean': time_stats.mean,
            'time_median': time_stats.median,
            'time_std': time_stats.std,
            'time_min': time_stats.minimum,
            'time_max': time_stats.maximum,
            'repetitions': time_stats.repetitions,
            'warmup_repetitions': time_stats.warmup_repetitions,
            'bandwidth_GBps': '' if bw_stats is None else bw_stats.mean,
            'gflops': '' if gf_stats is None else gf_stats.mean,
            'energy_J': '' if energy_stats is None else energy_stats.mean,
            'power_W': '' if power_stats is None else power_stats.mean,
            'arithmetic_intensity': str(exemplar.get('ai_flop_per_byte', '')),
            'status': status,
            'time_ci95_low': time_stats.ci95_low,
            'time_ci95_high': time_stats.ci95_high,
            'coefficient_of_variation': time_stats.coefficient_of_variation,
            'outliers_removed': time_stats.outliers_removed,
            'latency_mean_ns': '' if latency_stats is None else latency_stats.mean,
        }
        summary_row.update(meta)
        summary_row.update({f'stability_{k}': v for k, v in stability.items()})
        summary_rows.append(summary_row)
    metadata = {
        'workflow': workflow,
        'source_csv': str(csv_path),
        'rows': len(rows),
        'groups': len(summary_rows),
        'columns': list(rows[0].keys()) if rows else [],
        'warmup_repetitions': int(warmups),
    }
    model_metrics = {
        'rows': [_default_model_metrics(row) | {'benchmark_name': row.get('benchmark_name'), 'kernel_name': row.get('kernel_name')} for row in summary_rows],
    }
    return summary_rows, metadata, model_metrics


def _write_csv(path: Path, rows: list[dict[str, Any]]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    if not rows:
        path.write_text('', encoding='utf-8')
        return
    with path.open('w', encoding='utf-8', newline='') as handle:
        writer = csv.DictWriter(handle, fieldnames=list(rows[0].keys()))
        writer.writeheader()
        writer.writerows(rows)


def _write_json(path: Path, payload: dict[str, Any]) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(payload, indent=2, ensure_ascii=False), encoding='utf-8')


def _read_manifest_payload(path: Path) -> dict[str, Any]:
    if not path.exists():
        return {}
    try:
        payload = json.loads(path.read_text(encoding='utf-8'))
        return payload if isinstance(payload, dict) else {}
    except Exception:
        return {}


def _optimization_validation_payload(out_dir: Path, validations: list[Path]) -> dict[str, Any]:
    if validations:
        return aggregate_validation_records(validations, scope='optimization')
    summary = _read_manifest_payload(out_dir / 'summary.json')
    if str(summary.get('filip_mode', '')).strip().lower() != 'exact_reference':
        return default_validation_payload(scope='optimization')
    execution_mode = str(summary.get('execution_mode', '')).strip().lower()
    experiment_class = str(summary.get('experiment_class', '')).strip()
    if execution_mode == 'native':
        return {
            'scope': 'optimization',
            'status': 'warning',
            'pass_count': 0,
            'warning_count': 1,
            'fail_count': 0,
            'blocked': False,
            'reason': 'Uruchomiono metal exact-style port bez replay dump root; to nie jest correctness replay 1:1.',
            'records': [
                {
                    'status': 'warning',
                    'execution_mode': execution_mode,
                    'experiment_class': experiment_class,
                    'numerical_equivalence': str(summary.get('numerical_equivalence', '')),
                    'comparison_note': str(summary.get('comparison_note', '')),
                }
            ],
        }
    if execution_mode == 'exact_reference':
        return default_validation_payload(
            scope='optimization',
            reason='To jest kampania referencyjna OpenCL/source-of-truth. Correctness replay wykonuje sie osobno na bundle replay.',
        )
    return default_validation_payload(scope='optimization')


def _write_bundle(
    *,
    bundle_dir: Path,
    csv_path: Path,
    workflow: str,
    warmups: int,
    environment_manifest: dict[str, Any],
    validation_payload: dict[str, Any] | None = None,
    figure_paths: list[str] | None = None,
) -> dict[str, Any]:
    rows = _read_csv(csv_path)
    if not rows:
        return {}
    raw_path = bundle_dir / 'raw_results.csv'
    _copy_raw_csv(csv_path, raw_path)
    summary_rows, metadata, model_metrics = _summarize_bundle_rows(
        rows=rows,
        csv_path=csv_path,
        workflow=workflow,
        warmups=warmups,
        environment_manifest=environment_manifest,
    )
    _write_csv(bundle_dir / 'summary.csv', summary_rows)
    _write_json(bundle_dir / 'metadata.json', metadata)
    _write_json(bundle_dir / 'environment.json', environment_manifest)
    _write_json(bundle_dir / 'validation.json', validation_payload or default_validation_payload(scope=csv_path.stem))
    _write_json(bundle_dir / 'model_metrics.json', model_metrics)
    _write_json(bundle_dir / 'figures_manifest.json', _figure_manifest(figure_paths or []))
    return {
        'bundle_dir': str(bundle_dir),
        'raw_results.csv': str(raw_path),
        'summary.csv': str(bundle_dir / 'summary.csv'),
        'metadata.json': str(bundle_dir / 'metadata.json'),
        'environment.json': str(bundle_dir / 'environment.json'),
        'validation.json': str(bundle_dir / 'validation.json'),
        'model_metrics.json': str(bundle_dir / 'model_metrics.json'),
        'figures_manifest.json': str(bundle_dir / 'figures_manifest.json'),
    }


def _session_figure_paths(session_dir: Path) -> list[str]:
    out: list[str] = []
    for base in (session_dir / 'figures', ROOT / 'analysis' / 'figures' / 'thesis_core'):
        if base.exists():
            out.extend(str(p.resolve()) for p in base.glob('*.png'))
    return sorted(set(out))


def standardize_session_artifacts(*, session_dir: Path, workflow: str, resolved_backend: str = '', warmups: int = 0, command_args: list[str] | None = None) -> dict[str, Any]:
    session_dir = Path(session_dir).expanduser().resolve()
    env_manifest = capture_environment_manifest(backend=resolved_backend, command_args=command_args, root=ROOT)
    source_manifest = _read_manifest_payload(session_dir / 'manifest.json')
    contracts_root = session_dir / 'contracts'
    bundles: list[dict[str, Any]] = []
    figures = _session_figure_paths(session_dir)
    for section in ('cpu', 'gpu', 'real_kernels', 'ai_accel'):
        section_dir = session_dir / section
        if not section_dir.exists():
            continue
        for csv_path in sorted(section_dir.glob('*.csv')):
            bundle_dir = contracts_root / section / _bundle_slug(csv_path)
            bundle = _write_bundle(
                bundle_dir=bundle_dir,
                csv_path=csv_path,
                workflow=workflow,
                warmups=warmups,
                environment_manifest=env_manifest,
                figure_paths=figures,
            )
            if bundle:
                bundles.append(bundle)
    model_validation = build_model_validation_summary(session_dir=session_dir)
    if isinstance(model_validation, dict):
        model_validation_summary = model_validation.get('summary_csv', '')
        model_validation_report = model_validation.get('report_json', '')
        bottleneck_matrix = model_validation.get('bottleneck_matrix_csv', '')
    else:
        model_validation_summary = str(model_validation) if model_validation else ''
        model_validation_report = ''
        bottleneck_matrix = ''
    validation_payload = default_validation_payload(scope='session', reason='Walidacja numeryczna dotyczy tylko workflowów exact/replay.')
    _write_json(session_dir / 'environment_manifest.json', env_manifest)
    _write_json(session_dir / 'figures_manifest.json', _figure_manifest(figures))
    run_manifest = {
        'run_id': session_dir.name,
        'date': env_manifest.get('timestamp', ''),
        'profile': source_manifest.get('profile', ''),
        'source_manifest_hash': sha256_json(source_manifest) if source_manifest else '',
        'backend': resolved_backend,
        'host': env_manifest.get('hostname', ''),
        'git_commit': (env_manifest.get('git') or {}).get('commit', ''),
        'config_file': '',
        'config_hash': sha256_json({'command_args': command_args or [], 'workflow': workflow, 'warmups': warmups}),
        'selected_tests': [Path(b['bundle_dir']).name for b in bundles],
        'completed_tests': [Path(b['bundle_dir']).name for b in bundles],
        'failed_tests': [],
        'skipped_tests': [],
        'artifact_paths': bundles,
        'derived_artifacts': {
            'model_validation_summary': model_validation_summary,
            'model_validation_report': model_validation_report,
            'bottleneck_classification_matrix': bottleneck_matrix,
            'energy_metrics': str(session_dir / 'energy_metrics.csv'),
            'environment_manifest': str(session_dir / 'environment_manifest.json'),
            'figures_manifest': str(session_dir / 'figures_manifest.json'),
        },
        'health_status': 'ok',
    }
    _write_json(session_dir / 'run_manifest.json', run_manifest)
    energy_rows: list[dict[str, Any]] = []
    for bundle in bundles:
        summary_path = Path(bundle['summary.csv'])
        for row in _read_csv(summary_path):
            energy_rows.append(
                {
                    'kernel': row.get('kernel_name', ''),
                    'platform': row.get('platform', ''),
                    'backend': row.get('backend', ''),
                    'time_s': row.get('time_mean', ''),
                    'energy_J': row.get('energy_J', ''),
                    'avg_power_W': row.get('power_W', ''),
                    'max_power_W': '',
                    'energy_per_byte': '',
                    'energy_per_operation': '',
                    'gflops_per_watt': '',
                    'GBps_per_watt': '',
                    'measurement_source': '',
                    'measurement_confidence': '',
                }
            )
    if energy_rows:
        _write_csv(session_dir / 'energy_metrics.csv', energy_rows)
    return {
        'contracts_root': str(contracts_root),
        'environment_manifest': str(session_dir / 'environment_manifest.json'),
        'figures_manifest': str(session_dir / 'figures_manifest.json'),
        'run_manifest': str(session_dir / 'run_manifest.json'),
        'model_validation_summary': model_validation_summary,
        'model_validation_report': model_validation_report,
        'bottleneck_classification_matrix': bottleneck_matrix,
        'bundle_count': len(bundles),
    }


def standardize_validation_artifacts(*, out_dir: Path, workflow: str, resolved_backend: str = '', warmups: int = 0, command_args: list[str] | None = None) -> dict[str, Any]:
    out_dir = Path(out_dir).expanduser().resolve()
    env_manifest = capture_environment_manifest(backend=resolved_backend, command_args=command_args, root=ROOT)
    source_manifest = _read_manifest_payload(out_dir / 'manifest.json')
    contracts_root = out_dir / 'contracts'
    figures = [str(p.resolve()) for p in out_dir.glob('*.png')]
    bundles: list[dict[str, Any]] = []
    for csv_path in sorted(out_dir.glob('*.csv')):
        if csv_path.name in {'probe_summary.csv', 'category_summary.csv'}:
            bundle_dir = contracts_root / _bundle_slug(csv_path)
            bundle = _write_bundle(bundle_dir=bundle_dir, csv_path=csv_path, workflow=workflow, warmups=warmups, environment_manifest=env_manifest, figure_paths=figures)
            if bundle:
                bundles.append(bundle)
    _write_json(out_dir / 'environment_manifest.json', env_manifest)
    _write_json(out_dir / 'figures_manifest.json', _figure_manifest(figures))
    _write_json(out_dir / 'run_manifest.json', {
        'run_id': out_dir.name,
        'date': env_manifest.get('timestamp', ''),
        'profile': source_manifest.get('profile', ''),
        'backend': resolved_backend,
        'host': env_manifest.get('hostname', ''),
        'git_commit': (env_manifest.get('git') or {}).get('commit', ''),
        'config_file': '',
        'source_manifest_hash': sha256_json(source_manifest) if source_manifest else '',
        'config_hash': sha256_json({'command_args': command_args or [], 'workflow': workflow, 'warmups': warmups}),
        'selected_tests': [Path(b['bundle_dir']).name for b in bundles],
        'completed_tests': [Path(b['bundle_dir']).name for b in bundles],
        'failed_tests': [],
        'skipped_tests': [],
        'artifact_paths': bundles,
        'health_status': 'ok',
    })
    return {'contracts_root': str(contracts_root), 'bundle_count': len(bundles)}


def standardize_optimization_artifacts(*, out_dir: Path, workflow: str, resolved_backend: str = '', warmups: int = 0, command_args: list[str] | None = None) -> dict[str, Any]:
    out_dir = Path(out_dir).expanduser().resolve()
    env_manifest = capture_environment_manifest(backend=resolved_backend, command_args=command_args, root=ROOT)
    source_manifest = _read_manifest_payload(out_dir / 'manifest.json')
    contracts_root = out_dir / 'contracts'
    figures = []
    for rel in ('figures/thesis_core', 'figures/appendix', 'plots'):
        base = out_dir / rel
        if base.exists():
            figures.extend(str(p.resolve()) for p in base.glob('*.png'))
    bundles: list[dict[str, Any]] = []
    for csv_path in sorted((out_dir / 'csv').glob('*.csv')) if (out_dir / 'csv').exists() else []:
        bundle = _write_bundle(
            bundle_dir=contracts_root / 'csv' / _bundle_slug(csv_path),
            csv_path=csv_path,
            workflow=workflow,
            warmups=warmups,
            environment_manifest=env_manifest,
            validation_payload=default_validation_payload(scope='optimization'),
            figure_paths=figures,
        )
        if bundle:
            bundles.append(bundle)
    validations = sorted(
        path
        for path in out_dir.rglob('validation.json')
        if path.parent != out_dir and 'contracts' not in path.parts
    )
    validation_payload = _optimization_validation_payload(out_dir, validations)
    _write_json(out_dir / 'validation.json', validation_payload)
    _write_json(out_dir / 'environment_manifest.json', env_manifest)
    _write_json(out_dir / 'figures_manifest.json', _figure_manifest(figures))
    replay_summary = summarize_replay_validation(out_dir) if validations else default_validation_payload(scope='replay')
    reference_summary = summarize_reference_outputs(out_dir)
    _write_json(out_dir / 'replay_validation.json', replay_summary)
    _write_json(out_dir / 'reference_outputs.json', reference_summary)
    _write_json(out_dir / 'run_manifest.json', {
        'run_id': out_dir.name,
        'date': env_manifest.get('timestamp', ''),
        'profile': source_manifest.get('profile', ''),
        'backend': resolved_backend,
        'host': env_manifest.get('hostname', ''),
        'git_commit': (env_manifest.get('git') or {}).get('commit', ''),
        'config_file': '',
        'source_manifest_hash': sha256_json(source_manifest) if source_manifest else '',
        'config_hash': sha256_json({'command_args': command_args or [], 'workflow': workflow, 'warmups': warmups}),
        'selected_tests': [Path(b['bundle_dir']).name for b in bundles],
        'completed_tests': [Path(b['bundle_dir']).name for b in bundles],
        'failed_tests': ['validation'] if validation_payload.get('blocked') else [],
        'skipped_tests': [],
        'artifact_paths': bundles,
        'health_status': 'fail' if validation_payload.get('blocked') else 'ok',
    })
    return {
        'contracts_root': str(contracts_root),
        'environment_manifest': str(out_dir / 'environment_manifest.json'),
        'validation': str(out_dir / 'validation.json'),
        'run_manifest': str(out_dir / 'run_manifest.json'),
        'bundle_count': len(bundles),
    }


def standardize_campaign_artifacts(*, campaign_dir: Path, summary: dict[str, Any]) -> dict[str, Any]:
    campaign_dir = Path(campaign_dir).expanduser().resolve()
    env_manifest = capture_environment_manifest(backend=str(summary.get('resolved_gpu_backend') or summary.get('resolved_fem_backend') or ''), command_args=[], root=ROOT)
    source_manifest = _read_manifest_payload(campaign_dir / 'manifest.json')
    _write_json(campaign_dir / 'environment_manifest.json', env_manifest)
    plots = []
    for p in (campaign_dir / 'plots').glob('*.png'):
        plots.append(str(p.resolve()))
    _write_json(campaign_dir / 'figures_manifest.json', _figure_manifest(plots))
    run_manifest = {
        'run_id': campaign_dir.name,
        'date': summary.get('timestamp_utc', env_manifest.get('timestamp', '')),
        'profile': summary.get('profile', '') or source_manifest.get('profile', ''),
        'backend': summary.get('resolved_fem_backend') or summary.get('resolved_gpu_backend') or summary.get('requested_backend') or '',
        'host': env_manifest.get('hostname', ''),
        'git_commit': (env_manifest.get('git') or {}).get('commit', ''),
        'config_file': '',
        'source_manifest_hash': sha256_json(source_manifest) if source_manifest else '',
        'config_hash': sha256_json({
            'requested_backend': summary.get('requested_backend', ''),
            'benchmark_mode': summary.get('benchmark_mode', ''),
            'warmups': summary.get('warmups', 0),
            'experiment_profile': summary.get('experiment_profile', ''),
        }),
        'selected_tests': [str(step.get('id', '')) for step in summary.get('steps', [])],
        'completed_tests': [str(step.get('id', '')) for step in summary.get('steps', []) if str(step.get('status', '')) == 'ok'],
        'failed_tests': [str(step.get('id', '')) for step in summary.get('steps', []) if str(step.get('status', '')) == 'failed'],
        'skipped_tests': [str(step.get('id', '')) for step in summary.get('steps', []) if str(step.get('status', '')) == 'skipped'],
        'artifact_paths': summary.get('key_results', {}),
        'health_status': 'ok' if not summary.get('required_failures') else 'fail',
    }
    _write_json(campaign_dir / 'run_manifest.json', run_manifest)
    return {
        'environment_manifest': str(campaign_dir / 'environment_manifest.json'),
        'figures_manifest': str(campaign_dir / 'figures_manifest.json'),
        'run_manifest': str(campaign_dir / 'run_manifest.json'),
    }
