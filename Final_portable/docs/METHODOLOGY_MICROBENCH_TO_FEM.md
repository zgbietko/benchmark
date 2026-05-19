# Metodologia: od mikrobenchmarkow do realistycznej walidacji FEM

Ten dokument porzadkuje metodologie v3 pod katem doktoratu.

## 1. Glowna teza metodologiczna

Podstawa pracy to mikrobenchmarki, a nie benchmark Filipa jako taki.

Rola warstw jest nastepujaca:
- mikrobenchmarki opisuja cechy architektury
- benchmark Filipa dostarcza realistycznego kernela numerycznego
- replay `OpenCL -> Metal` sprawdza, czy porownanie dotyczy tego samego obliczenia
- profiler correlation laczy przewidywania z mikrobenchmarkow z rzeczywistym zachowaniem kernela

To oznacza, ze benchmark Filipa nie jest glownym wkładem sam w sobie. Jest walidatorem aplikacyjnym i referencja obliczeniowa.

## 2. Cztery klasy eksperymentow

### A. Microbenchmarks

Pytanie badawcze:
- jakie cechy architektury ograniczaja wydajnosc?

Typowe metryki:
- bandwidth
- pointer latency
- TLB / page-walk latency
- FMA throughput
- peak compute
- sensitivity na workgroup size i wzorzec pamieci

### B. Reference exact

Pytanie badawcze:
- jak wyglada referencyjne wykonanie legacy kernela Filipa?

To jest zrodlo prawdy dla:
- frozen inputs
- expected outputs
- exact campaign metadata

### C. Correctness replay

Pytanie badawcze:
- czy inny backend liczy to samo na tych samych danych?

To jest warstwa walidacji poprawnosci. Najwazniejsze kryterium nie brzmi "czy czas jest podobny", tylko:
- czy output dla tego samego frozen input zgadza sie w granicach tolerancji

### D. Native performance campaign

Pytanie badawcze:
- jak zachowuje sie natywna implementacja projektu?

Ta warstwa jest wydajnosciowo bardzo wazna, ale nie wolno jej mylic z strict correctness replay.

## 3. Dlaczego sam mesh i plik problemu nie wystarczaja

Dla walidacji 1:1 nie wystarczy tylko:
- `mesh_prism.dmp`
- `problem_conv_diff.dat`

To sa dane wysokiego poziomu. OpenCL kernel legacy liczy juz na zbudowanych buforach runtime:
- `execution_parameters.bin`
- `gauss_dat.bin`
- `shape_fun_ref.bin`
- `el_data_in.bin`

Dopiero zamrozenie tych buforow gwarantuje, ze dwa backendy dostaja naprawde to samo wejscie do obliczenia.

## 4. Rola compact replay bundle

Compact replay bundle to glowny artefakt walidacyjny v3.

Zawiera:
- `launch_meta.json`
- `execution_parameters.bin`
- `gauss_dat.bin`
- `shape_fun_ref.bin`
- `el_data_in.bin`
- opcjonalnie `el_data_out.bin`

Wartosc metodologiczna:
- minimalizuje koszt przenoszenia danych
- zamraza rzeczywiste wejscie do kernela
- pozwala wykonac correctness replay na innym backendzie
- moze zawierac expected output do automatycznej walidacji

## 5. Rola FEM option validation

Nowa warstwa `fem_option_validation` odpowiada bezposrednio opcjom walidacyjnym istotnym dla kampanii exact, ale robi to w sposob kontrolowany i multiplatformowy.

To nie jest exact replay. To jest most interpretacyjny miedzy:
- prostymi mikrobenchmarkami architektury
- a realistyczna przestrzenia opcji kernela FEM

Przykladowe osie:
- `coal_read`
- `coal_write`
- `compute_all_shape_fun_der`
- `workspace_*`
- `padding`

Ta warstwa ma odpowiedziec na pytanie:
- czy lokalny wplyw danej opcji jest zgodny z tym, czego spodziewamy sie po danej architekturze?

Praktyczne artefakty tej warstwy:
- `fem_option_validation.csv`
- `probe_summary.csv`
- `category_summary.csv`
- `fem_option_validation.md`

## 6. Rola profiler correlation

Profiler correlation jest warstwa laczaca:
- wynik kampanii exact/native
- wynik FEM option validation
- eksporty z profilerow

To pozwala przejsc od prostego stwierdzenia:
- "opcja A byla szybsza"

Do silniejszego stwierdzenia:
- "opcja A byla szybsza, bo mikrobenchmarki wskazuja ograniczenie pamieci, a profiler pokazuje zgodny wzorzec bandwidth/stalls"

Praktyczne artefakty tej warstwy:
- `profiler_correlation.json`
- `profiler_correlation.md`
- `option_alignment.csv`
- `profile_proximity.csv`
- `category_summary.csv`

## 7. Correctness vs performance

Te dwie rzeczy musza byc rozdzielone.

### Correctness

Najsilniejsze kryterium:
- ten sam frozen input
- ten sam expected output
- `max_abs_diff <= tol`
- male `rms_diff`

### Performance

Najsilniejsze kryterium:
- porownywanie backendow dopiero po upewnieniu sie, ze licza to samo
- analiza czasu, bandwidth, FLOPS i energii dopiero na tle potwierdzonej poprawnosci

## 8. Co mozna twierdzic naukowo

Mozna uczciwie twierdzic:
- mikrobenchmarki opisuja cechy architektury
- te cechy maja wartosc wyjasniajaca dla realistycznego kernela FEM
- correctness replay pokazuje, ze porownanie backendow dotyczy tego samego obliczenia

Nie nalezy twierdzic bez dodatkowych zastrzezen:
- ze natywna kampania `native_performance_campaign` jest strict 1:1 z legacy exact
- ze podobny czas oznacza automatycznie poprawnosc obliczen

## 9. Co stanowi najmocniejszy wklad v3

Najmocniejsze elementy v3 to:
- uporzadkowana metodologia od mikrobenchmarku do realistycznego kernela
- correctness-aware replay na frozen inputs
- FEM option validation odpowiadajacy opcjom walidacyjnym istotnym dla kampanii exact
- provenance i hash-based reproducibility
- profiler correlation jako warstwa interpretacyjna

To jest silniejszy wklad niz sama reprodukcja benchmarku Filipa.

## 10. Co dalej warto dodawac

Najbardziej sensowne kolejne kroki:
- rozbudowac FEM option validation o bardziej wyspecjalizowane wzorce pamieci i reuse
- rozwinac dekoder `el_data_out` dla wiekszej liczby przypadkow i `nreq > 1`
- dodac bardziej formalne raporty statystyczne i przedzialy niepewnosci
- dopiac lepszy eksport z profilerow (VTune, Xcode GPU, Nsight, ROCm)
- przygotowac male kanoniczne bundle regresyjne jako stale artefakty testowe

To wszystko wzmacnia glowna os pracy: mikrobenchmarki jako narzedzie wyjasniajace, a nie tylko pomiarowe.

Zobacz tez:
- `docs/THESIS_NAMING_MAP.md`
- `docs/END_TO_END_TESTING_AND_WRITING.md`
