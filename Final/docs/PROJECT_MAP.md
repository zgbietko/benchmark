# apple_microbench v4 - mapa projektu

Repo w wersji `v4` jest zorganizowane wokol czterech warstw eksperymentalnych:

1. `Microbenchmarks`
2. `Reference exact kernels`
3. `Correctness replay`
4. `Native application campaigns`

Taki podzial jest celowy. Mikrobenchmarki sa glowna warstwa badawcza. Exact/replay sluza do walidacji poprawnosci i do sprawdzania, czy wnioski architektoniczne z mikrobenchmarkow tlumacza zachowanie realistycznego kernela FEM.

Najwazniejsze dokumenty metodologiczne:
- `docs/V3_DOCUMENTATION_INDEX.md`
- `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
- `docs/THESIS_RESEARCH_PLAN.md`
- `docs/THESIS_NAMING_MAP.md`
- `docs/EXPERIMENTAL_PROTOCOL.md`
- `docs/THREATS_TO_VALIDITY.md`
- `docs/SYSTEM_REFERENCE.md`
- `docs/END_TO_END_TESTING_AND_WRITING.md`

## 1. CPU microbenchmarks

Katalogi:
- `cpu/lib/`
- `cpu/benchmarks/`
- `run_all_cpu_benchmarks.py`

Glowne benchmarki:
- `run_bandwidth.py`
- `run_bandwidth_mt.py`
- `run_pointer_latency.py`
- `run_tlb_latency.py`
- `run_compute_fma.py`
- `run_compute_fma_peak.py`

Wyniki trafiaja zwykle do `data/cpu/`.

## 2. GPU microbenchmarks

Katalogi:
- `gpu/metal/`
- `gpu/cuda/`
- `gpu/hip/`
- `gpu/opencl/`
- `run_all_gpu_benchmarks.py`

Backendy:
- `metal` - macOS / Apple GPU
- `cuda` - Linux / NVIDIA
- `hip` - Linux / AMD ROCm
- `opencl` - przekrojowo dla Intel/AMD/NVIDIA

Te benchmarki sa podstawa porownan architektonicznych. To tutaj mierzone sa cechy sprzetu, ktore pozniej interpretuja zachowanie kerneli FEM.

## 3. Filip reference exact

Glowne pliki:
- `run_filip_reference_exact.py`
- `Kod Filipa/mod_2022/`
- opcjonalnie `legacy/filip_exact_bundle/mod_2022/`

Ta warstwa sluzy do uruchomienia mozliwie wiernej referencji legacy OpenCL Filipa.

Klasy eksperymentow w `summary.json`:
- `reference_exact`
- `correctness_replay`
- `native_performance_campaign`

Najwazniejsze znaczenia:
- `reference_exact`
  - Linux/OpenCL
  - oryginalny tor Filipa
  - zrodlo prawdy dla wejsc i wynikow
- `correctness_replay`
  - Metal replay na zamrozonych wejsciach OpenCL
  - walidacja poprawnosci 1:1 w granicach tolerancji
- `native_performance_campaign`
  - natywna kampania wydajnosciowa projektu
  - nie jest strict 1:1 z legacy OpenCL

## 4. Replay bundles

Nowe skrypty v3:
- `scripts/export_filip_replay_inputs.py`
- `scripts/export_canonical_replay_bundles.py`

### Compact replay bundle

To maly artefakt, ktory zawiera tylko dane niezbedne do odtworzenia tego samego wejscia kernela na innym backendzie:
- `launch_meta.json`
- `execution_parameters.bin`
- `gauss_dat.bin`
- `shape_fun_ref.bin`
- `el_data_in.bin`
- opcjonalnie `el_data_out.bin`

Ten bundle jest glownym artefaktem do walidacji poprawnosci.

### Canonical replay bundles

To male, stale zestawy regresyjne, przeznaczone do szybkich testow poprawnosci i utrzymania translatora replay.

Aktualne presety:
- `smoke_test_prism_qss_opt000`
- `smoke_test_prism_sqs_opt017`
- `smoke_laplace_prism_ssq_opt062`

## 5. FEM option validation

Nowy workflow v3:
- `run_fem_option_validation.py`

Cel:
- zmierzyc w kontrolowany sposob wplyw opcji walidacyjnych na wykonanie
- zachowac multiplatformowosc
- powiazac przestrzen opcji walidacyjnych z cechami architektury

To nie jest replay legacy kernela. To jest kontrolowany zestaw prob lokalnych, dzialajacy przez wspolny `FemParametricProblem`.

Aktualne grupy prob:
- `coal_read`
- `coal_write`
- `compute_all_shape_fun_der`
- `workspace_pde_coeff`
- `workspace_geo`
- `workspace_shape_fun`
- `workspace_stiff_mat`
- `padding`

Wyniki trafiaja do `data/fem_option_validation/`.

## 6. Profiler correlation

Nowy workflow v3:
- `scripts/run_profiler_correlation.py`

Cel:
- zestawic:
  - wynik exact/native kampanii FEM
  - wynik FEM option validation
  - opcjonalne eksporty z profilerow
- zapisac jeden raport korelacyjny

Wyjscie:
- `profiler_correlation.json`
- `profiler_correlation.md`

To jest warstwa interpretacyjna, laczaca mikrobenchmarki, profilery i realistyczny kernel.

## 7. Dekodowanie `el_data_out`

Nowy modul v3:
- `analysis/filip_output_decode.py`

Cel:
- zamienic plaski `el_data_out.bin` na bardziej czytelne artefakty:
  - `stiffness_matrix__elem000.csv`
  - `rhs_vector__elem000.csv`
  - `decoded_output_summary.json`

Dzisiaj dekoder jest przygotowany glownie pod skalarne przypadki exact (`nreq = 1`). To wystarcza dla obecnych `laplace_prism` i `test_prism`.

## 8. Provenance i reproducibility

Nowy modul v3:
- `analysis/provenance.py`

Co zapisujemy:
- hash plikow
- hash summary/bundle
- runtime provenance
- informacje Git, jesli repo jest dostepne

To wzmacnia reprodukowalnosc i pozwala traktowac bundle oraz wyniki jako artefakty badawcze, a nie tylko pomocnicze pliki robocze.

## 9. Workflow orchestration i GUI

Glowne entrypointy:
- `run_workflow.py`
- `run_autotune_gui.py`

Nowe workflowy v3:
- `filip_original`
- `fem_option_validation`
- `profiler_correlation`
- `filip_autotune`
- `filip_firefly`

GUI pozwala teraz:
- uruchomic exact OpenCL
- wyeksportowac compact replay bundle
- wyeksportowac canonical replay bundles
- uruchomic Metal replay z `Replay dump root`
- uruchomic FEM option validation
- uruchomic profiler correlation
- podejrzec zapisany output i jego CSV
- zobaczyc zdekodowana macierz i RHS w summary exact runu

## 10. Jak czytac repo metodologicznie

Najwazniejsze rozroznienie:
- `microbenchmarks` odpowiadaja na pytanie: "jak zachowuje sie architektura?"
- `reference_exact` odpowiada na pytanie: "jak wyglada referencyjne wykonanie legacy kernela?"
- `correctness_replay` odpowiada na pytanie: "czy inny backend liczy to samo na tych samych danych?"
- `native_performance_campaign` odpowiada na pytanie: "jak zachowuje sie natywna implementacja projektu?"

To rozroznienie jest kluczowe dla interpretacji wynikow w doktoracie.
