# v3 - przewodnik startowy

Ten dokument jest glownym, praktycznym przewodnikiem po `v3`.

Jesli chcesz:
- uruchamiac eksperymenty,
- sprawdzac gdzie zapisaly sie wyniki i wykresy,
- rozumiec jak lacza sie warstwy systemu,
- i miec jeden dokument startowy do codziennej pracy,

to zacznij od tego pliku.

## 1. Czym jest `v3`

`v3` to uporzadkowana platforma badawcza, w ktorej:
- **mikrobenchmarki** mierza cechy architektury,
- **FEM option validation** sprawdza wzorce obliczeniowo-pamieciowe bliskie realistycznemu kernelowi FEM,
- **reference exact** odtwarza referencyjny tor obliczen,
- **correctness replay** pozwala uruchomic te same zamrozone dane wejsciowe na innym backendzie,
- **profiler correlation** laczy wyniki mikrobenchmarkow i walidacji z zachowaniem realistycznego kernela.

Najwazniejsza narracja `v3`:

- mikrobenchmarki sa glowna warstwa badawcza,
- realistyczny kernel sluzy jako walidacja interpretacyjna,
- replay poprawnosci sluzy jako walidacja obliczen,
- profiler correlation sluzy jako warstwa wyjasniajaca.

## 2. Najwazniejsze workflowy

### `cpu_benchmark` / `gpu_benchmark`
Warstwa mikrobenchmarkow architektury.

Uzywaj jej do:
- pomiaru bandwidth,
- pomiaru latency,
- pomiaru compute throughput,
- budowy ceiling/roofline.

### `fem_option_validation`
Twoja warstwa walidacji FEM bliska realistycznemu kernelowi.

Uzywaj jej do:
- sprawdzania wrazliwosci na wzorce odczytu/zapisu,
- sprawdzania reuse/workspace/computation tradeoff,
- budowy mostu miedzy mikrobenchmarkami a realistycznym kernelem.

### `filip_original`
Warstwa referencyjna i realistyczna.

Zawiera:
- `native performance campaign`,
- `reference exact`,
- `correctness replay`.

### `profiler_correlation`
Warstwa korelacyjna laczaca:
- najlepsze konfiguracje z kampanii,
- wyniki `fem_option_validation`,
- raporty profilerow.

## 3. Jak uruchomic system

### Start GUI

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v3
if [ -f .venv/bin/activate ]; then source .venv/bin/activate; fi
python run_autotune_gui.py
```

### Z GUI najwazniejsze scenariusze

#### A. Mikrobenchmarki
Uruchamiaj przez workflowy CPU/GPU.

#### B. FEM option validation
W GUI:
- wybierz `Workflow = FEM option validation`,
- ustaw backend,
- uruchom.

Artefakty pojawia sie w:
- `data/fem_option_validation/<run>/`

Najwazniejsze pliki:
- `summary.json`
- `fem_option_validation.csv`
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`
- `fem_option_validation_probes.png`
- `fem_option_validation_matrix.png`

#### C. Reference exact / correctness replay
W GUI:
- wybierz `Workflow = Filip original`,
- dla referencji OpenCL ustaw `Filip mode = exact_reference`,
- dla replayu na Metalu ustaw `Replay dump root`.

#### D. Profiler correlation
W GUI:
- wybierz `Workflow = Profiler correlation`,
- wskaz katalog optimization,
- wskaz katalog `FEM option validation`,
- uruchom.

Artefakty pojawia sie w:
- `<optimization_run>/profiler_correlation/`

Najwazniejsze pliki:
- `summary.json`
- `profiler_correlation.json`
- `option_alignment.csv`
- `profile_proximity.csv`
- `category_summary.csv`
- `profiler_correlation.md`
- `option_alignment.png`
- `profile_proximity.png`
- `category_summary.png`

## 4. Gdzie sa wykresy

To jest wazne, bo to byl realny problem roboczy.

### Optimization / article-style runs
Wykresy zapisuja sie do:
- `data/optimization/<run>/plots/`

Najwazniejsze wykresy czasu wykonania i ustawien autotuningu:
- `article_filip_execution_time_by_option.png`
- `article_autotuning_trace_with_settings.png`
- `article_autotuning_settings_heatmap.png`
- `article_best_configuration_card.png`

Ich znaczenie opisuje:
- `docs/FILIP_TIMING_PLOTS.md`

### FEM option validation
Wykresy zapisuja sie do:
- `data/fem_option_validation/<run>/`

Pliki:
- `fem_option_validation_probes.png`
- `fem_option_validation_matrix.png`

### Profiler correlation
Wykresy zapisuja sie do:
- `data/optimization/<run>/profiler_correlation/`

Pliki:
- `option_alignment.png`
- `profile_proximity.png`
- `category_summary.png`

### Jak GUI wybiera wykres
GUI probuje najpierw:
1. uzyc zapisanych PNG z `summary.json`,
2. jesli ich nie ma, narysowac wykres dynamicznie z danych.

To oznacza:
- nowe runy po poprawce powinny miec stale artefakty PNG,
- starsze runy bez PNG nadal moga byc rysowane dynamicznie, o ile zawieraja dane.

## 5. PDF-y dokumentacji

System generuje trzy glowne PDF-y:

### `docs/v3_teoria_od_podstaw.pdf`
Teoretyczne wprowadzenie:
- dla osoby spoza informatyki,
- do spokojnego zrozumienia takich pojęć jak FEM, backend, replay, profiler i mikrobenchmark,
- jako material pomocniczy do pisania wstepu i metodologii.

### `docs/v3_ready_reference.pdf`
Skondensowany dokument startowy:
- do codziennej pracy,
- do szybkiego onboardingu,
- do trzymania obok eksperymentow.

### `docs/v3_documentation_bundle.pdf`
Pelny bundle dokumentacji:
- do archiwizacji,
- do pracy nad rozprawa,
- do przegladu calego systemu.

Budowanie:

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v3
if [ -f .venv/bin/activate ]; then source .venv/bin/activate; fi
python scripts/build_v3_documentation_pdf.py
```

## 6. Rekomendowana kolejnosc pracy

### Jesli celem jest testowanie
1. `README.md`
2. `docs/V3_READY_REFERENCE.md`
3. `docs/END_TO_END_TESTING_AND_WRITING.md`
4. `docs/EXPERIMENTAL_PROTOCOL.md`

### Jesli celem jest pisanie rozprawy
1. `docs/TEORIA_OD_PODSTAW.md`
2. `docs/THESIS_RESEARCH_PLAN.md`
3. `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
4. `docs/THESIS_NAMING_MAP.md`
5. `docs/THREATS_TO_VALIDITY.md`
6. `docs/SYSTEM_REFERENCE.md`

## 7. Co jest juz gotowe

Na poziomie platformy:
- uporzadkowane workflowy,
- rozdzielenie warstw badawczych,
- correctness replay,
- provenance i hashe,
- compact replay bundles,
- canonical bundles,
- zapisywane wykresy dla validation i correlation,
- GUI z loaderami wynikow i wykresow,
- komplet dokumentacji technicznej i metodologicznej.

## 8. Co sprawdzic przed finalnymi kampaniami

Przed seria do rozprawy zrob:
1. smoke test `fem_option_validation`,
2. smoke test `filip_original`,
3. smoke test `profiler_correlation`,
4. potwierdzenie, ze `summary.json` zawiera `plots`,
5. potwierdzenie, ze PNG-i zapisaly sie w katalogu wyniku,
6. zapis wersji systemu i srodowiska.

## 9. Jednozdaniowe podsumowanie

- `v3` jest gotowa do realnych testow i do budowania rozprawy, o ile finalne kampanie beda prowadzone juz na zamrozonym protokole eksperymentalnym.
