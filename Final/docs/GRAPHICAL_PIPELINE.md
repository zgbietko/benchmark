# Pipeline Graficzny v4

## Cel

Panel WWW porzadkuje cala kampanie `v4` w postaci uproszczonego pulpitu operacyjnego.
Ma sluzyc do dwoch rzeczy jednoczesnie:

1. uruchamiania kampanii bez pamietania dlugich komend,
2. czytania procesu eksperymentalnego w sposob wizualny: postep etapow, najwazniejsze wykresy, logi i obrazy.

To nie jest osobny backend obliczeniowy. Panel steruje tym, co juz istnieje w `v4`:

- `run_workflow.py`
- `run_full_thesis_pipeline.py`
- `analysis/generate_plots.py`
- `analysis/filip_article_plots.py`

Najwazniejsze doprecyzowanie metodologiczne:

- panel pokazuje nie tylko **kroki uruchomieniowe**,
- ale tez **etapy badawcze**.

To rozroznienie jest krytyczne, bo w przeciwnym razie latwo pomylic:

- mikrobenchmarki,
- badanie cache `L1/L2/L3`,
- kampanie aplikacyjne,
- oraz walidacje poprawnosci.

Pelny opis etapow znajduje sie tutaj:

- `docs/PIPELINE_BADAWCZY.md`

## Jak uruchomic

Najprosciej:

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
./scripts/run_graphical_pipeline.sh
```

Panel domyslnie uruchamia sie na:

- `http://127.0.0.1:8765/`

Jesli istnieje lokalne `.venv`, skrypt je aktywuje. Jesli nie istnieje, uzyje systemowego `python3`.

### Wlasny port lub host

```bash
PIPELINE_WEB_PORT=8877 ./scripts/run_graphical_pipeline.sh
```

Albo bez otwierania przegladarki:

```bash
./scripts/run_graphical_pipeline.sh --no-browser
```

## Co widac w panelu

### 1. Glowne akcje

Na gorze panelu sa tylko trzy podstawowe pakiety uruchomieniowe:

- `Uruchom benchmarki`
- `Uruchom real kernels`
- `Uruchom test Filipa`

To jest domyslny, uproszczony sposob pracy. Uzytkownik nie musi od razu myslec o wszystkich technicznych krokach pipeline.

Pod spodem zostaja dopiero ustawienia i narzedzia zaawansowane.

### 2. Ustawienia podstawowe

Widoczne domyslnie zostaly tylko najwazniejsze pola:

- `Backend`
- `Tryb benchmarkow`
- `Platform profile`
- `Dostępne urządzenie`
- `Filip case`

Backend i urzadzenie nie sa juz wolnym tekstem. Panel pobiera z backendu:

- aktualnie dostepne backendy,
- aktualnie wykryte urzadzenia,
- oraz podstawia je do list wyboru.

To zmniejsza ryzyko uruchamiania kampanii na nieistniejacej konfiguracji.

Pole `Tryb benchmarkow` ma dwa znaczenia:

- `standard` oznacza wspolna sciezke porownawcza miedzy platformami,
- `extended` oznacza bogatszy tryb diagnostyczny, w ktorym benchmarki CPU i GPU
  dostaja architektur-specyficzne profile testowe dla Apple, NVIDIA, AMD i Intel.

To rozroznienie jest wazne metodologicznie: wyniki `extended` maja sluzyc przede
wszystkim do interpretacji zachowania danej architektury, a nie do zastapienia
wspolnego, porownawczego rdzenia eksperymentu.

Pole `Replay dump root` zostalo przeniesione do sekcji rozszerzonej, bo jest potrzebne glownie do walidacji `exact/replay` na macOS.

### 3. Narzedzia zaawansowane

W narzedziach zaawansowanych zostaly:

- `Uruchom pełną kampanię`
- `Zbuduj wykresy zbiorcze`
- `Odśwież wykresy Filipa`
- `Zbuduj ZIP wszystkich wykresów`
- wczytywanie i otwieranie katalogow kampanii

To sa rzeczy potrzebne do pelnego przebiegu rozprawowego, ale nie musza byc na pierwszym planie przy codziennej pracy.

Przycisk ZIP buduje jedna paczke zawierajaca:

- globalny zestaw `thesis_core` z `analysis/figures/thesis_core`,
- zestaw `thesis_core` Filipa z wybranej kampanii albo z najnowszego runu `filip_original`,
- appendix Filipa, jesli istnieje,
- `summary.json`, `campaign.md` i `steps.json` kampanii, jesli sa dostepne,
- manifesty figur i `manifest.json` opisujacy zawartosc paczki.

### 4. Zwijanie sekcji

Kazda glowna sekcja widoku moze byc teraz zwijana:

- `Postęp etapów badawczych`
- `Benchmarki platformy`
- `Real kernels i roofline`
- `Wykresy Filipa: warianty`
- `Wykresy Filipa: strojenie`
- `Techniczne kroki, logi i ręczne uruchamianie`

Na gorze panelu sa tez dwa przyciski:

- `Rozwiń sekcje`
- `Zwiń sekcje`

Stan sekcji jest zapamietywany lokalnie w przegladarce, wiec po odswiezeniu panel zachowuje ostatni uklad.

### 5. Etapy badawcze

Nad samymi bloczkami panel pokazuje tez uporzadkowana mape etapow badawczych:

1. `Charakterystyka platformy`
2. `Jadra uproszczone i real kernels`
3. `Most interpretacyjny FEM`
4. `Kampania aplikacyjna: kod Filipa`
5. `Walidacja poprawnosci obliczen`
6. `Synteza i interpretacja`

To jest poziom, na ktorym warto myslec o rozprawie.

W szczegolnosci:

- `L1/L2/L3/TLB/page-walk` nalezy do etapu 1,
- a nie do osobnego rownorzednego etapu.

### 6. Postep etapow

Glowny widok panelu nie pokazuje juz bloczkow pipeline. Zostaly one usuniete celowo, bo w praktyce wprowadzaly wiecej szumu niz porzadku.

Zamiast tego dla kazdego etapu badawczego widzisz:

- status,
- pasek postepu,
- liczbe zakonczonych krokow,
- liste krokow nalezacych do etapu.

### 7. Panel szczegolow technicznych

Po kliknieciu bloczka widzisz:

- opis kroku,
- status i czas,
- katalog wyniku,
- sciezke logu,
- ostatnie linie logu,
- powiazane obrazy i wykresy.

Szczegoly techniczne, takie jak surowy `payload JSON`, logi i reczne uruchamianie pojedynczego kroku, zostaly schowane pod rozwijanym panelem:

- `Techniczne szczegóły kroku`

To bylo celowe uporzadkowanie. Na pierwszym planie ma byc analiza procesu, a nie szum techniczny.

### 8. Podglad wykresow

Panel pokazuje od razu cztery najwazniejsze sekcje wykresow:

- `Benchmarki platformy`
- `Real kernels i roofline`
- `Wykresy Filipa: warianty QSS / SQS / SSQ`
- `Wykresy Filipa: strojenie i najlepsze konfiguracje`

Dolna galeria pokazuje globalne wykresy `thesis_core`, np.:

- `cpu_memcpy_bandwidth_scaling.png`
- `cpu_stream_triad_scaling.png`
- `cpu_peak_compute_scaling.png`
- `cpu_memory_latency_hierarchy.png`
- `gpu_microbenchmark_suite.png`
- `platform_roofline_measured.png`
- `real_kernels_model_validation.png`
- `real_kernels_filip_contrast_map.png`

Miniatury wykresow mozna kliknac. Otwiera sie wtedy duzy podglad w nakladce, razem z przyciskiem:

- `Otwórz plik`

Kazdy zapisany wykres ma teraz tez dopisek w prawym gornym rogu:

- `Platforma testowa: ...`

Ten podpis jest nanoszony bezposrednio na obraz, zeby po wyeksportowaniu do PNG albo ZIP nie bylo watpliwosci, z jakiej platformy pochodzi dany wynik.

## Co mozna uruchamiac z panelu

### Pakiet 1. Benchmarki platformy

Przycisk:

- `Uruchom benchmarki`

uruchamia pakiet:

- `cpu_benchmark`
- `gpu_benchmark`

### Pakiet 2. Real kernels

Przycisk:

- `Uruchom real kernels`

uruchamia pakiet:

- `cpu_real_kernels`
- `gpu_real_kernels`

### Pakiet 3. Test Filipa

Przycisk:

- `Uruchom test Filipa`

uruchamia pakiet:

- `fem_option_validation`
- `filip_original_portable`
- `filip_autotune`
- `filip_firefly`

Najwazniejsze wykresy z tego pakietu sa prezentowane osobno dla:

- `QSS`
- `SQS`
- `SSQ`

z pelnym zakresem kombinacji opcji.

### Uruchom pelna kampanie

Przycisk:

- `Uruchom pełną kampanię`

wywoluje:

```text
run_workflow.py --workflow full_thesis_pipeline
```

To jest glowna sciezka do zebrania danych.

### Uruchom pojedynczy krok

W panelu szczegolow kliknietego bloczka jest przycisk:

- `Uruchom ten krok`

To jest dobre do:

- poprawiania tylko jednego etapu,
- ponownego uruchomienia kroku po zmianie konfiguracji,
- pracy diagnostycznej.

### Otwieranie wynikow i logow

Przyciski:

- `Otwórz wynik`
- `Otwórz log`
- `Otwórz katalog kampanii`

korzystaja z systemowego otwierania sciezek:

- `open` na macOS,
- `xdg-open` na Linuxie.

### Odswiezanie pakietow wykresow

Dodatkowe przyciski:

- `Zbuduj wykresy zbiorcze`
- `Odśwież wykresy Filipa`

nie uruchamiaja calej kampanii od zera. One tylko przebudowuja obrazy na podstawie juz istniejacych wynikow.

## Skad panel bierze dane

Panel preferuje ostatnia **pelna kampanie zakonczona `summary.json`**, a nie samo wskazanie `latest`, jesli `latest` prowadzi do kampanii niedokonczonej.

To jest celowe. Chroni przed sytuacja, w ktorej:

- nowy katalog kampanii juz istnieje,
- ale `summary.json` jeszcze nie zostal zapisany,
- i interfejs pokazalby pusty lub mylacy stan.

## Gdzie sa logi panelu

Uruchomienia zlecane z panelu zapisują logi tutaj:

- `data/web_pipeline/`

Przykladowo:

- `20260505_132540__session_plots.log`
- `20260505_133000__filip_autotune.log`
- `20260505_133120__full_pipeline.log`

To sa logi samego uruchomienia z panelu, niezalezne od logow wewnatrz katalogow kampanii.

## Szybki raport zdrowia

Jesli chcesz szybko sprawdzic, czy masz:

- kompletna kampanie,
- summary,
- steps,
- campaign.md,
- wykresy zbiorcze,
- oraz podstawowe wykresy kodu Filipa,

uzyj:

```bash
cd /Users/mateusznytko/Desktop/Praca/Doktorat/Testy/Testy_grudzien/apple_microbench_variant2_streamfix/v4
python3 scripts/check_v4_health.py
```

Raport zapisuje sie do:

- `data/health/v4_health_report.md`

## Ograniczenia

### 1. Panel nie zastępuje `summary.json`

Prawda eksperymentalna nadal siedzi w wynikach zapisanych przez workflowy. Panel jest warstwa sterujaca i wizualna.

### 2. Exact/replay na macOS wymaga danych replay

Jesli nie uzupelnisz `Replay dump root`, krok exact/replay nie ruszy i panel zgłosi to jawnie.

### 3. Pelna kampania trwa dlugo

To nie jest smoke test. Na pelnych parametrach kampania moze trwac bardzo dlugo i generowac duzo danych.

## Minimalny scenariusz pracy

1. Uruchom panel.
2. Kliknij `Uruchom pełną kampanię`.
3. Poczekaj na zakonczenie.
4. Wczytaj najnowsza kampanie.
5. Klikaj kolejne bloczki i czytaj logi oraz obrazy.
6. Jesli potrzebujesz obrazow do rozprawy, kliknij:
   - `Zbuduj wykresy zbiorcze`
   - `Odśwież wykresy Filipa`

## Powiazane pliki

- `web/pipeline_server.py`
- `web/static/index.html`
- `web/static/style.css`
- `web/static/app.js`
- `scripts/run_graphical_pipeline.sh`
