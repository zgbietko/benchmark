# Ubuntu setup for Filip exact, replay bundles, and validation

Ten dokument opisuje workflow v3 na Linuxie dla warstwy referencyjnej i walidacyjnej:

1. `OpenCL exact reference`
2. `compact replay bundle export`
3. `canonical replay bundles`
4. `FEM option validation`
5. `profiler correlation`

Dokument nie zastępuje ogolnego setupu mikrobenchmarkow. Koncentruje sie na tej czesci, ktora spina benchmark Filipa z walidacja i analiza.

## 1. Wymagania systemowe

Na Ubuntu potrzebujesz:
- dzialajacego runtime OpenCL dla urzadzenia referencyjnego
- `tcsh` lub `csh`
- `oneAPI compiler` i `MKL`, jesli exact ma budowac `mod_2022`
- `python3-tk`, jesli chcesz uzywac GUI

Dokladny bootstrap zalezy od maszyny, ale v3 zaklada, ze exact runner sam probuje przygotowac:
- `PATH` dla `icx`
- `LD_LIBRARY_PATH` dla `MKL`
- rozpakowanie brakujacych `mesh_prism.dmp.zip`

## 2. Exact reference z GUI

Uruchom GUI:

```bash
cd /path/to/v3
if [ -f .venv/bin/activate ]; then source .venv/bin/activate; fi
python run_autotune_gui.py
```

W zakladce `Workflows` ustaw:
- `Workflow`: `Filip original`
- `Backend`: `opencl` albo `intel`
- `Filip mode`: `exact_reference`
- `Filip case`: `test_prism`, `laplace_prism` albo `prism_pair`

Jesli chcesz przygotowac dane pod replay na Metalu, zaznacz:
- `Dump OpenCL launch artifacts`
- `Export compact replay inputs bundle`

Jesli chcesz od razu walidacje outputu na Macu, zaznacz dodatkowo:
- `Include OpenCL output in replay bundle`

Jesli chcesz zbudowac male zestawy regresyjne, zaznacz tez:
- `Export canonical replay bundles`

## 3. Exact reference z CLI

Pelny run exact:

```bash
python run_workflow.py \
  --workflow filip_original \
  --backend opencl \
  --filip-mode exact_reference \
  --filip-case test_prism \
  --filip-dump-launch-artifacts \
  --filip-export-replay-inputs \
  --filip-export-replay-include-expected-output \
  --filip-export-canonical-replay-bundles
```

To wygeneruje:
- wynik exact
- pelne `launch_dumps`
- compact replay bundle
- canonical replay bundles

## 4. Gdzie trafiaja wyniki

Dokladne wyniki exact trafiaja do:

- `data/optimization/<timestamp>__filip_original__backend-opencl__exact`

Najwazniejsze artefakty w tym katalogu:
- `summary.json`
- `csv/`
- `raw/`
- `numerical_outputs/`
- `launch_dumps/`
- `replay_inputs_bundle/`
- `canonical_replay_bundles/`

## 5. Co znajduje sie w `replay_inputs_bundle`

To jest lekki zestaw wejsc niezbednych do replayu na innym backendzie.

Minimalny zestaw:
- `launch_meta.json`
- `execution_parameters.bin`
- `gauss_dat.bin`
- `shape_fun_ref.bin`
- `el_data_in.bin`

Opcjonalnie:
- `el_data_out.bin`

Jesli bundle zawiera `el_data_out.bin`, replay moze zrobic automatyczna walidacje OpenCL vs Metal.

## 6. Co znajduje sie w `canonical_replay_bundles`

To male zestawy regresyjne, przeznaczone do:
- szybkich testow poprawnosci
- testow CI/manualnych po zmianach w replayu
- zmniejszenia kosztu kopiowania pelnych dumpow

Aktualne presety:
- `smoke_test_prism_qss_opt000`
- `smoke_test_prism_sqs_opt017`
- `smoke_laplace_prism_ssq_opt062`

## 7. FEM option validation z GUI

W zakladce `Workflows` ustaw:
- `Workflow`: `FEM option validation`
- `Backend`: docelowy backend
- `FEM validation ops`: np. `laplace,test`
- `Variants`: np. `qss,sqs,ssq`
- `n_elements`, `n_qp`, `workgroup`

To wygeneruje wynik w:
- `data/fem_option_validation/<timestamp>__fem_option_validation__backend-...`

Najwazniejsze pliki:
- `fem_option_validation.csv`
- `records.jsonl`
- `summary.json`

## 8. Profiler correlation z GUI

W zakladce `Workflows` ustaw:
- `Workflow`: `Profiler correlation`
- `Correlation dirs -> Optimization`: katalog exact/native runu z `summary.json`
- `FEM option validation dir`: katalog wyniku `fem_option_validation`
- `Profiler reports`: opcjonalne eksporty `.json` lub `.csv`

To utworzy raport w:
- `<optimization_dir>/profiler_correlation/`

Pliki wynikowe:
- `profiler_correlation.json`
- `profiler_correlation.md`

## 9. Interpretacja wynikow

W v3 rozrozniaj trzy klasy eksperymentow:

- `reference_exact`
  - referencyjne wykonanie legacy OpenCL
- `correctness_replay`
  - ten sam frozen input, inny backend, walidacja outputu
- `native_performance_campaign`
  - natywny benchmark wydajnosciowy projektu

Najwazniejsze ograniczenie:
- tylko `OpenCL exact -> replay bundle -> Metal replay` pozwala stawiac mocne twierdzenie, ze porownujesz to samo obliczenie
- `native` kampanie sa bardzo cenne wydajnosciowo, ale nie sa dowodem numerycznej rownowaznosci z legacy exact

## 10. Najkrotsza sciezka robocza

### Linux

1. Exact OpenCL z bundle export:

```bash
python run_workflow.py \
  --workflow filip_original \
  --backend opencl \
  --filip-mode exact_reference \
  --filip-case test_prism \
  --filip-dump-launch-artifacts \
  --filip-export-replay-inputs \
  --filip-export-replay-include-expected-output
```

2. FEM option validation:

```bash
python run_workflow.py \
  --workflow fem_option_validation \
  --backend opencl \
  --repeats 3
```

3. Skopiuj tylko `replay_inputs_bundle/` na maszyne z Metalem.

### macOS

1. W GUI ustaw:
- `Workflow`: `Filip original`
- `Backend`: `metal`
- `Filip mode`: `exact_reference`
- `Replay dump root`: wskaz na skopiowany `replay_inputs_bundle`

2. Uruchom replay.

3. Jesli bundle zawieral expected output, sprawdz w `Results` sekcje `Validation`.

## 11. Co sprawdzac w `summary.json`

Najwazniejsze pola v3:
- `experiment_class`
- `mode_label`
- `numerical_equivalence`
- `replay_input_bundle`
- `canonical_replay_bundles`
- `numerical_outputs`
- `validation_summary`
- `provenance`
- `summary_hash`

To jest minimalny zestaw, ktory warto archiwizowac do doktoratu razem z wykresami i tabelami.
