# Mapa nazewnictwa do rozprawy dla v3

Ten dokument porzadkuje nazewnictwo pod rozprawe i prezentacje.

## 1. Co jest glowna osia pracy

Glowna osia pracy sa `microbenchmarks`, a nie benchmark referencyjny sam w sobie.

Najbezpieczniejsza narracja:
- mikrobenchmarki opisuja architekture
- realistyczny kernel FEM sluzy jako test aplikacyjny
- correctness replay potwierdza, ze porownywane backendy licza to samo
- profiler correlation spina warstwe architektoniczna z warstwa aplikacyjna

## 2. Nazwy eksperymentow w repo i w rozprawie

### Repo / tooling
- `reference_exact`
- `correctness_replay`
- `native_performance_campaign`
- `fem_option_validation`
- `profiler_correlation`

### Rozprawa / tekst naukowy

Rekomendowane nazwy:
- `OpenCL exact reference run`
- `Metal correctness replay`
- `native FEM performance campaign`
- `FEM option validation probes`
- `profiler correlation report`

## 3. Czego unikac

Nie polecam nazywac warstwy walidacyjnej:
- `Filip microbench`
- `Filip option microbench`
- `microbenchmark Filipa`

To zaciera granice miedzy:
- Twoim wkladem metodologicznym
- a benchmarkiem referencyjnym, ktory juz istnial

## 4. Jak opisywac benchmark referencyjny

Bezpieczny opis:
- `legacy FEM reference kernel campaign`
- `reference exact campaign based on the original OpenCL implementation`
- `reference application-level validation kernel`

Jesli chcesz wskazac pochodzenie, ale nie oddawac nazwy calemu rozdzialowi, uzywaj raczej:
- `reference campaign derived from the original implementation`

zamiast robic z tego marke calej warstwy badawczej.

## 5. Jak opisywac Twoja warstwe walidacyjna

Rekomendowany termin:
- `FEM option validation probes`

Co ten termin komunikuje:
- to jest warstwa probe'ow
- sluzy do walidacji i interpretacji
- dotyczy opcji istotnych dla realistycznego kernela FEM
- nie rosi sobie prawa do bycia oryginalnym benchmarkiem referencyjnym

## 6. Jak opisywac replay

Rekomendowane terminy:
- `frozen-input correctness replay`
- `cross-backend correctness replay`
- `Metal replay on frozen OpenCL inputs`

To wzmacnia najwazniejsza ceche metodologiczną:
- porownanie backendow dotyczy tego samego obliczenia na tych samych danych wejściowych

## 7. Jak opisywac profiler correlation

Rekomendowane terminy:
- `microbenchmark-to-application correlation`
- `profiler-assisted correlation layer`
- `cross-layer interpretation workflow`

To dobrze ustawia role tej warstwy:
- nie jako osobnego benchmarku
- tylko jako narzedzia interpretacji i wyjasniania wynikow

## 8. Propozycja nazw rozdzialow

1. `Charakterystyka architektury z uzyciem mikrobenchmarkow`
2. `Od mikrobenchmarkow do prob walidacyjnych specyficznych dla FEM`
3. `Referencyjna kampania exact i replay poprawnosci na zamrozonych danych wejsciowych`
4. `Korelacja wsparta profilerem miedzy ograniczeniami architektury a zachowaniem kernela FEM`
5. `Natywne kampanie wydajnosciowe FEM na wielu backendach`

## 9. Propozycja nazw tabel i wykresow

### Tabele
- `Podsumowanie mikrobenchmarkow w podziale na backendy`
- `Podsumowanie prob walidacyjnych opcji FEM`
- `Najlepsze konfiguracje referencyjnego exact`
- `Podsumowanie bledow replayu poprawnosci`
- `Podsumowanie korelacji profilerowej`

### Wykresy
- `Roofline backendu wyznaczony z maksimow mikrobenchmarkow`
- `Delty walidacji opcji FEM w podziale na backendy`
- `Krajobraz opcji referencyjnego exact`
- `Rozklad bledow replayu poprawnosci`
- `Korelacja miedzy deltami prob a najlepsza konfiguracja exact`

## 10. Najsilniejsze zdanie pod doktorat

Najmocniejsza wersja narracji brzmi mniej wiecej tak:

- `Glownym wkladem jest metodologia laczaca mikrobenchmarki architektury z realistycznym kernelem FEM przez proby walidacyjne, replay poprawnosci na zamrozonych danych wejsciowych oraz korelacje profilerowe.`

To ustawia benchmark referencyjny na jego wlasciwym miejscu:
- jako referencje i walidator
- a nie jako glowny wklad sam w sobie.
