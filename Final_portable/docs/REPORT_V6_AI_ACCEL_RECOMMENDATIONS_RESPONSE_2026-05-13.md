# Raport odpowiedzi na rekomendacje AI acceleration (v6)

Data: 2026-05-13
Repo: `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v6`

## 1) Co wdrożono teraz (po rekomendacjach)

### A. Rozdzielenie "AI cores" od faktycznej ścieżki wykonania

Wyniki `ai_accel` zostały rozszerzone o pola:
- `execution_device`
- `acceleration_class`
- `implementation_level`
- `vendor_ai_unit_used`
- `native_ai_path` + `native_ai_available`

To pozwala odróżnić m.in.:
- `native_vendor_gemm` (np. CUDA/CuPy path)
- `native_runtime` (np. Core ML probe)
- `portable_proxy` (mapowany HIP/OpenCL proxy)
- `cpu_fallback`

Plik:
- `ai_accel/run_ai_accel_suite.py`

### B. Walidacja numeryczna matmul

Dodano metryki poprawności (tam, gdzie walidacja jest zaimplementowana):
- `max_abs_error`
- `mean_abs_error`
- `max_rel_error`
- `mean_rel_error`
- `reference_dtype`
- `validation_status`
- `validation_reason`

Obecnie:
- CPU: walidacja vs referencja CPU (`float64` / `int32` dla int8)
- CUDA: walidacja probe vs referencja CPU
- Metal / HIP / OpenCL / CoreML probe: status `unsupported` (jawnie oznaczone)

Plik:
- `ai_accel/run_ai_accel_suite.py`

### C. Raport detekcji ścieżek native/proxy/fallback

Dodano automatyczny raport:
- `ai_accel_path_report.json`

Generator:
- `analysis/ai_accel_path_report.py`

Workflow `ai_accel` uruchamia raport automatycznie i dodaje go do payload/contracts.

Plik integracji:
- `run_workflow.py`

### D. Nowe figury doktoratowe

Generator AI wykresów produkuje teraz:
- `ai_accel_overview.png`
- `ai_accel_break_even.png`
- `ai_precision_scaling.png`

Plik:
- `analysis/generate_ai_accel_plots.py`

### E. Integracja UI (web)

Nowe figury AI dodane do galerii real-kernels:
- `ai_accel_break_even.png`
- `ai_precision_scaling.png`

Krok `ai_accel` pozostaje elementem pakietu `real_kernels`.

Pliki:
- `web/pipeline_server.py`
- `web/static/index.html`
- `web/static/app.js`

### F. Dokumentacja

Zaktualizowano:
- `README.md`
- `docs/V6_AI_ACCELERATION.md`

Dodano ten raport:
- `docs/REPORT_V6_AI_ACCEL_RECOMMENDATIONS_RESPONSE_2026-05-13.md`

## 2) Co zostało zweryfikowane

Przeszły:
- `py_compile` dla wszystkich zmodyfikowanych plików
- smoke workflow:

```bash
python3 run_workflow.py \
  --workflow ai_accel \
  --profile quick \
  --backend cpu \
  --benchmark-mode standard \
  --real-runs 1 \
  --warmups 0 \
  --ai-shapes 64x64x64 \
  --ai-dtypes float32 \
  --no-ai-coreml-ne-probe
```

Wynik smoke:
- `exit_code = 0`
- wygenerowane CSV z nowymi polami semantycznymi i walidacyjnymi
- wygenerowane 3 figury AI
- wygenerowany `ai_accel_path_report.json`

## 3) Stan względem rekomendacji (wdrożone / częściowe / pending)

### Wdrożone
- Rozdzielenie semantyki ścieżki wykonania (`implementation_level`, `execution_device`).
- Oznaczanie native/proxy/fallback.
- Raport ścieżek (`ai_accel_path_report.json`).
- Break-even figure.
- Precision-scaling figure.
- Częściowa walidacja numeryczna (CPU/CUDA).

### Częściowe
- Klasy `acceleration_class` są heurystyczne i wymagają dalszego strojenia pod pełny przekrój platform.
- Core ML / NE: oznaczenie `probable` zamiast twardego `confirmed` (celowo konserwatywne).

### Pending (kolejny krok)
- Vendor-native GEMM dla wszystkich platform (cuBLAS/rocBLAS/oneMKL/MPS/Accelerate).
- Szerszy sweep kształtów (także nieregularne profile) jako default dla `extended`.
- Rozszerzenie walidacji numerycznej o Metal i backendi mapowane (jeśli pojawi się deterministyczna ścieżka output).
- Dodatkowe workloady AI (attention, conv2d) jako osobna faza.

## 4) Lista plików zmienionych w tej iteracji

- `ai_accel/run_ai_accel_suite.py`
- `analysis/ai_accel_path_report.py` (nowy)
- `analysis/generate_ai_accel_plots.py`
- `run_workflow.py`
- `web/pipeline_server.py`
- `web/static/index.html`
- `web/static/app.js`
- `README.md`
- `docs/V6_AI_ACCELERATION.md`
- `docs/REPORT_V6_AI_ACCEL_RECOMMENDATIONS_RESPONSE_2026-05-13.md` (nowy)

