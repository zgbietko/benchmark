# V6 AI acceleration / AI cores

Modul `ai_accel` dodaje do `v6` osobny workflow do testow sciezek akceleracji AI.

## Co mierzy

- `matmul` (GFLOP/s, GB/s, AI FLOP/byte) dla backendow:
  - `cpu`
  - `metal`
  - `cuda`
  - `hip`
  - `opencl`
- Probe Apple `Core ML / Neural Engine` (gdy dostepne):
  - `CPU_ONLY`
  - `CPU_AND_GPU`
  - `CPU_AND_NE`
  - `ALL`

## Jak uruchomic

Przyklad szybki (portable):

```bash
python3 run_workflow.py \
  --workflow ai_accel \
  --profile quick \
  --backend auto \
  --benchmark-mode standard \
  --real-runs 3
```

Przyklad rozszerzony:

```bash
python3 run_workflow.py \
  --workflow ai_accel \
  --profile paper \
  --backend all \
  --benchmark-mode extended \
  --real-runs 5 \
  --warmups 2 \
  --ai-shapes "512x512x512,1024x1024x1024,2048x2048x2048" \
  --ai-dtypes "float16,float32,float64,int8"
```

## Artefakty

W sesji (`data/runs/<session>/`) powstaja:

- `ai_accel/*.csv` - surowe wyniki matmul / Core ML probe
- `contracts/*/` - bundle kontraktowe (`raw_results.csv`, `summary.csv`, `validation.json`, itd.)
- `analysis/figures/thesis_core/ai_accel_overview.png` - wykres zbiorczy
- `analysis/figures/thesis_core/ai_accel_break_even.png` - punkt przelamania CPU vs akceleratory
- `analysis/figures/thesis_core/ai_precision_scaling.png` - skalowanie po precyzjach
- `ai_accel_path_report.json` - raport sciezek native/proxy/fallback

## Integracja z web UI

W panelu WWW krok `AI acceleration / AI cores` jest dolaczony do pakietu `Uruchom real kernels`.

## Uwagi metodologiczne

- `hip/opencl` dla `matmul` dzialaja przez mapowany proxy adapter (przenosna estymacja oparta o prymitywy backendu).
- `coremltools` jest opcjonalne; brak biblioteki nie przerywa kampanii - probe zapisuje status `unsupported`.
- Na platformach bez natywnej sciezki AI workflow pozostaje porownywalny dzieki fallbackowi CPU/GPU.
- Wyniki zawieraja pola: `execution_device`, `acceleration_class`, `implementation_level`, `vendor_ai_unit_used` oraz metryki bledu numerycznego (tam, gdzie walidacja jest zaimplementowana).
