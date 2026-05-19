# Indeks dokumentacji v3

Ten indeks porzadkuje dokumentacje `v3` w dwoch perspektywach:
- **technicznej**: jak uruchamiac, interpretowac i archiwizowac eksperymenty,
- **doktoratowej**: jak z tych eksperymentow zbudowac obronna narracje naukowa.

## 1. Dokumenty startowe

Jesli wracasz do projektu po przerwie, zacznij od:

1. `docs/V3_READY_REFERENCE.md`
2. `docs/PROJECT_MAP.md`
3. `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
4. `docs/END_TO_END_TESTING_AND_WRITING.md`

## 1a. Gotowe PDF-y

Najwazniejsze gotowe PDF-y to:

### `docs/v3_ready_reference.pdf`
Skondensowany przewodnik startowy:
- do uruchamiania,
- do codziennej pracy,
- do szybkiej orientacji.

### `docs/v3_teoria_od_podstaw.pdf`
Teoretyczne wprowadzenie:
- dla osoby spoza informatyki,
- do zrozumienia pojec takich jak FEM, backend, replay, profiler i mikrobenchmark,
- jako material pomocniczy do pisania wstepu i czesci metodologicznej.

### `docs/v3_documentation_bundle.pdf`
Pelny bundle dokumentacji:
- technicznej,
- metodologicznej,
- eksperymentalnej,
- pisarskiej.

### `docs/v3_rozdzialy_teoretyczne.pdf`
Zwarty pakiet rozdzialow teoretycznych:
- do czytania liniowego,
- do przerabiania na tekst rozprawy,
- do pracy nad wstepem, metodologia i zakresem wnioskow.

Budowanie PDF-ow:

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v3
if [ -f .venv/bin/activate ]; then source .venv/bin/activate; fi
python scripts/build_v3_documentation_pdf.py
```

## 2. Dokumenty metodologiczne pod rozprawe

### `docs/TEORIA_OD_PODSTAW.md`
Dokument wyjasniajacy od zera:
- czym sa obliczenia numeryczne,
- czym jest FEM,
- czym roznia sie CPU, GPU i backend,
- po co sa mikrobenchmarki,
- po co jest replay poprawnosci,
- dlaczego sam czas nie wystarcza.

### `docs/ROZDZIAL_01_MOTYWACJA_I_KONTEKST.md`
Teoretyczny rozdzial o problemie badawczym, motywacji i miejscu `v3` w logice rozprawy.

### `docs/ROZDZIAL_02_OBLICZENIA_NUMERYCZNE_I_FEM.md`
Rozdzial wyjasniajacy podstawy obliczen numerycznych i metode elementow skonczonych.

### `docs/ROZDZIAL_03_ARCHITEKTURA_I_BACKENDY.md`
Rozdzial o CPU, GPU, pamieci, backendach i organizacji wykonania kernela.

### `docs/ROZDZIAL_04_METODOLOGIA_BADAWCZA_I_WALIDACJA.md`
Rozdzial metodologiczny opisujacy mikrobenchmarki, walidacje FEM, exact i replay poprawnosci.

### `docs/ROZDZIAL_05_METRYKI_PROFILING_I_INTERPRETACJA.md`
Rozdzial o metrykach, profilerach, stabilnosci wynikow i interpretacji danych pomiarowych.

### `docs/ROZDZIAL_06_SYNTEZA_WKLADU_I_ZAKRES_WNIOSKOW.md`
Rozdzial spinajacy wklad wlasny, ograniczenia i zakres wnioskow rozprawy.

### `docs/V3_READY_REFERENCE.md`
Najkrotszy dokument roboczy:
- czym jest `v3`,
- jak uruchamiac workflowy,
- gdzie sa wyniki i wykresy,
- jakie sa dwa glowne PDF-y,
- co sprawdzic przed finalnymi kampaniami.

### `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
Glowny dokument metodologiczny:
- rola mikrobenchmarkow,
- rola `reference_exact`,
- rola `correctness_replay`,
- rola `fem_option_validation`,
- rola `profiler_correlation`.

### `docs/THESIS_NAMING_MAP.md`
Nazewnictwo do rozprawy, prezentacji i publikacji:
- jak nazywac warstwy eksperymentalne,
- czego unikac,
- jak nie mieszac Twojego wkladu z benchmarkiem referencyjnym.

### `docs/THESIS_RESEARCH_PLAN.md`
Plan naukowy rozprawy:
- glowna teza,
- pytania badawcze,
- hipotezy,
- oczekiwany wklad,
- mapowanie pytan badawczych na workflowy i artefakty.

### `docs/EXPERIMENTAL_PROTOCOL.md`
Pelny protokol eksperymentalny:
- definicja kampanii,
- liczba powtorzen,
- statystyka,
- polityka warmup / replay / profilerow,
- zasady archiwizacji,
- kryteria "paper-ready".

### `docs/THREATS_TO_VALIDITY.md`
Zagrozenia dla trafnosci i ich ograniczenia:
- construct validity,
- internal validity,
- external validity,
- conclusion validity.

## 3. Dokumenty techniczne systemu

### `docs/PROJECT_MAP.md`
Przeglad calego `v3`:
- glowne warstwy,
- workflowy,
- katalogi wynikow,
- replay bundles,
- provenance,
- GUI i entrypointy.

### `docs/SYSTEM_REFERENCE.md`
Techniczny opis tego, co jest zaimplementowane:
- glowne skrypty,
- role modulow,
- schemat przeplywu danych,
- klasy eksperymentow,
- struktura wynikow i artefaktow.

### `docs/CSV_SCHEMA.md`
Kanoniczne pola CSV i ich znaczenie.

### `docs/METRICS.md`
Definicje metryk:
- czas,
- throughput,
- GFLOP/s,
- energia,
- roofline.

### `docs/REPRODUCIBILITY.md`
Uwagi dot. odtwarzalnosci kampanii.

## 4. Dokumenty uruchomieniowe

### `docs/UBUNTU_FILIP_SETUP.md`
Linux/OpenCL:
- `reference_exact`,
- bundle export,
- replay preparation,
- `fem_option_validation`,
- `profiler_correlation`.

### `docs/FRESH_UBUNTU_BOOTSTRAP.md`
Bootstrap swiezego Ubuntu.

### `docs/FIREFLY_OPTIMIZER.md`
Dokumentacja firefly optimization.

### `docs/FILIP_TIMING_PLOTS.md`
Opis wykresow pokazujacych czas wykonania kodu Filipa oraz ustawienia autotuningu.

### `docs/END_TO_END_TESTING_AND_WRITING.md`
Praktyczna checklista:
- smoke tests,
- pelne workflowy,
- archiwizacja wynikow,
- materialy do rozprawy.

## 5. Dokumenty starsze lub pomocnicze

### `docs/MICROBENCH_DOKUMENTACJA.md`
Szerszy opis warstwy mikrobenchmarkow.

### `docs/MICROBENCH_DOKUMENTACJA.pdf`
Wersja PDF powyzszej dokumentacji.

## 6. Rekomendowana kolejnosc czytania pod doktorat

Jesli celem jest pisanie rozprawy, rekomendowana kolejnosc jest taka:

1. `docs/TEORIA_OD_PODSTAW.md`
2. `docs/V3_READY_REFERENCE.md`
3. `docs/THESIS_RESEARCH_PLAN.md`
4. `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
5. `docs/THESIS_NAMING_MAP.md`
6. `docs/EXPERIMENTAL_PROTOCOL.md`
7. `docs/THREATS_TO_VALIDITY.md`
8. `docs/END_TO_END_TESTING_AND_WRITING.md`
9. `docs/SYSTEM_REFERENCE.md`

## 7. Rekomendowana kolejnosc czytania pod implementacje

Jesli celem jest rozwijanie lub uruchamianie systemu:

1. `docs/V3_READY_REFERENCE.md`
2. `docs/PROJECT_MAP.md`
3. `docs/SYSTEM_REFERENCE.md`
4. `docs/UBUNTU_FILIP_SETUP.md`
5. `docs/END_TO_END_TESTING_AND_WRITING.md`
6. `docs/CSV_SCHEMA.md`
7. `docs/METRICS.md`
8. `docs/REPRODUCIBILITY.md`

## 8. Jednozdaniowe podsumowanie v3

Najwazniejsza narracja `v3` brzmi:

- `v3` to platforma, ktora laczy mikrobenchmarki architektury z realistycznym kernelem FEM przez warstwe prob walidacyjnych, replay poprawnosci na zamrozonych danych wejsciowych i korelacje profilerowa.
