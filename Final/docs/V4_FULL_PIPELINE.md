# Pelny pipeline v4

`v4` dodaje jedna nadrzedna kampanie:
- `full_thesis_pipeline`

Jej celem nie jest szybki test, tylko uruchomienie calej potrzebnej warstwy eksperymentalnej w jednym kroku i zapisanie wszystkiego jako jednego artefaktu kampanii.

## Co uruchamia

Pelny pipeline uruchamia kolejno:
1. `cpu_benchmark`
2. `gpu_benchmark` jesli backend GPU jest dostepny
3. `cpu_real_kernels`
4. `gpu_real_kernels` jesli backend GPU jest dostepny
5. `fem_option_validation`
6. `filip_original` w trybie `portable_sweep`
7. `filip_autotune`
8. `filip_firefly`
9. `filip_original` w trybie `exact_reference` lub replay, jesli platforma na to pozwala
10. `profiler_correlation`, jesli sa dostepne katalogi wynikowe potrzebne do korelacji

## Jak to rozumiec metodologicznie

Pelny pipeline wykonuje wiecej niz cztery proste kroki typu:

- mikrobenchmarki,
- real kernels,
- kod Filipa,
- cache.

To bylby podzial nieprecyzyjny, bo:

- analiza `L1/L2/L3/TLB` jest czescia mikrobenchmarkow,
- `autotune` i `Firefly` sa metodami strojenia wewnatrz kampanii aplikacyjnej,
- `exact/replay` jest walidacja poprawnosci, a nie zwyklym benchmarkiem wydajnosci.

Wlasciwy porzadek etapow badawczych jest opisany tutaj:

- `docs/PIPELINE_BADAWCZY.md`

## Jak dziala logika platformowa

### macOS
- pipeline uzywa CPU i Metal tam, gdzie to ma sens,
- krok `exact_reference` jest wykonywany tylko wtedy, gdy podasz `Replay dump root`,
- bez `Replay dump root` krok exact jest oznaczany jako `skipped`, a kampania idzie dalej.

### Linux / OpenCL
- pipeline probuje uruchomic `exact_reference` przez OpenCL,
- automatycznie wlacza:
  - dump launch artifacts,
  - export compact replay inputs,
  - include expected output,
  - export canonical replay bundles.

## Gdzie zapisywane sa wyniki

Pelna kampania trafia do:
- `data/thesis_full/<timestamp>__full_thesis__profile-full__backend-...`

W katalogu kampanii sa:
- `summary.json`
- `steps.json`
- `campaign.md`
- `logs/`
- `plots/`
- `artifacts/`

## Najwazniejsze pliki

### `summary.json`
Zbiorcze podsumowanie kampanii:
- statusy krokow,
- czasy,
- glowne katalogi wynikowe,
- provenance,
- `summary_hash`.

### `steps.json`
Lista krokow w formie prostszej do przetwarzania.

### `campaign.md`
Czytelne podsumowanie tekstowe kampanii.

### `plots/full_thesis_overview.png`
Syntetyczny wykres kampanii:
- czas krokow,
- statusy `ok/skipped/failed`.

### `artifacts/`
Symlinki lub notatki wskazujace na najwazniejsze katalogi wynikowe poszczegolnych krokow.

## Jak uruchomic z GUI

W `Workflows` wybierz:
- `10. Full thesis pipeline`

Albo w `Launcher` template:
- `[v4] Full thesis pipeline`

## Jak uruchomic z CLI

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
if [ -f .venv/bin/activate ]; then source .venv/bin/activate; fi
python3 run_workflow.py --workflow full_thesis_pipeline --platform-profile auto --backend auto --device-index 0
```

## Co jest wymuszane przez pipeline

To jest celowo workflow pelny, a nie oszczedny.
Pipeline wymusza co najmniej:
- `profile = full`
- `real_runs >= 5`
- `repeats >= 5`
- `trials >= 256`
- `population >= 24`
- `iterations >= 40`
- `fem_option_validation_n_elements >= 16384`
- `fem_option_validation_n_qp >= 6`
- `fem_option_validation_workgroup_size >= 64`

Jesli chcesz lekkie testy, do tego sa pozostale workflowy. `v4` ma byc sciezka pelna.
