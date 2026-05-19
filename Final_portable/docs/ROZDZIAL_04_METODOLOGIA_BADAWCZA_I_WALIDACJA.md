# Rozdzial 4. Metodologia badawcza, walidacja i replay poprawnosci

## 4.1. Potrzeba metodologii wielowarstwowej

Dobra metodologia wydajnosciowa powinna oddzielac od siebie trzy pytania:
1. jakie sa mozliwosci architektury,
2. jak zachowuje sie realistyczny kernel,
3. czy porownanie backendow dotyczy tego samego obliczenia.

`v3` realizuje to przez kilka warstw eksperymentu.

## 4.2. Mikrobenchmarki jako warstwa diagnostyczna

Mikrobenchmarki odpowiadaja za pomiar podstawowych cech sprzetu, takich jak:
- przepustowosc pamieci,
- opoznienie,
- throughput obliczeniowy,
- wrazliwosc na wzorce pracy.

Ich rola nie polega na symulowaniu calej aplikacji, lecz na dostarczaniu czytelnych sygnalow o mozliwosciach i ograniczeniach platformy.

## 4.3. FEM option validation jako warstwa pomostowa

Warstwa `fem_option_validation` zajmuje miejsce pomiedzy mikrobenchmarkiem a realistycznym kernelem. Jej celem jest sprawdzenie, czy wnioski z prostych testow pozostaja prawdziwe dla prob silniej przypominajacych wlasciwe obliczenie FEM.

Dzieki temu mozna zmniejszyc ryzyko zbyt dalekiego przejscia od prostych pomiarow do bardzo zlozonej aplikacji.

## 4.4. Reference exact jako zrodlo referencji

Reference exact nie jest tutaj glownym wkladem pracy, lecz punktem odniesienia. Ta warstwa dostarcza:
- referencyjnego przebiegu wykonania,
- zamrozonych danych wejsciowych dla kernela,
- referencyjnego wyniku,
- metadanych potrzebnych do odtworzenia obliczenia.

To tworzy laboratorium odniesienia, wzgledem ktorego mozna oceniac inne backendy.

## 4.5. Dlaczego sama zgodnosc wysokopoziomowego problemu nie wystarcza

Jesli dwa backendy dostana ten sam plik siatki i ten sam opis problemu, nie oznacza to jeszcze automatycznie identycznego wejscia do kernela. Po drodze powstaja bowiem szczegolowe bufory runtime, przygotowane specjalnie dla danego przebiegu obliczenia.

W zwiazku z tym rzetelne porownanie wymaga zamrozenia nie tylko opisu problemu, lecz takze rzeczywistych danych wejsciowych dla kernela.

## 4.6. Correctness replay

Correctness replay polega na uruchomieniu innego backendu na dokladnie tych samych danych wejsciowych, ktore zostaly wykorzystane w referencji. To pozwala odpowiedziec na pytanie:
- czy obliczenie zostalo zachowane,
- a nie tylko czy czas jest porownywalny.

Replay eliminuje najwazniejszy zarzut wobec porownan wieloplatformowych: ze rozne systemy mogly liczyc cos odrobine innego.

## 4.7. Tolerancja numeryczna

Porownanie wynikow nie musi oznaczac identycznosci bit po bicie. W obliczeniach zmiennoprzecinkowych dopuszcza sie male roznice wynikajace z porzadku operacji i szczegolow wykonania. Dlatego stosuje sie metryki takie jak:
- `max_abs_diff`,
- `rms_diff`,
- liczba rekordow miesciacych sie w tolerancji.

To daje uczciwe kryterium poprawnosci miedzy backendami.

## 4.8. Profiler correlation jako warstwa interpretacyjna

Profiler correlation spina dane z trzech zrodel:
- mikrobenchmarkow,
- walidacji FEM,
- profilera i realistycznego kernela.

Jej celem jest przejscie od prostego stwierdzenia „wariant A jest szybszy” do interpretacji „wariant A jest szybszy, poniewaz architektura premiuje dany wzorzec organizacji danych i pracy”.

## 4.9. Znaczenie metodologiczne

Wspolne zastosowanie tych warstw daje mocniejszy argument naukowy niz pojedynczy benchmark. Wnioski nie wynikaja wtedy z jednej liczby, lecz z uporzadkowanego ciagu obserwacji i sprawdzen:
- pomiar,
- walidacja,
- kontrola poprawnosci,
- interpretacja mechanizmu.
