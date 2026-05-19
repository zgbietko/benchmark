# Przewodnik: od testow do pisania rozprawy

Ten dokument zbiera minimalny, praktyczny workflow v3 tak, zeby dalo sie:
- szybko robic smoke testy,
- wykonywac pelne eksperymenty,
- odkladac artefakty wprost pod rozprawe.

Dokumenty uzupelniajace:
- `docs/V3_DOCUMENTATION_INDEX.md`
- `docs/THESIS_RESEARCH_PLAN.md`
- `docs/EXPERIMENTAL_PROTOCOL.md`
- `docs/SYSTEM_REFERENCE.md`

## 1. Zasada ogolna

Glowna os pracy to:
1. `microbenchmarks`
2. `fem_option_validation`
3. `reference_exact`
4. `correctness_replay`
5. `profiler_correlation`

Kolejnosc ma znaczenie:
- najpierw opis architektury,
- potem probe'y walidacyjne FEM,
- potem referencyjny kernel,
- potem replay poprawnosci,
- na koncu korelacja i interpretacja.

## 2. Szybki smoke test

### 2.1. FEM option validation

GUI:
- Workflow: `FEM option validation`
- Backend: wybrany backend testowy
- Repeats: `1`
- n_elements: `64`
- n_qp: `2`
- workgroup_size: `32`

Oczekiwany wynik:
- katalog w `data/fem_option_validation/...`
- pliki:
  - `summary.json`
  - `fem_option_validation.csv`
  - `probe_summary.csv`
  - `category_summary.csv`
  - `fem_option_validation.md`

### 2.2. Profiler correlation

Wymagane:
- jeden katalog `optimization/exact`
- jeden katalog `fem_option_validation`

GUI:
- Workflow: `Profiler correlation`
- wskaż:
  - `Optimization dir`
  - `FEM option validation dir`

Oczekiwany wynik:
- katalog `.../profiler_correlation/`
- pliki:
  - `summary.json`
  - `profiler_correlation.json`
  - `profiler_correlation.md`
  - `option_alignment.csv`
  - `profile_proximity.csv`
  - `category_summary.csv`

## 3. Pelny workflow correctness

### 3.1. Linux OpenCL exact

GUI:
- Workflow: `Filip original`
- Backend: `opencl` albo `intel`
- Filip mode: `exact_reference`
- Filip case: `test_prism` albo `laplace_prism`
- zaznacz:
  - `Dump OpenCL launch artifacts`
  - `Export compact replay inputs bundle`
- opcjonalnie do walidacji:
  - `Include OpenCL output in replay bundle`
  - `Export canonical replay bundles`

Artefakty:
- `launch_dumps/`
- `replay_inputs_bundle/`
- opcjonalnie `canonical_replay_bundles/`

### 3.2. macOS Metal replay

GUI:
- Workflow: `Filip original`
- Backend: `metal`
- Filip mode: `exact_reference`
- `Replay dump root` ustaw na:
  - `replay_inputs_bundle`
  - albo `canonical_replay_bundles/<bundle>`

Oczekiwany wynik:
- `execution_mode = exact_reference_metal_replay`
- `validation_summary`
- zapisane outputy i ich dekodowanie

## 4. Pelny workflow interpretacyjny

### 4.1. Microbenchmarks

Uruchom:
- CPU benchmark
- GPU benchmark
- ewentualnie real kernels

Zachowaj:
- roofline
- session summaries
- generated plots

### 4.2. FEM option validation

Uruchom probe'y na tym samym backendzie, na ktorym chcesz interpretowac realny kernel.

Zachowaj:
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`

### 4.3. Exact/native run

Uruchom:
- `Filip original`

Zachowaj:
- `summary.json`
- `combined_csv`
- decoded output, jesli dotyczy
- replay bundle, jesli robisz walidacje poprawnosci

### 4.4. Profiler correlation

Uruchom:
- `Profiler correlation`

Zachowaj:
- `profiler_correlation.md`
- `option_alignment.csv`
- `profile_proximity.csv`

## 5. Minimalny zestaw artefaktow do rozprawy

Na jeden backend / jedna kampanie warto archiwizowac:
- `summary.json` dla kazdego glownego workflowu
- `probe_summary.csv`
- `category_summary.csv`
- `profiler_correlation.md`
- `option_alignment.csv`
- `profile_proximity.csv`
- najwazniejsze PNG zapisane z GUI
- compact replay bundle lub canonical replay bundle

## 6. Co warto wkladac do tabel i wykresow

### Tabele
- microbenchmark summary by backend
- FEM option validation probe summary
- best exact/native configuration
- correctness replay error summary
- profiler correlation summary

### Wykresy
- roofline from microbenchmark peaks
- FEM option validation delta ratios
- best exact configuration landscape
- correctness replay error distribution
- correlation alignment bar chart
- profile proximity chart

## 7. Co sprawdzac przed kazda seria pomiarow

Checklist:
- backend i device-index sa poprawne
- `summary_hash` jest zapisany
- provenance jest obecne
- output directories sa rozdzielone miedzy eksperymentami
- replay bundle zawiera to, czego potrzebujesz
- w correctness replay masz:
  - `records_checked`
  - `records_within_tolerance`
  - `records_out_of_tolerance`

## 8. Co sprawdzac przed pisaniem rozdzialu

Checklist:
- rozrozniasz `reference_exact`, `correctness_replay`, `native_performance_campaign`
- nie mieszasz natywnej kampanii z 1:1 replayem
- kazdy wykres ma odpowiadajacy mu plik z danymi wejsciowymi
- kazda tabela ma katalog z artefaktami i `summary_hash`
- wnioski wydajnosciowe sa wspierane przez:
  - mikrobenchmark
  - probe walidacyjny
  - profiler correlation

## 9. Rekomendowany porzadek katalogow do archiwizacji

Najbezpieczniej trzymac to tak:
- `artifacts/microbench/<backend>/...`
- `artifacts/fem_option_validation/<backend>/...`
- `artifacts/reference_exact/<backend>/...`
- `artifacts/correctness_replay/<backend>/...`
- `artifacts/profiler_correlation/<backend>/...`

To bardzo ulatwia skladanie wykresow i tabel bez wracania do surowych eksperymentow.
