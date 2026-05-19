# Podsumowanie projektu do NotebookLM

## 1. Czym jest ten projekt

Projekt `apple_microbench_variant2_streamfix` w wersji `v4` jest platforma badawcza do doktoratu. Jego celem nie jest tylko uruchamianie benchmarkow, ale zbudowanie **spojnej metodologii** laczacej:

- mikrobenchmarki architektury,
- bardziej realistyczne jadra obliczeniowe,
- obliczenia zblizone do problemu FEM/MES,
- pelna kampanie aplikacyjna oparta na kodzie Filipa,
- walidacje poprawnosci obliczen miedzy backendami,
- oraz synteze wynikow przez profilowanie i korelacje.

Najwazniejsze zalozenie metodologiczne jest takie:

- **mikrobenchmarki sa glowna warstwa wyjasniajaca**,
- kod Filipa sluzy jako **kampania aplikacyjna i punkt odniesienia dla zachowania realistycznego obciazenia**,
- exact/replay sluzy jako **warstwa walidacji poprawnosci**, a nie jako zwykly benchmark wydajnosci.

Dodatkowo w `v4` istnieja teraz dwa tryby benchmarkowe:

- `standard` - wspolna sciezka porownawcza miedzy platformami,
- `extended` - osobny tryb diagnostyczny, ktory bada bogatsze, architektur-specyficzne ustawienia dla Apple, NVIDIA, AMD i Intel.

To rozroznienie jest celowe. `Extended` nie ma zastapic rdzenia porownawczego,
tylko uzupelnic go o lepsza interpretacje limitow konkretnej architektury.

Projekt jest rozwijany glownie w:

- `/Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4`

Starsze `v2` i `v3` sa historyczne. Aktualna wersja robocza do badan i porzadkowania to `v4`.

---

## 2. Najwazniejszy porzadek pojęciowy

W projekcie bardzo latwo pomylic trzy rzeczy:

1. **etapy badawcze**
2. **workflowy uruchomieniowe**
3. **techniki pomocnicze i narzedzia**

Zeby uniknac chaosu, przyjeto nastepujacy podzial.

### 2.1. Etapy badawcze

Poprawny pipeline badawczy ma 6 etapow:

1. **Charakterystyka platformy**
2. **Jadra uproszczone i real kernels**
3. **Most interpretacyjny FEM**
4. **Kampania aplikacyjna: kod Filipa**
5. **Walidacja poprawnosci obliczen**
6. **Synteza i interpretacja**

To jest glowna os metodologiczna projektu.

### 2.2. Co nie jest osobnym etapem

Niektore elementy sa wazne, ale nie powinny byc traktowane jako osobne rownorzedne etapy:

- `L1/L2/L3/TLB/page-walk` to **czesc mikrobenchmarkow**, a nie osobny etap
- `autotune` to **metoda przeszukiwania** w ramach kampanii aplikacyjnej
- `Firefly` to rowniez **metoda przeszukiwania**, a nie osobny poziom badawczy
- `replay` to **technika walidacji poprawnosci**

To rozroznienie bylo jednym z glownych powodow porzadkowania `v4`.

---

## 3. Etap 1: Charakterystyka platformy

To jest etap opisujacy surowe zachowanie sprzetu.

### Pytania badawcze tego etapu

- jaka jest przepustowosc pamieci CPU i GPU?
- jakie sa opoznienia dostepu do danych?
- gdzie pojawiaja sie granice cache i pamieci?
- jaki jest roofline CPU i GPU?
- jak zachowuja sie prymitywy obliczeniowe i transferowe?

### Workflowy

- `cpu_benchmark`
- `gpu_benchmark`

### Co nalezy do tego etapu

- bandwidth
- pointer latency
- roofline
- compute throughput
- stream
- analiza cache `L1/L2/L3`
- analiza `TLB/page-walk`

### Najwazniejsza decyzja metodologiczna

Badanie opoznien pamieci `L1/L2/L3` zostalo jawnie wlaczone do etapu mikrobenchmarkow. Wczesniej istnialo ryzyko traktowania tego jako osobnego, rownorzednego etapu, co mieszalo poziomy opisu.

### Artefakty

- `data/runs/<session>/cpu`
- `data/runs/<session>/gpu`
- `analysis/figures/thesis_core/*.png`
- `analysis/figures/manifests/thesis_core_manifest.json`

Przyklady najwazniejszych wykresow `thesis_core`:

- `cpu_memcpy_bandwidth_scaling.png`
- `cpu_stream_triad_scaling.png`
- `cpu_peak_compute_scaling.png`
- `cpu_memory_latency_hierarchy.png`
- `gpu_microbenchmark_suite.png`
- `platform_roofline_measured.png`

---

## 4. Etap 2: Jadra uproszczone i real kernels

To jest etap przejsciowy miedzy prymitywami architektonicznymi a pelna aplikacja.

### Pytania badawcze tego etapu

- czy obserwacje z mikrobenchmarkow utrzymuja sie na bardziej realistycznych jadrach?
- czy platforma nadal pokazuje te same ograniczenia przy obciazeniach blizszych praktyce?

### Workflowy

- `cpu_real_kernels`
- `gpu_real_kernels`

### Artefakty

- `data/runs/<session>/real_kernels`
- `analysis/figures/thesis_core/real_kernels_model_validation.png`
- `analysis/figures/thesis_core/real_kernels_filip_contrast_map.png`

---

## 5. Etap 3: Most interpretacyjny FEM

To jest warstwa posrednia miedzy charakterystyka platformy a pelnym kodem aplikacyjnym.

### Rola

Ten etap odpowiada na pytanie:

- jak przejsc od obserwacji sprzetowych do jezyka obliczen FEM?

### Workflow

- `fem_option_validation`

### Co robi

- bada warianty `qss`, `sqs`, `ssq`
- bada operatorow typu `laplace` i `test`
- pozwala porownac wzorce obliczeniowe i pamieciowe w kontekscie problemu FEM

### Artefakty

- `data/fem_option_validation/...`
- `probe_summary.csv`
- `category_summary.csv`
- wykresy walidacyjne

---

## 6. Etap 4: Kampania aplikacyjna: kod Filipa

To jest glowny etap aplikacyjny projektu.

### Workflowy

- `filip_original_portable`
- `filip_autotune`
- `filip_firefly`

### Znaczenie workflowow

#### `filip_original_portable`

To jest pelny sweep wszystkich kombinacji. Daje kompletny krajobraz konfiguracji i czasow dla danej platformy.

#### `filip_autotune`

To jest losowe przeszukiwanie przestrzeni ustawien. Jego celem jest znalezienie dobrych konfiguracji szybciej niz pelny sweep.

#### `filip_firefly`

To jest metaheurystyka Firefly sluzaca do alternatywnej eksploracji przestrzeni ustawien. Ma sens jako metoda optymalizacyjna, ale nie jest osobnym poziomem metodologicznym. Sluzy do porownania sposobow strojenia.

### Artefakty

- `data/optimization/<run>/`
- `summary.json`
- `figures/thesis_core/*.png`
- `figures/appendix/*.png`

### Wazne wykresy

Projekt generuje teraz osobne wykresy dla:

- wszystkich opcji `qss`
- wszystkich opcji `sqs`
- wszystkich opcji `ssq`

Najwazniejsze pliki:

- `filip_variant_qss.png`
- `filip_variant_sqs.png`
- `filip_variant_ssq.png`
- `filip_autotuning_trace.png`
- `filip_best_summary.png`
- `filip_memory_compute_breakdown.png`
- `filip_best_configuration_card.png`

To zostalo zrobione specjalnie po to, aby analiza platformy byla maksymalnie czytelna i zeby nie mieszac wszystkich wariantow na jednym wykresie bez potrzeby.

---

## 7. Etap 5: Walidacja poprawnosci obliczen

To jest warstwa odpowiedzialna za sprawdzenie, czy rozne backendy licza to samo.

### Workflow

- `filip_exact_reference`

### Idea

Etap ten opiera sie na:

- `exact reference`
- `frozen inputs`
- `replay`
- `expected output`

Jego rola nie polega glownie na mierzeniu wydajnosci, ale na potwierdzeniu:

- czy backendy porownujemy na tych samych danych wejściowych,
- czy daja ten sam lub bardzo zblizony wynik,
- czy roznice czasowe nie wynikaja z liczenia czegos innego.

### Platformowa logika uruchamiania

#### macOS

- exact/replay wymaga `Replay dump root`
- bez niego krok jest oznaczany jako `skipped`

#### Linux / OpenCL

- exact reference probuje uruchomic OpenCL
- automatycznie wlacza eksport dumpow i replay bundle

### Aktualny stan stabilnej kampanii

W ostatniej stabilnej kampanii `v4` na macOS:

- `filip_exact_reference` ma status `skipped`

To nie oznacza bledu systemu. To znaczy, ze dla tej kampanii nie podano `Replay dump root`, wiec exact/replay zostal pominięty zgodnie z logika platformy.

---

## 8. Etap 6: Synteza i interpretacja

To jest etap koncowy, na ktorym wszystkie pozostale warstwy sa skladane w jedna interpretacje.

### Workflow

- `profiler_correlation`

### Rola

Ten etap laczy:

- mikrobenchmarki
- hierarchie pamieci
- real kernels
- walidacje FEM
- kod Filipa
- oraz dane profilera

To tutaj powstaje warstwa interpretacyjna najbardziej bezposrednio zwiazana z wnioskami do rozprawy.

---

## 9. Full pipeline v4

W `v4` istnieje nadrzedny workflow:

- `full_thesis_pipeline`

To nie jest szybki test, tylko pelna kampania badawcza.

### Co uruchamia

1. `cpu_benchmark`
2. `gpu_benchmark`
3. `cpu_real_kernels`
4. `gpu_real_kernels`
5. `fem_option_validation`
6. `filip_original` w trybie `portable_sweep`
7. `filip_autotune`
8. `filip_firefly`
9. `filip_original` w trybie `exact_reference` lub replay, jesli platforma na to pozwala
10. `profiler_correlation`

### Co zapisuje

- `summary.json`
- `steps.json`
- `campaign.md`
- `logs/`
- `plots/`
- `artifacts/`

### Lokalizacja

- `data/thesis_full/<timestamp>__full_thesis__profile-full__backend-...`

### Aktualna ostatnia stabilna pelna kampania

- `20260505_125102__full_thesis__profile-full__backend-metal`

Jej status:

- `critical_success = true`
- `exit_code = 0`

Kroki:

- `cpu_benchmark` -> `ok`
- `gpu_benchmark` -> `ok`
- `cpu_real_kernels` -> `ok`
- `gpu_real_kernels` -> `ok`
- `fem_option_validation` -> `ok`
- `filip_original_portable` -> `ok`
- `filip_autotune` -> `ok`
- `filip_firefly` -> `ok`
- `filip_exact_reference` -> `skipped`
- `profiler_correlation` -> `ok`

---

## 10. Panel graficzny WWW

W `v4` istnieje lokalny panel WWW do sterowania calym pipeline.

### Pliki

- `web/pipeline_server.py`
- `web/static/index.html`
- `web/static/style.css`
- `web/static/app.js`
- `scripts/run_graphical_pipeline.sh`

### Co zostalo uporzadkowane

Panel zostal przebudowany tak, aby nie mieszal:

- etapow badawczych
- workflowow technicznych
- oraz szczegolow wykonania

### Obecny widok panelu

Panel pokazuje:

- 6 etapow badawczych
- 10 krokow pipeline
- uproszczone bloczki
- status kroku
- postęp kroku
- zgrupowanie krokow po etapach
- szczegoly wybranego kroku w panelu bocznym

### Wazna zmiana

Z bloczkow usunieto pseudo-wejscia i pseudo-wyjscia, bo nie zwiekszaly czytelnosci. Zostawiono to, co pomaga najbardziej:

- nazwe kroku
- status
- czas
- etap badawczy
- postep

### Duzy podglad wykresow

Klikniecie miniatury wykresu otwiera duzy podglad.

### Auto-port fallback

Jesli domyslny port jest zajety, serwer sam wybiera kolejny wolny port.

---

## 11. Zapis postepu kampanii

Jedna z wazniejszych zmian porzadkujacych byla taka:

- pelna kampania zapisuje teraz `summary.json` i `steps.json` **w trakcie dzialania**

Wczesniej podsumowanie bylo pelne dopiero na koncu, co utrudnialo sensowne pokazywanie zywego postepu w panelu.

Obecnie:

- panel widzi krok `running`
- pokazuje biezacy postep kampanii
- nie jest skazany na „pusty stan” az do zakonczenia calego przebiegu

---

## 12. Logika generowania wykresow

To byl jeden z glownych obszarow porzadkowania.

### Problem historyczny

Generator mogl mieszac dane z roznych sesji, jesli po prostu bral „najnowsze niepuste katalogi”.

### Obecna logika

`analysis/generate_plots.py` preferuje:

- **ostatnia stabilna kampanie zakonczona**

a nie:

- niedokonczony najnowszy katalog

To bardzo wazne metodologicznie, bo ogranicza ryzyko budowania wykresow na niepelnych lub mieszanych danych.

### Potwierdzone dzialajace wykresy zbiorcze

- `cpu_memcpy_bandwidth_scaling.png`
- `cpu_stream_triad_scaling.png`
- `cpu_peak_compute_scaling.png`
- `cpu_memory_latency_hierarchy.png`
- `gpu_microbenchmark_suite.png`
- `platform_roofline_measured.png`
- `real_kernels_model_validation.png`
- `real_kernels_filip_contrast_map.png`

### Potwierdzone wykresy Filipa

- `filip_variant_qss.png`
- `filip_variant_sqs.png`
- `filip_variant_ssq.png`
- `filip_autotuning_trace.png`
- `filip_best_summary.png`
- `filip_memory_compute_breakdown.png`

---

## 13. Raport zdrowia systemu

Dodano prosty skrypt diagnostyczny:

- `scripts/check_v4_health.py`

### Cel

Ma szybko odpowiedziec na pytanie:

- czy system jest kompletny?
- czy sa summary i steps?
- czy sa wykresy zbiorcze?
- czy sa kluczowe wykresy Filipa?
- jaka jest ostatnia stabilna kampania?

### Wynik

Raport zapisuje sie do:

- `data/health/v4_health_report.md`

To jest szybkie narzedzie porzadkujace, zeby nie przekopywac recznie katalogow.

---

## 14. Co zostalo zweryfikowane

Zweryfikowano mechanicznie:

- skladnie Pythona dla kluczowych skryptow
- skladnie JavaScriptu panelu
- dzialanie API panelu
- zwracanie 6 etapow badawczych i 10 krokow
- zywy zapis postepu kampanii
- generowanie wykresow zbiorczych
- generowanie wykresow Filipa
- raport zdrowia systemu

### Potwierdzony aktualny stan

Na dzień `2026-05-06`:

- dokumentacja kluczowa jest obecna
- ostatnia stabilna kampania `full_thesis_pipeline` jest kompletna i ma `critical_success = true`
- wykresy zbiorcze sa wygenerowane
- wykresy `qss/sqs/ssq` dla kodu Filipa sa wygenerowane

---

## 15. Co jest obecnie najwazniejsze do zrozumienia

Najwazniejszy porzadek pojęciowy projektu jest taki:

1. **Mikrobenchmarki i hierarchia pamięci**
2. **Real kernels**
3. **Walidacja opcji FEM**
4. **Kod Filipa: kampania aplikacyjna i strojenie**
5. **Walidacja poprawności: exact/replay**
6. **Profiler correlation i synteza**

To jest w tej chwili najwazniejsza rama interpretacyjna calego projektu.

---

## 16. Najwazniejsze pliki i dokumenty

### Dokumenty metodologiczne

- `docs/PIPELINE_BADAWCZY.md`
- `docs/THESIS_RESEARCH_PLAN.md`
- `docs/METHODOLOGY_MICROBENCH_TO_FEM.md`
- `docs/EXPERIMENTAL_PROTOCOL.md`
- `docs/THREATS_TO_VALIDITY.md`

### Dokumenty techniczne

- `docs/GRAPHICAL_PIPELINE.md`
- `docs/V4_FULL_PIPELINE.md`
- `docs/SYSTEM_REFERENCE.md`
- `README.md`

### Główne skrypty

- `run_workflow.py`
- `run_full_thesis_pipeline.py`
- `run_autotune_gui.py`
- `analysis/generate_plots.py`
- `analysis/filip_article_plots.py`
- `scripts/check_v4_health.py`
- `scripts/run_graphical_pipeline.sh`

---

## 17. Jak uruchamiac projekt

### Pelna kampania

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
python3 run_workflow.py --workflow full_thesis_pipeline --platform-profile auto --backend auto --device-index 0
```

### Panel graficzny

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
./scripts/run_graphical_pipeline.sh
```

### Wykresy zbiorcze

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
python3 analysis/generate_plots.py
```

### Wykresy Filipa

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
latest_run="$(find data/optimization -maxdepth 1 -type d -name '*__filip_original__backend-*' | sort | tail -n 1)"
python3 analysis/filip_article_plots.py --optimization-dir "$latest_run"
```

### Raport zdrowia

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
python3 scripts/check_v4_health.py
```

---

## 18. Najkrotsze podsumowanie

Projekt `v4` to uporzadkowana platforma doktorancka, w ktorej:

- mikrobenchmarki i pamiec opisuja surowe wlasnosci platformy,
- real kernels sprawdzaja, czy te obserwacje utrzymuja sie na bardziej realistycznych jadrach,
- `fem_option_validation` buduje most do obliczen FEM,
- kod Filipa daje pelna kampanie aplikacyjna i pole do strojenia,
- exact/replay waliduje poprawnosc obliczen,
- profiler correlation spina wszystkie warstwy w interpretacje naukowa.

Najwazniejsza os porzadkujaca to:

- **platforma -> real kernels -> FEM bridge -> kod Filipa -> correctness -> synthesis**

To jest stan projektu, ktory powinien byc traktowany jako aktualna, uporzadkowana rama do dalszych analiz i do pisania rozprawy.
## Aktualne doprecyzowanie metodologiczne

- `L1 / L2 / L3 / TLB / page-walk` to czesc etapu mikrobenchmarkow, nie osobny etap.
- `standard` to wspolna sciezka porownawcza miedzy platformami.
- `extended` to bogatsza diagnostyka architektury, z profilami specyficznymi dla Apple / NVIDIA / AMD / Intel.
- panel WWW ma trzy glowne pakiety:
  - `Benchmarki platformy`
  - `Real kernels`
  - `Test Filipa`
- kazdy z tych pakietow ma osobny limit CPU (`rdzenie CPU`) ustawiany w panelu.

## Filip i algorytm przeciwny

Na obecnych danych `Filip` nie wyglada jak czysto `memory-bound`.
Szacowany udzial dla kampanii `filip_original` na backendzie `metal` wskazuje raczej na przebieg bardziej `compute-leaning`:
- srednio ok. `56-61%` czesci compute,
- srednio ok. `24-27%` czesci read / transfer,
- srednio ok. `15-17%` czesci write.

Dlatego jako algorytm przeciwny, eksponowany w warstwie `real_kernels`, traktowany jest:
- `SpMV`

Rola `SpMV`:
- niski poziom arithmetic intensity,
- wyraznie silniejsza zaleznosc od pamieci i lokalnosci,
- kontrast interpretacyjny wobec `Filip-like FEM`.

Do tego dochodza dwa nowe wykresy:
- `real_kernels_filip_contrast.png`
- `real_kernels_filip_contrast_map.png`
