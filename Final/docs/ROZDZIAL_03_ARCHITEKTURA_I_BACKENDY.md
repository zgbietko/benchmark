# Rozdzial 3. Architektura komputerow, backendy i zrodla roznic wydajnosciowych

## 3.1. Dlaczego ta sama aplikacja nie zachowuje sie tak samo wszedzie

Program nie wykonuje sie w prozni. Jego zachowanie zalezy od:
- architektury CPU lub GPU,
- organizacji pamieci,
- szerokosci i liczby jednostek wykonawczych,
- narzutu uruchomienia,
- warstwy backendowej,
- sterownika i runtime'u.

To oznacza, ze to samo zadanie moze miec inne wąskie gardlo na roznych platformach.

## 3.2. CPU a GPU

CPU dobrze radzi sobie z:
- zlozona logika,
- nieregularnym przeplywem sterowania,
- mniejsza liczba bardziej zroznicowanych zadan.

GPU dobrze radzi sobie z:
- ogromna liczba podobnych operacji,
- praca silnie rownolegla,
- dobrze zorganizowanym dostepem do pamieci,
- powtarzalna struktura kerneli.

Dla kernela FEM oznacza to, ze GPU moze byc bardzo efektywne, ale tylko wtedy, gdy organizacja danych i pracy jest zgodna z charakterem architektury.

## 3.3. Pamięć jako glowny wspoluczestnik wydajnosci

W wielu zastosowaniach wydajnosc nie jest ograniczona przez samo liczenie, ale przez sposob dostepu do danych. Dwa kluczowe pojecia to:
- **przepustowosc** - ile danych mozna przeniesc w jednostce czasu,
- **opoznienie** - jak dlugo trwa pojedynczy dostep.

W praktyce oznacza to, ze program moze byc wolny nie dlatego, ze liczy za malo, lecz dlatego, ze zbyt dlugo czeka na dane.

## 3.4. Backend jako warstwa wykonawcza

Backend to warstwa, ktora mapuje obliczenie na konkretne srodowisko wykonawcze. Ten sam problem matematyczny moze byc uruchamiany przez:
- backend CPU,
- backend OpenCL,
- backend Metal,
- inne backendy GPU.

Backend decyduje o tym:
- jak przygotowac dane,
- jak skompilowac kernel,
- jak uruchomic obliczenie,
- i jak odebrac rezultat.

## 3.5. Wzorce dostepu do pamieci i ich znaczenie

Roznice wydajnosci bardzo czesto wynikaja z tego, *jak* wiele watkow siega po dane. W szczegolnosci:
- uporzadkowany, wspolbieżny odczyt jest zwykle bardziej przyjazny,
- chaotyczny lub rozproszony zapis bywa drozszy,
- ponowne wykorzystanie danych moze byc korzystne, ale wymaga miejsca i dobrej organizacji.

Wlasnie z takich powodow opcje kernela typu `coal_read`, `coal_write`, `workspace_*` i `padding` nie sa detalami implementacyjnymi bez znaczenia, lecz rzeczywistymi decyzjami architektonicznymi.

## 3.6. Obliczenia vs ruch danych

Jedna z klasycznych osi interpretacji brzmi:
- czy program jest ograniczony przez moc obliczeniowa,
- czy przez ruch danych?

Jesli ograniczeniem jest pamiec, zwiekszanie mocy obliczeniowej nie pomoze. Jesli ograniczeniem jest moc obliczeniowa, sama poprawa organizacji pamieci moze nie wystarczyc.

To rozroznienie jest fundamentem modeli typu roofline oraz analizy profilerowej.

## 3.7. Dlaczego backend-dependent tuning ma sens

Skoro architektury roznie reaguja na:
- wzorce odczytu,
- wzorce zapisu,
- uklad danych,
- wielkosc grup roboczych,
- ilosc pamieci roboczej,

to naturalne jest oczekiwanie, ze najlepsza konfiguracja kernela nie bedzie uniwersalna. Jednym z celow `v3` jest zbadanie, czy i jak mozna te zaleznosci przewidywac na podstawie warstwy mikrobenchmarkowej i walidacyjnej.
