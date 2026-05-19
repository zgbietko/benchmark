# Raport wdrozenia v6: AI acceleration / AI cores

Data: 2026-05-13
Katalog roboczy: `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v6`

## Cel

Dodanie testowania sciezek akceleracji AI (AMD/Intel/NVIDIA/Apple + CPU fallback) do v6, z integracja w pipeline, kontraktach wynikow i interfejsie web.

## Co zostalo zrobione

### 1) Nowy modul benchmarkow AI

Dodane pliki:
- `ai_accel/__init__.py`
- `ai_accel/common.py`
- `ai_accel/run_ai_accel_suite.py`

Zakres:
- Matmul benchmark (`m,n,k`, `dtype`) dla backendow: `cpu`, `metal`, `cuda`, `hip`, `opencl`, tryby `auto` i `all`.
- Rozne precyzje: `float16`, `float32`, `float64`, `int8` (zalezne od backendu).
- Opcjonalny probe Core ML / Neural Engine na macOS (`coreml_mlp_predict`).
- Obsluga warmup/repeats.
- Zapis pelnych CSV z metrykami: `elapsed_s`, `gflops`, `throughput_gbps`, `ai_flop_per_byte`, `status`, `error`, `native_ai_path`, itp.

### 2) Integracja workflow `ai_accel` w orchestratorze

Zmiany:
- `run_workflow.py`

Dodano:
- Nowy workflow: `--workflow ai_accel`.
- Parametry:
  - `--ai-shapes`
  - `--ai-dtypes`
  - `--ai-include-cpu-baseline` / `--no-ai-include-cpu-baseline`
  - `--ai-coreml-ne-probe` / `--no-ai-coreml-ne-probe`
- Krok analizy po runie:
  - `analysis/ai_accel_summary.py`
  - `analysis/generate_ai_accel_plots.py`
- Kontrakty artefaktow + opcjonalny sync Google Drive jak w innych workflowach.

### 3) Naprawa krytycznego bledu w `run_workflow.py`

Problem:
- Wczesniej `run_gpu_real_kernels()` byl przypadkowo przeciety i nie domykal sie poprawnie logicznie (fragmenty finalizacji trafiły pod `run_ai_accel`).

Naprawa:
- Przywrocono kompletna finalizacje `run_gpu_real_kernels()`:
  - liczenie `exit_code`
  - zapis manifestu sesji
  - kontrakty
  - zwrot `_session_result(...)`
- Usunieto martwy/duplikowany kod po `return` w `run_ai_accel()`.

### 4) Integracja z kontraktami i klasyfikacja bottleneck

Zmiany:
- `analysis/contract_utils.py`
  - sekcja `ai_accel` dodana do standardowego bundlowania artefaktow sesji.
- `analysis/bottleneck_classifier.py`
  - heurystyki dla AI paths (`matmul`, `coreml`, `neural_engine`, `probe`) klasyfikowane jako compute-bound.

### 5) Nowe analizy i wykres AI

Dodane:
- `analysis/ai_accel_summary.py`
- `analysis/generate_ai_accel_plots.py`

Wynik:
- `analysis/figures/thesis_core/ai_accel_overview.png`
- Etykiety wykresu spolszczone (np. `Przeglad akceleracji AI`, `jednostki obliczeniowe`).
- Zachowany badge z platforma testowa (system/arch/device) przez `save_figure(..., platform_label=...)`.

### 6) Integracja w web UI (pakiet Real kernels)

Zmiany:
- `web/pipeline_server.py`
- `web/static/index.html`
- `web/static/app.js`

Zakres:
- Dodano krok pipeline: `ai_accel` (stage: real kernels).
- Dodano `ai_accel` do grupy uruchomieniowej `real_kernels`.
- Dodano obsluge komendy kroku `ai_accel` w `_step_command(...)`.
- Dodano `ai_accel_overview.png` do globalnych/real plot lists.
- Uaktualniono opisy w UI (karta Real kernels i hint galerii) o AI acceleration.

### 7) Integracja z desktop GUI

Zmiany:
- `run_autotune_gui.py`

Zakres:
- Dodano workflow `ai_accel` do listy workflowow w GUI.

### 8) Dokumentacja

Zmiany:
- `README.md`:
  - dodany workflow `ai_accel`
  - dodany `ai_accel_overview.png` do listy thesis-core
  - dodany quick-start dla `ai_accel`
  - wzmianka o AI step w pakiecie Real kernels
- Nowy dokument:
  - `docs/V6_AI_ACCELERATION.md`

## Weryfikacja wykonana lokalnie

### Kompilacja Python (syntax check)
Przeszly bez bledow:
- `run_workflow.py`
- `run_autotune_gui.py`
- `web/pipeline_server.py`
- `ai_accel/run_ai_accel_suite.py`
- `analysis/ai_accel_summary.py`
- `analysis/generate_ai_accel_plots.py`

### Smoke test workflow
Uruchomiono:
- `python3 run_workflow.py --workflow ai_accel --profile quick --backend cpu --benchmark-mode standard --real-runs 1 --warmups 0 --ai-shapes 64x64x64 --ai-dtypes float32 --no-ai-coreml-ne-probe`

Wynik:
- `exit_code: 0`
- wygenerowane CSV w `data/runs/<session>/ai_accel/`
- wygenerowany wykres `analysis/figures/thesis_core/ai_accel_overview.png`
- wygenerowane bundle kontraktowe w `data/runs/<session>/contracts/`

## Status funkcjonalny

- `v6` zawiera nowy workflow AI i dziala end-to-end dla smoke run.
- Naprawiony zostal blad logiczny `run_gpu_real_kernels`.
- Web UI i GUI widza nowy krok/workflow.
- Wykres AI jest wlaczony do finalnych figur sekcji real kernels.

## Ograniczenia / uwagi

- Probe Core ML/NE wymaga macOS + `coremltools`; bez tego zapisuje `unsupported` (workflow nadal konczy sie poprawnie).
- Dla `hip/opencl` sciezka matmul jest mapowanym proxy (estymacja portable oparta o prymitywy adaptera), nie pelnym vendor-native GEMM kernel.
- Pelny regression run calego `full_thesis_pipeline` nie byl odpalany w tym kroku (wykonano smoke + kompilacje + integracje UI).

## Lista plikow zmienionych/dodanych

Dodane:
- `ai_accel/__init__.py`
- `ai_accel/common.py`
- `ai_accel/run_ai_accel_suite.py`
- `analysis/ai_accel_summary.py`
- `analysis/generate_ai_accel_plots.py`
- `docs/V6_AI_ACCELERATION.md`
- `docs/REPORT_V6_AI_ACCEL_IMPLEMENTATION_2026-05-13.md`

Zmodyfikowane:
- `run_workflow.py`
- `analysis/contract_utils.py`
- `analysis/bottleneck_classifier.py`
- `analysis/publication_style.py`
- `run_autotune_gui.py`
- `web/pipeline_server.py`
- `web/static/index.html`
- `web/static/app.js`
- `README.md`

