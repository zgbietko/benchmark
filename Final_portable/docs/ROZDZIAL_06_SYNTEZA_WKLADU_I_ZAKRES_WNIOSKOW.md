# Rozdzial 6. Synteza wkladu i zakres wnioskow

## 6.1. Co jest przedmiotem pracy

Przedmiotem pracy nie jest pojedynczy benchmark ani pojedyncza implementacja kernela. Przedmiotem pracy jest metodologia badawcza laczaca:
- mikrobenchmarki architektury,
- warstwe walidacji opcji FEM,
- referencyjny przebieg exact,
- replay poprawnosci na zamrozonych danych wejsciowych,
- korelacje profilerowe.

Taka konstrukcja pozwala przechodzic od prostych miar architektury do realistycznego obciazenia obliczeniowego w sposob interpretowalny i kontrolowany.

## 6.2. Co stanowi wklad wlasny

Wklad wlasny pracy mozna uporzadkowac w kilku punktach:

1. **spojna platforma eksperymentalna** laczaca mikrobenchmarki, kampanie FEM i replay poprawnosci,
2. **warstwa FEM option validation** jako pomost miedzy testami syntetycznymi a realistycznym kernelem,
3. **correctness replay** pozwalajacy porownywac backendy na tych samych danych wejciowych i tym samym oczekiwanym wyniku,
4. **korelacja wsparta profilerem**, ktora pomaga przejsc od pomiaru do interpretacji mechanizmu,
5. **uporzadkowany protokol eksperymentalny** z provenance, hashowaniem i artefaktami gotowymi do archiwizacji.

## 6.3. Czego praca nie twierdzi

Rzetelnosc rozprawy wymaga rowniez jasnego wskazania ograniczen. Praca nie twierdzi, ze:
- jeden benchmark realistyczny opisuje cala klase zastosowan FEM,
- kazdy backend ma identyczna semantyke pomiaru czasu,
- korelacja sama w sobie jest dowodem przyczynowym,
- wyniki dla jednej architektury automatycznie generalizuja sie na wszystkie inne platformy.

Zamiast tego praca pokazuje, w jakim zakresie mozna budowac wiarygodne wnioski i jakie warunki musza byc spelnione, aby porownanie bylo uczciwe.

## 6.4. Zakres wnioskow

Na podstawie metodologii `v3` mozna formuowac wnioski w trzech warstwach:

1. **architektonicznej** - jakie cechy sprzetu ujawniaja mikrobenchmarki,
2. **algorytmicznej** - ktore grupy opcji FEM sa wrazliwe na okreslone cechy sprzetu,
3. **walidacyjnej** - czy porownywane backendy licza to samo na tych samych danych.

Dopiero polaczenie tych trzech warstw daje mocniejszy argument naukowy niz sama tabela czasow.

## 6.5. Dlaczego taka konstrukcja jest silna doktoratowo

Najwieksza sila tej konstrukcji polega na tym, ze nie ogranicza sie ona do jednego rodzaju dowodu. W pracy wspolwystepuja:
- pomiar niskopoziomowy,
- eksperyment pomostowy,
- benchmark realistyczny,
- walidacja poprawnosci,
- oraz interpretacja wsparta profilerem.

To sprawia, ze rozprawa ma zarowno warstwe narzedziowa, jak i wyjasniajaca. Nie jest tylko zbiorem eksperymentow, lecz uporzadkowana metoda analizy zaleznosci miedzy architektura, organizacja kernela i poprawnoscia obliczen.

## 6.6. Jak czytac wyniki koncowe

Wyniki koncowe powinny byc interpretowane w nastepujacej kolejnosci:

1. czy backend przeszedl walidacje poprawnosci,
2. jaki jest jego profil architektoniczny z mikrobenchmarkow,
3. jak zachowuja sie proby walidacyjne FEM,
4. czy profiler wspiera te obserwacje,
5. dopiero na koncu - jaki jest czas wykonania i ranking konfiguracji.

Takie uporzadkowanie chroni przed zbyt szybkim przejsciem do prostego wniosku: "szybciej znaczy lepiej".

## 6.7. Konkluzja teoretyczna

W sensie teoretycznym `v3` uzasadnia nastepujace podejscie: 
- wydajnosc nalezy analizowac razem z poprawnoscia i z interpretacja mechanizmu,
- mikrobenchmarki zyskuja wartosc dopiero wtedy, gdy potrafia wyjasniac zachowanie realistycznego kernela,
- a porownanie backendow staje sie naukowo mocniejsze, gdy odbywa sie na zamrozonych danych wejsciowych i z kontrola zgodnosci wyniku.
