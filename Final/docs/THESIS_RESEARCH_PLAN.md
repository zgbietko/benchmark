# Plan badawczy rozprawy dla v3

Ten dokument porzadkuje czesc naukowa pracy doktorskiej wokol `v3`.

## 1. Glowny cel pracy

Glownym celem pracy nie jest reprodukcja benchmarku referencyjnego sama w sobie, ale:

- zbudowanie metodologii, ktora laczy **mikrobenchmarki architektury** z zachowaniem **realistycznego kernela FEM**,
- oraz sprawdzenie, czy taka metodologia daje:
  - wartosc wyjasniajaca,
  - poprawna walidacje miedzy backendami,
  - i podstawe do interpretacji wynikow wydajnosciowych.

## 2. Glowna teza

Najsilniejsza wersja tezy doktorskiej:

- `Metodologia laczaca mikrobenchmarki architektury, probe'y walidacyjne FEM, replay poprawnosci na zamrozonych danych wejsciowych oraz korelacje profilerowe pozwala wiarygodniej wyjasniac i walidowac zalezne od backendu zachowanie realistycznego kernela FEM niz sama analiza czasow wykonania.`

## 3. Pytania badawcze

### RQ1. Cechy architektury
- Ktore cechy architektury ujawnione przez mikrobenchmarki sa najistotniejsze dla zachowania kernela FEM?

### RQ2. Wartosc probe'ow walidacyjnych
- Czy `fem_option_validation` stanowi sensowny most interpretacyjny miedzy prostymi mikrobenchmarkami a realistyczna przestrzenia opcji kernela FEM?

### RQ3. Poprawnosc porownan backendow
- Czy frozen-input replay pozwala stwierdzic, ze dwa backendy licza to samo na tych samych danych?

### RQ4. Wartosc profilerow
- Czy dane z profilerow wzmacniaja interpretacje wynikow wynikajaca z mikrobenchmarkow i probe'ow walidacyjnych?

### RQ5. Uzytecznosc metodologii
- Czy cala warstwa `microbenchmarks -> validation probes -> exact/replay -> profiler correlation` daje praktycznie uzyteczny i obronny workflow badawczy dla wieloplatformowych backendow FEM?

## 4. Hipotezy badawcze

### H1. Microbenchmarks maja wartosc wyjasniajaca
- Peak bandwidth, pointer latency, TLB/page-walk latency, compute throughput i sensitivity na layout/workgroup size beda istotnie skorelowane z rankingiem konfiguracji w realistycznym kernelu FEM.

### H2. FEM option validation jest wartosciowym mostem
- Probe'y walidacyjne ukierunkowane na `coal_read`, `coal_write`, `compute_all_shape_fun_der`, `workspace_*` i `padding` beda zgodne kierunkowo z najlepszymi konfiguracjami w kampanii exact/native.

### H3. Frozen-input replay jest warunkiem uczciwego porownania
- Porownanie backendow bez frozen-input replay moze dawac mylacy obraz poprawnosci, podczas gdy replay na tych samych danych umozliwi walidacje wyjscia w granicach tolerancji.

### H4. Profiler-assisted correlation wzmacnia interpretacje
- Liczniki profilerowe beda zgodne z interpretacja wynikow wynikajaca z mikrobenchmarkow i probe'ow walidacyjnych dla dominujacych bottleneckow.

### H5. Sama analiza czasu nie wystarcza
- Same czasy wykonania nie beda wystarczajace do poprawnej interpretacji roznic miedzy backendami bez warstwy poprawnosci i interpretacji architektonicznej.

## 5. Wklad wlasny pracy

Najmocniejsze elementy wkladu:

1. **Metodologia wyjasniajaca**:
   - polaczenie mikrobenchmarkow z realistycznym kernelem FEM.
2. **FEM option validation**:
   - multiplatformowa warstwa probe'ow odpowiadajaca kluczowym kontrolom istotnym dla kernela FEM.
3. **Frozen-input correctness replay**:
   - porownywanie backendow na tych samych danych wejściowych do obliczen.
4. **Profiler-assisted correlation**:
   - warstwa spinajaca wyniki mikrobenchmarkow, probe'ow i realistycznego kernela.
5. **Reproducibility / provenance**:
   - hash-based artefakty i uporzadkowany workflow eksperymentalny.

## 6. Co nie jest glownym wkladem

To jest bardzo wazne i warto to napisac wprost.

Nie nalezy przedstawiac jako glowny wklad:
- samego benchmarku referencyjnego,
- samej reprodukcji legacy OpenCL,
- samego uruchomienia kernela na innym backendzie bez warstwy poprawnosci i interpretacji.

Benchmark referencyjny pelni role:
- aplikacyjnej referencji,
- zrodla frozen inputs,
- zewnetrznego walidatora hipotez wynikajacych z mikrobenchmarkow.

## 7. Mapa pytan badawczych na workflowy

### RQ1 / H1
Uzyj:
- `cpu_benchmark`
- `gpu_benchmark`
- roofline
- `METRICS.md`

Artefakty:
- session summaries,
- roofline outputs,
- benchmark CSV.

### RQ2 / H2
Uzyj:
- `fem_option_validation`

Artefakty:
- `fem_option_validation.csv`
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`

### RQ3 / H3
Uzyj:
- `reference_exact`
- `correctness_replay`
- compact/canonical replay bundles

Artefakty:
- `launch_dumps/`
- `replay_inputs_bundle/`
- `validation_summary`
- decoded outputs

### RQ4 / H4
Uzyj:
- `profiler_correlation`
- eksporty profilerow

Artefakty:
- `profiler_correlation.md`
- `option_alignment.csv`
- `profile_proximity.csv`
- `category_summary.csv`

### RQ5 / H5
Uzyj calego pipeline'u:
- microbenchmarks
- validation probes
- exact/replay
- profiler correlation

Artefakty:
- przekrojowe tabele i wykresy z wielu backendow.

## 8. Minimalny korpus eksperymentalny do doktoratu

Potrzebujesz co najmniej:

1. **Jednej architektury CPU**
2. **Jednej architektury Apple GPU / Metal**
3. **Jednej architektury Intel GPU / OpenCL**
4. Opcjonalnie:
   - NVIDIA / CUDA
   - AMD / HIP

Na kazdej architekturze:
- microbenchmarks,
- `fem_option_validation`,
- przynajmniej jedna realistyczna kampania FEM,
- profiler correlation,
- replay poprawnosci dla reprezentatywnych przypadkow.

## 9. Minimalne kryteria sukcesu dla tez

### Dla H1
- widoczna zgodnosc miedzy peak/sensitivity z mikrobenchmarkow a rankingiem opcji w realistycznym kernelu.

### Dla H2
- `option_alignment` i `profile_proximity` pokazuja sensowna zgodnosc probe'ow z najlepszymi konfiguracjami.

### Dla H3
- replay daje zgodnosc wynikow wyjsciowych w granicach tolerancji.

### Dla H4
- przynajmniej czesc profilerowych bottleneckow jest zgodna z interpretacja mikrobenchmarkowa.

### Dla H5
- mozna pokazac przypadki, gdzie sama analiza czasu bylaby mylaca, a warstwa poprawnosci i korelacji usuwa niejednoznacznosc.

## 10. Proponowany uklad rozdzialow

1. `Charakterystyka architektury z uzyciem mikrobenchmarkow`
2. `Proby walidacyjne opcji FEM jako warstwa interpretacyjna`
3. `Referencyjna kampania exact i replay poprawnosci na zamrozonych danych wejsciowych`
4. `Korelacja wsparta profilerem miedzy ograniczeniami architektury a zachowaniem kernela FEM`
5. `Natywne kampanie wydajnosciowe FEM na wielu backendach`
6. `Zagrozenia dla trafnosci i zakres wnioskow`

## 11. Co trzeba jeszcze dostarczyc przed finalnym pisaniem

Najwazniejsze brakujace elementy naukowe:
- finalna lista platform i backendow,
- zestaw finalnych kampanii,
- statystyka i przedzialy niepewnosci,
- opis profilerow i licznikow,
- rozdzial `threats to validity`,
- finalny zbior tabel i wykresow.

## 12. Jednozdaniowa narracja pracy

Najbezpieczniejsza finalna narracja brzmi:

- `Rdzeniem pracy jest metodologia, w ktorej mikrobenchmarki architektury sa laczone z realistycznym kernelem FEM przez probe'y walidacyjne, replay poprawnosci na zamrozonych danych oraz korelacje profilerowe.`
