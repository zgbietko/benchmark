# Host acceptance checklist

Ten dokument sluzy do odbioru nowego hosta przed uruchamianiem kampanii doktoratowych.
Nie zastapi pelnej walidacji naukowej, ale pozwala szybko stwierdzic, czy dany komputer
jest gotowy do:
- szybkich smoke testow,
- kampanii porownawczych `standard`,
- kampanii diagnostycznych `extended`,
- workflow `exact_reference`,
- pracy z `Final_portable`.

## Status koncowy hosta

Po przejsciu checklisty oznacz host jednym z trzech statusow:
- `ACCEPT`
  - host jest gotowy do kampanii badawczych na zadanym backendzie,
  - smoke testy przechodza,
  - kontrakty artefaktow sa poprawne,
  - generowanie wykresow dziala.
- `CONDITIONAL`
  - rdzen pipeline dziala,
  - ale sa ograniczenia, np. brak energii, brak `exact_reference`, brak jednego backendu GPU.
- `REJECT`
  - host nie przechodzi preflightu albo smoke testow,
  - backend krytyczny dla planowanych badan nie jest dostepny.

## 1. Dane hosta do zapisania

Przed testami zapisz:
- nazwe hosta,
- system i wersje kernela,
- model CPU,
- model GPU,
- ilosc RAM,
- backend planowany do kampanii,
- czy host jest zasilany z AC,
- czy planujesz `standard`, `extended`, `full_thesis_pipeline`,
- czy potrzebujesz `exact_reference`,
- czy testujesz wersje `Final` czy `Final_portable`.

Artefakty do zachowania:
- `reports/platform_matrix_audit.md`
- `reports/platform_matrix_audit.json`
- albo w portable:
  - `portable/host_compat.md`
  - `portable/host_compat.json`

## 2. Preflight ogolny

### 2.1. Final

Uruchom:

```bash
cd /sciezka/do/Final
python3 scripts/platform_matrix_audit.py \
  --md-out reports/platform_matrix_audit.md \
  --json-out reports/platform_matrix_audit.json
python3 run_device_discovery.py --backends auto
```

Warunek `PASS`:
- raport tworzy sie bez bledu,
- `run_device_discovery.py` pokazuje przynajmniej `cpu`,
- jesli planujesz GPU, widoczny jest tez backend docelowy.

### 2.2. Final_portable

Uruchom:

```bash
cd /sciezka/do/Final_portable
python3 scripts/portable_compat_report.py \
  --md-out portable/host_compat.md \
  --json-out portable/host_compat.json
python3 run_device_discovery.py --backends auto
```

Warunek `PASS`:
- raport tworzy sie bez bledu,
- launcher portable ma sens tylko na Linuxie,
- backend docelowy jest widoczny w discovery.

## 3. Wymagania backendowe

### 3.1. Apple Silicon

Warunek `PASS`:
- `metal` jest `available`,
- `gpu_benchmark` i `ai_accel` rozwiazuja backend do `metal`,
- desktop GUI startuje,
- `exact_reference_metal_port` jest oznaczony jako dostepny.

Uwagi:
- energia przez `powermetrics` bywa tylko `conditional`,
- to nie blokuje kampanii wydajnosciowej, ale blokuje twarde wnioski energetyczne.

### 3.2. NVIDIA

Warunek `PASS`:
- `cuda` jest dostepne w raporcie hosta,
- `nvcc` i runtime CUDA sa poprawnie zainstalowane,
- `run_device_discovery.py` pokazuje backend `cuda`,
- smoke `gpu_benchmark` przechodzi na `--backend cuda`.

Warunek `CONDITIONAL`:
- tylko `opencl` dziala, a `cuda` nie.

### 3.3. AMD

Warunek `PASS`:
- `hip` jest dostepne albo przynajmniej sprawne `opencl`,
- `hipcc` i runtime ROCm sa obecne, jesli planujesz HIP,
- smoke `gpu_benchmark` przechodzi na `--backend hip` lub `--backend opencl`.

Warunek `CONDITIONAL`:
- tylko `opencl` dziala, a `hip` nie.

### 3.4. Intel Arc / Intel iGPU

Warunek `PASS`:
- `opencl` jest dostepne,
- `pyopencl` dziala,
- smoke `gpu_benchmark` lub `ai_accel` przechodzi na `--backend opencl`.

## 4. Smoke testy obowiazkowe

Uruchom minimalnie:

```bash
python3 run_workflow.py --workflow cpu_benchmark --profile quick --backend auto --benchmark-mode standard --repeats 1 --real-runs 1
python3 run_workflow.py --workflow gpu_benchmark --profile quick --backend auto --benchmark-mode standard --repeats 1 --real-runs 1
python3 run_workflow.py --workflow ai_accel --profile quick --backend auto --benchmark-mode standard --repeats 1 --real-runs 1
```

Jesli nie planujesz GPU na danym hoscie:
- `cpu_benchmark` jest nadal obowiazkowy,
- `gpu_benchmark` moze zostac oznaczony jako `not applicable`.

Warunek `PASS`:
- kazdy wymagany workflow konczy sie `exit_code=0`,
- powstaje `run_manifest.json`,
- powstaje `contracts/`,
- `scripts/validate_artifacts.py --path <run_dir>` zwraca `ok=true`.

## 5. Smoke testy FEM / Filip

Minimalnie:

```bash
python3 run_workflow.py --workflow fem_option_validation --profile quick --backend auto
python3 run_workflow.py --workflow filip_original --profile quick --backend auto --filip-case portable
```

Warunek `PASS`:
- workflowy koncza sie bez bledu,
- zapisuja artefakty kampanii,
- nie ma cichego `skipped` w krytycznym kroku bez zrozumialej przyczyny.

Warunek `CONDITIONAL`:
- `filip_original` dziala tylko na CPU fallback,
- albo dziala bez `exact_reference`, ale to jest zgodne z planem kampanii.

## 6. Exact reference

### Apple

Uruchom:

```bash
python3 run_workflow.py --workflow filip_original --profile quick --backend auto --filip-mode exact_reference
```

Warunek `PASS`:
- workflow idzie przez `exact_reference_metal_port`,
- nie wymaga dodatkowych assetow replay.

### Linux / OpenCL

Wymagane:
- `pyopencl`,
- runtime OpenCL,
- `icx` / oneAPI,
- MKL,
- zdrowe `mod_2022`.

Uruchom:

```bash
python3 run_workflow.py \
  --workflow filip_original \
  --profile quick \
  --backend opencl \
  --filip-mode exact_reference \
  --filip-modfem-dir /sciezka/do/mod_2022
```

Warunek `PASS`:
- workflow konczy sie bez bledu,
- host nie raportuje brakow toolchainu,
- uzyte `mod_2022` jest czytelne i kompletne.

Warunek `CONDITIONAL`:
- rdzen projektu dziala,
- ale exact jest niedostepny i badania exact trzeba przeniesc na inny host.

## 7. Wykresy i publikacyjne artefakty

Po smoke testach uruchom:

```bash
python3 analysis/generate_plots.py
```

Jesli masz kampanie Filipa:

```bash
latest_run="$(find data/optimization -maxdepth 1 -type d -name '*__filip_original__backend-*' | sort | tail -n 1)"
python3 analysis/filip_article_plots.py --optimization-dir "$latest_run"
```

Warunek `PASS`:
- wykresy generuja sie bez bledu,
- powstaje aktualny `analysis/figures/thesis_core`,
- etykieta platformy jest obecna na figurach,
- brak pustych lub uszkodzonych PNG.

## 8. Desktop GUI i WWW

### Desktop GUI

Uruchom:

```bash
bash ./LAUNCH_DESKTOP_GUI.sh
```

Warunek `PASS`:
- okno startuje,
- widoczne sa trzy glowne pakiety,
- zakladki wykresow laduja sie bez crasha.

### WWW

Uruchom:

```bash
bash ./scripts/run_graphical_pipeline.sh
```

Warunek `PASS`:
- panel otwiera sie lokalnie,
- backend i urzadzenia sa wykrywane,
- kampanie i wykresy da sie odswiezyc.

## 9. Energia i moc

To jest warstwa `best-effort`, wiec oceniaj ja osobno.

Warunek `PASS`:
- raport hosta pokazuje sensowne zrodlo energii dla planowanej platformy,
- dane nie sa puste, jesli energia jest elementem eksperymentu.

Warunek `CONDITIONAL`:
- benchmarki wydajnosciowe dzialaja,
- ale energia jest `unsupported` albo `conditional`.

Na Apple:
- bez uprawnien administratora `powermetrics` moze dawac `0` lub `nan`.

## 10. Decyzja koncowa

Host oznacz jako `ACCEPT`, jesli:
- przeszedl preflight,
- przeszedl smoke CPU,
- przeszedl wymagany smoke GPU,
- przeszedl `ai_accel`, jesli to czesc kampanii,
- przeszedl walidacje artefaktow,
- generuje wykresy,
- ma jasny status exact i energii.

Host oznacz jako `CONDITIONAL`, jesli:
- dziala rdzen CPU/GPU,
- ale exact albo energia sa ograniczone,
- albo brakuje jednego backendu, ktory nie jest krytyczny dla tej kampanii.

Host oznacz jako `REJECT`, jesli:
- nie przechodzi `run_device_discovery.py`,
- wymagany backend GPU nie jest dostepny,
- smoke workflowy koncza sie bledem,
- `validate_artifacts.py` zwraca problemy dla obowiazkowych runow.

## 11. Minimalny raport po odbiorze hosta

Po zakonczeniu checklisty zapisz:
- status hosta: `ACCEPT` / `CONDITIONAL` / `REJECT`,
- backend docelowy,
- data odbioru,
- sciezki do raportow:
  - `reports/platform_matrix_audit.md`
  - albo `portable/host_compat.md`
- sciezki do smoke runow,
- lista ograniczen.

To powinno byc dolaczane do dokumentacji kampanii badawczej.
