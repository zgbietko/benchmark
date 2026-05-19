# Referencja systemowa v3

Ten dokument jest technicznym opisem tego, co jest zaimplementowane w `v3`.

## 1. Glowna idea systemu

`v3` nie jest pojedynczym benchmarkiem, tylko platforma eksperymentalna zlozona z warstw:

1. `microbenchmarks`
2. `fem_option_validation`
3. `reference_exact`
4. `correctness_replay`
5. `native_performance_campaign`
6. `profiler_correlation`

Kazda warstwa ma inna role:
- mikrobenchmarki opisuja architekture,
- `fem_option_validation` buduje warstwe interpretacyjna blisko realistycznego kernela,
- `reference_exact` daje zrodlo frozen inputs,
- `correctness_replay` sprawdza rownowaznosc wyniku,
- `native_performance_campaign` bada natywne backendy projektu,
- `profiler_correlation` scala warstwy interpretacyjne.

## 2. Glowne entrypointy

### GUI
- `run_autotune_gui.py`

### Orkiestracja workflowow
- `run_workflow.py`

### Kampanie mikrobenchmarkowe
- `run_all_cpu_benchmarks.py`
- `run_all_gpu_benchmarks.py`
- `run_all_benchmarks.py`
- `run_all_backends.py`

### Kampanie realistyczne / FEM
- `run_filip_original.py`
- `run_filip_reference_exact.py`
- `run_fem_option_validation.py`
- `scripts/run_profiler_correlation.py`

## 3. Klasy eksperymentow

W `summary.json` i w narracji `v3` wystepuja trzy podstawowe klasy w obszarze FEM:

### `reference_exact`
- uruchomienie legacy OpenCL exact
- Linux / OpenCL
- zrodlo frozen inputs i expected outputs

### `correctness_replay`
- replay na frozen inputs
- np. Metal replay na wejsciach OpenCL
- walidacja outputu

### `native_performance_campaign`
- natywna kampania wydajnosciowa projektu
- nie jest strict replayem 1:1

Dodatkowo:

### `fem_option_validation`
- kontrolowane probe'y walidacyjne

### `profiler_correlation`
- raport laczacy probe'y, wynik FEM i profiler

## 4. Glowna struktura katalogow

### Kod
- `cpu/`
- `gpu/`
- `real_kernels/`
- `analysis/`
- `scripts/`
- `optimization/`
- `Kod Filipa/`
- `legacy/`

### Wyniki
- `data/cpu/`
- `data/gpu/`
- `data/optimization/`
- `data/fem_option_validation/`
- `data/runs/`
- `reports/`

## 5. Warstwa `fem_option_validation`

### Entry point
- `run_fem_option_validation.py`

### Cel
- zbadac lokalny wplyw kontrol odpowiadajacych:
  - `coal_read`
  - `coal_write`
  - `compute_all_shape_fun_der`
  - `workspace_*`
  - `padding`

### Architektura
- oparta na `FemParametricProblem`
- wykonywana przez wspolny cross-platform problem layer
- nie jest strict replayem legacy exact

### Artefakty
- `fem_option_validation.csv`
- `records.jsonl`
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`
- `summary.json`

### Najwazniejsze pola summary
- `workflow`
- `experiment_class`
- `backend`
- `execution_mode`
- `device`
- `records`
- `probe_catalog`
- `probe_summary`
- `category_summary`
- `provenance`
- `summary_hash`

## 6. Warstwa `profiler_correlation`

### Entry point
- `scripts/run_profiler_correlation.py`

### Cel
- powiazac:
  - wynik exact/native,
  - wynik `fem_option_validation`,
  - opcjonalne raporty profilerow.

### Artefakty
- `profiler_correlation.json`
- `summary.json`
- `profiler_correlation.md`
- `option_alignment.csv`
- `profile_proximity.csv`
- `category_summary.csv`

### Najwazniejsze pola summary
- `workflow`
- `optimization_dir`
- `fem_option_validation_dir`
- `best_overall`
- `option_alignment`
- `profile_proximity`
- `category_summary`
- `profiler_reports`
- `provenance`
- `summary_hash`

## 7. Warstwa exact / replay

### Entry point
- `run_filip_reference_exact.py`

### Role
- `reference_exact`
- `correctness_replay`
- `native_performance_campaign`

### Exact OpenCL
- uruchamiany na Linuxie
- uzywa legacy `mod_2022`
- potrafi budowac exact kernel path
- zapisuje launch dumps i wyniki numeryczne

### Replay
- wykorzystuje frozen inputs z exact
- potrafi odpalic Metal replay
- waliduje output, jesli jest expected output

### Dodatkowe artefakty v3
- compact replay bundle
- canonical replay bundles
- decoded matrix / rhs outputs
- provenance i hashe

## 8. Replay bundles

### Compact replay bundle

Zawiera:
- `launch_meta.json`
- `execution_parameters.bin`
- `gauss_dat.bin`
- `shape_fun_ref.bin`
- `el_data_in.bin`
- opcjonalnie `el_data_out.bin`

### Canonical replay bundles

Male zestawy regresyjne do:
- szybkich testow,
- walidacji replayu,
- utrzymania translatora.

## 9. Dekoder outputu

### Modul
- `analysis/filip_output_decode.py`

### Cel
- przetworzyc `el_data_out.bin` do:
  - `stiffness_matrix__elem000.csv`
  - `rhs_vector__elem000.csv`
  - `decoded_output_summary.json`

### Stan
- przygotowany glownie dla przypadkow skalarowych exact (`nreq = 1`)

## 10. Provenance

### Modul
- `analysis/provenance.py`

### Co robi
- zbiera informacje runtime,
- zapisuje srodowisko,
- liczy hashe plikow,
- zapisuje `summary_hash`,
- pobiera informacje Git, jesli repo jest dostepne.

### Gdzie jest uzywany
- `fem_option_validation`
- `profiler_correlation`
- exact / replay

## 11. GUI

### Glowny plik
- `run_autotune_gui.py`

### Najwazniejsze funkcje
- uruchamianie workflowow
- live output
- live utilization
- ladowanie wynikow
- probe support panel
- ladowanie exact output CSV
- ladowanie latest validation / correlation
- wykresy dla:
  - optimization
  - validation
  - correlation
  - matrix comparison

### Wazne przyciski v3
- `Load Latest FEM Validation`
- `Load Latest Correlation`
- `Load Summary`
- `Loaded Result Plot`
- `Validation Plot`
- `Correlation Plot`

## 12. Orkiestracja workflowow

### Plik
- `run_workflow.py`

### Odpowiedzialnosc
- zamiana przyjaznych workflowow na konkretne skrypty i argumenty,
- wykrywanie backendow,
- rozstrzyganie backend tokens (`intel`, `amd`, `auto`),
- spójne zwracanie `out_dir` i `summary`.

### Najwazniejsze workflowy v3
- `cpu_benchmark`
- `gpu_benchmark`
- `cpu_real_kernels`
- `gpu_real_kernels`
- `filip_original`
- `fem_option_validation`
- `profiler_correlation`
- `filip_autotune`
- `filip_firefly`

## 13. Artefakty pod rozprawe

Najwazniejsze artefakty, ktore system juz produkuje:

### Dla validation
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`

### Dla correlation
- `option_alignment.csv`
- `profile_proximity.csv`
- `profiler_correlation.md`

### Dla exact/replay
- `validation_summary`
- replay bundles
- decoded outputs

## 14. Co system juz robi dobrze

1. rozdziela correctness od performance,
2. wspiera frozen-input replay,
3. ma warstwe probe'ow walidacyjnych,
4. wspiera provenance i hash-based reproducibility,
5. daje GUI i CLI dla calego pipeline'u,
6. zachowuje multiplatformowosc.

## 15. Co nie jest jeszcze uniwersalne

Nalezy to traktowac jako ograniczenia systemu, nie jako blad dokumentacji:

- replay translator nie jest ogolnym kompilatorem OpenCL -> Metal,
- dekoder outputu jest dzis najmocniejszy dla `nreq = 1`,
- profiler correlation wymaga sensownych eksportow z zewnetrznych profilerow,
- `native_performance_campaign` nie jest strict exact replayem.

## 16. Jak korzystac z tej dokumentacji

Jesli chcesz:
- zrozumiec metodyke: czytaj `METHODOLOGY_MICROBENCH_TO_FEM.md`
- planowac doktorat: czytaj `THESIS_RESEARCH_PLAN.md`
- prowadzic eksperymenty: czytaj `EXPERIMENTAL_PROTOCOL.md`
- uruchamiac system: czytaj `END_TO_END_TESTING_AND_WRITING.md`
