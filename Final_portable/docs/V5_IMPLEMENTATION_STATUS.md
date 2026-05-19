# V5 Implementation Status

## Cel
V5 rozszerza V4 o warstwę walidacji, kontrakty artefaktów, statystykę pomiarową, model validation dla roofline, klasyfikację bottlenecków, profile eksperymentów, real-kernel extended mode oraz audyt środowiska eksperymentalnego.

## Najważniejsze nowe moduły
- `validation/`
  - `correctness/contract.py`
  - `numerical_error/metrics.py`
  - `replay/summary.py`
  - `reference_outputs/catalog.py`
- `analysis/statistics/`
  - `repetitions.py`
  - `confidence_intervals.py`
  - `outlier_detection.py`
  - `stability_report.py`
- `analysis/model_validation.py`
- `analysis/bottleneck_classifier.py`
- `analysis/contract_utils.py`
- `monitoring/`
  - `thermal_monitor.py`
  - `power_monitor.py`
  - `throttling_detector.py`
- `scripts/capture_environment.py`
- `scripts/validate_artifacts.py`
- `profiles/*.yaml`
- `analysis/cross_platform/compare_platforms.py`
- `analysis/tables/generate_thesis_tables.py`
- `tests/performance_regression/compare_runs.py`
- `.github/workflows/*.yml`

## Kontrakt wyników
Session/validation/optimization runs są standaryzowane do zestawu:
- `contracts/**/raw_results.csv`
- `contracts/**/summary.csv`
- `contracts/**/metadata.json`
- `contracts/**/environment.json`
- `contracts/**/validation.json`
- `contracts/**/model_metrics.json`
- `contracts/**/figures_manifest.json`

Top-level run artifacts:
- `environment_manifest.json`
- `figures_manifest.json`
- `run_manifest.json`
- `model_validation_summary.csv` (jeśli dostępne)
- `energy_metrics.csv` (dla session runs)

## Profile eksperymentów
Dostępne profile:
- `smoke`
- `standard`
- `extended`
- `thesis_core`
- `full_cross_platform`
- `debug`

Parametry profilu mogą nadpisywać:
- `benchmark_mode`
- `repetitions`
- `warmups`
- `real_runs`
- `trials`
- `population`
- `iterations`

## Real kernels
Rozszerzenia w V5:
- `standard` / `extended` mode w `real_kernels/run_all_real_kernels.py`
- `--warmups`
- nowy kernel `SAXPY`
- istniejące kernels włączone do V5:
  - `SpMV`
  - `GEMM`
  - `reduction`
  - `stencil2d`
  - `stencil3d`
  - `fem`
  - `fem_integration`

## Full pipeline
`run_full_thesis_pipeline.py` w V5:
- rozumie `--experiment-profile`
- rozumie `--warmups`
- zapisuje `pipeline.log`
- zapisuje `pipeline_events.jsonl`
- zapisuje kampanijny `run_manifest.json`
- zapisuje `environment_manifest.json`
- zapisuje `figures_manifest.json`

## Zweryfikowane lokalnie
1. `py_compile` dla zmodyfikowanych runnerów i nowych modułów: OK
2. `run_workflow.py --workflow cpu_benchmark --profile quick --experiment-profile smoke`: OK
3. `scripts/validate_artifacts.py` na powstałej sesji CPU: OK
4. `real_kernels/benchmarks/run_saxpy.py --backend cpu --runs 1 --warmups 1`: OK
5. `real_kernels/run_all_real_kernels.py` minimalny run CPU: OK
6. `standardize_session_artifacts(...)` na sesji z real kernels: OK
7. `scripts/validate_artifacts.py` na sesji real kernels: OK

## Znane ograniczenia
- `filip_exact_reference` nadal zależy od dostępności replay/reference inputs oraz backendu exact.
- Na macOS energia przez `powermetrics` wymaga uprawnień administratora; bez nich warstwa energii przechodzi w best-effort / unsupported.
- `run_workflow.py --workflow cpu_real_kernels` nadal poprzedza real kernels pełnym CPU benchmark suite, co jest poprawne metodologicznie, ale wydłuża smoke testy.
- Dla HIP/OpenCL pełna natywna ścieżka wszystkich real kernels nie jest jeszcze tak szeroka jak dla CPU/CUDA/Metal; pełna przenośność real kernels poza FEM integration wymaga dalszego rozszerzenia.

## Jak uruchamiać
### CPU benchmark session
```bash
python3 run_workflow.py --workflow cpu_benchmark --profile quick --experiment-profile smoke
```

### Minimalny real-kernel check
```bash
python3 real_kernels/run_all_real_kernels.py \
  --backend cpu \
  --runs 1 \
  --warmups 1 \
  --benchmark-mode standard \
  --skip-gemm --skip-reduction --skip-stencil3d --skip-fem \
  --spmv-sizes 1000 \
  --stencil-shapes 256x256 \
  --saxpy-sizes 100000 \
  --with-fem-integration \
  --fem-integration-sizes 1000 \
  --fem-integration-n-qp 2
```

### Walidacja artefaktów
```bash
python3 scripts/validate_artifacts.py --path <run_or_session_dir>
```
