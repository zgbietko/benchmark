# Pipeline badawczy v4

## Po co jest ten dokument

Ten dokument porzadkuje `v4` na poziomie metodologicznym.
Jego celem nie jest pokazanie wszystkich skryptow, tylko odpowiedz na proste pytanie:

- **jakie sa etapy badawcze i po co kazdy z nich istnieje?**

To jest szczegolnie wazne, bo latwo pomylic:

- etapy badawcze,
- konkretne workflowy uruchomieniowe,
- oraz techniczne szczegoly implementacji.

## Najkrotsza odpowiedz

Nie, poprawny podzial nie wyglada tak:

1. `microbenchmark`
2. `real kernels`
3. `kod Filipa`
4. `badanie opoznien pamieci L1/L2/L3`

Taki zapis miesza poziomy opisu.

### Dlaczego?

Bo:

- `L1/L2/L3/TLB/page-walk` to **nie jest osobny, rownorzedny etap**,
- tylko **czesc mikrobenchmarkow**, czyli czesci opisujacej surowe wlasnosci platformy.

## Wlasciwy podzial pipeline'u

W `v4` uporzadkowany pipeline badawczy wyglada tak:

1. **Charakterystyka platformy**
2. **Jadra uproszczone i real kernels**
3. **Most interpretacyjny FEM**
4. **Kampania aplikacyjna: kod Filipa**
5. **Walidacja poprawnosci obliczen**
6. **Synteza i interpretacja**

To jest podzial, ktory ma sens zarowno:

- w narzedziu,
- w analizie wynikow,
- jak i w rozprawie.

---

## Etap 1. Charakterystyka platformy

To jest pierwszy i najbardziej podstawowy etap.
Jego zadaniem nie jest jeszcze liczenie pelnego problemu aplikacyjnego, tylko opisanie:

- jak zachowuje sie procesor,
- jak zachowuje sie GPU,
- gdzie sa granice przepustowosci,
- gdzie pojawiaja sie opoznienia,
- i jaka jest roznica miedzy obliczeniami a ruchem danych.

### Co wchodzi do tego etapu

- `cpu_benchmark`
- `gpu_benchmark`

### Co nalezy do tego etapu merytorycznie

- przepustowosc pamieci,
- pointer latency,
- roofline,
- wydajnosc obliczeniowa,
- analiza cache i pamieci,
- analiza `L1`, `L2`, `L3`,
- analiza `TLB/page-walk`.

### Najwazniejszy wniosek porzadkujacy

**Badanie opoznien pamieci `L1/L2/L3` nie jest osobnym etapem.**

To jest **podzbior etapu 1**, czyli czesc mikrobenchmarkow.

### Co dostajemy na wyjsciu

- benchmarkowe CSV,
- wykresy platformy,
- wykresy pamieci i cache,
- roofline CPU,
- roofline GPU,
- pierwsza, surowa charakterystyke platformy.

---

## Etap 2. Jadra uproszczone i real kernels

To jest etap przejsciowy miedzy prymitywem a aplikacja.

Mikrobenchmarki odpowiadaja na pytanie:

- **jak dziala sprzet sam w sobie?**

Ale nie odpowiadaja jeszcze wprost na pytanie:

- **jak te wlasnosci zachowaja sie na czyms bardziej realistycznym?**

Wlasnie po to jest etap 2.

### Co wchodzi do tego etapu

- `cpu_real_kernels`
- `gpu_real_kernels`

### Rola tego etapu

Ten etap sprawdza, czy obserwacje z etapu 1:

- utrzymuja sie na bardziej praktycznych jadrach,
- nadal maja znaczenie po przejsciu z prostego testu do bardziej realistycznego obciazenia,
- i czy widzimy podobne ograniczenia: pamiec, compute, lokalnosc, roofline.

### Co dostajemy na wyjsciu

- wykresy `real kernels`,
- metryki czasu i przepustowosci,
- lacznik miedzy mikrobenchmarkiem a obciazeniem praktycznym.

---

## Etap 3. Most interpretacyjny FEM

To jest etap, ktory bardzo latwo przeoczyc, a on jest kluczowy metodologicznie.

Mikrobenchmarki i `real kernels` nadal nie sa pelnym problemem aplikacyjnym.
Z kolei pelny kod Filipa jest juz obciazeniem zbyt zlozonym, zeby bezposrednio tlumaczyc go tylko na podstawie jednego wykresu przepustowosci.

Dlatego potrzebny jest etap posredni.

### Co wchodzi do tego etapu

- `fem_option_validation`

### Rola tego etapu

Ten etap:

- wprowadza jezyk problemu FEM,
- testuje opcje `qss`, `sqs`, `ssq`,
- laczy obserwacje architektoniczne z zachowaniem obliczen zblizonych do rzeczywistego problemu MES/FEM,
- pozwala przejsc od pytania:
  - `jak zachowuje sie sprzet?`
do pytania:
  - `jak zachowuje sie obliczenie numeryczne na tym sprzecie?`

### Co dostajemy na wyjsciu

- katalog walidacji FEM,
- zestaw probe'ow i podsumowan,
- wykresy walidacyjne,
- material interpretacyjny do porownywania z kodem Filipa.

---

## Etap 4. Kampania aplikacyjna: kod Filipa

To jest glowny etap aplikacyjny.
Tutaj pracujesz juz na rzeczywistym kodzie, rzeczywistych kombinacjach konfiguracji i rzeczywistym krajobrazie strojenia.

### Co wchodzi do tego etapu

- `filip_original_portable`
- `filip_autotune`
- `filip_firefly`

### Jak rozumiec te trzy kroki

#### 4A. Pelny sweep

`filip_original_portable`

To jest kampania pelna, czyli:

- wszystkie kombinacje,
- wszystkie warianty,
- wszystkie wyniki czasowe dla danej platformy.

To jest punkt odniesienia.

#### 4B. Autotuning

`filip_autotune`

To nie jest nowy etap naukowy rowny pelnemu sweepowi.
To jest **metoda przeszukiwania przestrzeni ustawien**.

Jej rola:

- znalezc dobre konfiguracje szybciej niz pelny sweep,
- porownac jakosc znalezionych wynikow z punktem odniesienia.

#### 4C. Firefly

`filip_firefly`

To rowniez nie jest osobny poziom metodologiczny, tylko:

- alternatywna metoda przeszukiwania,
- do porownania z autotuningiem losowym i z pelnym sweepem.

### Co dostajemy na wyjsciu

- czasy wykonania kodu Filipa,
- wykresy dla `qss`, `sqs`, `ssq`,
- wykresy wszystkich kombinacji,
- wykresy trajektorii autotuningu,
- wykresy Firefly,
- najlepsze konfiguracje dla platformy.

---

## Etap 5. Walidacja poprawnosci obliczen

To jest etap, ktory odpowiada na inne pytanie niz reszta.

Wczesniejsze etapy odpowiadaja glownie na pytania:

- `jak szybko?`
- `jakie sa ograniczenia?`
- `ktora konfiguracja jest lepsza?`

Etap 5 odpowiada na pytanie:

- **czy porownujemy to samo obliczenie?**

### Co wchodzi do tego etapu

- `filip_exact_reference`

### Rola tego etapu

To jest warstwa:

- `exact reference`,
- `frozen inputs`,
- `replay`,
- `expected output`,
- porownania wyniku miedzy backendami.

To oznacza, ze ten etap sluzy glownie do:

- walidacji poprawnosci,
- a nie do podstawowego budowania rankingu wydajnosci.

### Co dostajemy na wyjsciu

- replay bundle,
- dumpy wejsc i wyjsc,
- walidacje roznic numerycznych,
- potwierdzenie, ze backendy licza to samo albo bardzo blisko tego samego.

---

## Etap 6. Synteza i interpretacja

To jest etap koncowy.

Tutaj nie chodzi juz o uruchomienie kolejnego benchmarku, tylko o polaczenie wszystkich wnioskow:

- mikrobenchmarkow,
- cache i pamieci,
- real kernels,
- walidacji FEM,
- kodu Filipa,
- exact/replay,
- oraz profilera.

### Co wchodzi do tego etapu

- `profiler_correlation`

### Rola tego etapu

Ten etap odpowiada na pytanie:

- **czy obserwacje z wczesniejszych etapow rzeczywiscie tlumacza zachowanie realnego kodu?**

To jest etap interpretacyjny, najbardziej zblizony do finalnych wnioskow do rozprawy.

### Co dostajemy na wyjsciu

- raport korelacji,
- wykresy i tabele zgodnosci,
- podstawe do dyskusji wynikow w rozprawie.

---

## Jak to czytac jako pipeline

Najprosciej:

1. najpierw opisujesz platforme,
2. potem sprawdzasz jadra uproszczone,
3. potem przechodzisz do mostu FEM,
4. potem analizujesz pelny kod aplikacyjny,
5. potem weryfikujesz poprawnosc obliczen,
6. na koncu skladasz interpretacje.

Mozna to zapisac rowniez tak:

```text
Charakterystyka platformy
-> Real kernels
-> Most FEM
-> Kod Filipa
-> Walidacja poprawnosci
-> Synteza
```

## Najwazniejsze uporzadkowanie pojęciowe

### Co jest etapem badawczym

- charakterystyka platformy,
- real kernels,
- most FEM,
- kod Filipa,
- walidacja poprawnosci,
- synteza.

### Co jest tylko podetapem lub technika

- `L1/L2/L3/TLB` to czesc mikrobenchmarkow,
- `autotune` to metoda przeszukiwania w ramach kampanii aplikacyjnej,
- `Firefly` to rowniez metoda przeszukiwania, a nie osobny poziom badawczy,
- `replay` to technika walidacji poprawnosci.

## Wniosek koncowy

Jesli chcesz to opisywac jasno, to Twoj pipeline powinien byc rozumiany tak:

1. **Mikrobenchmarki i hierarchia pamieci**
2. **Real kernels**
3. **Walidacja opcji FEM**
4. **Kod Filipa: kampania aplikacyjna i strojenie**
5. **Walidacja poprawnosci: exact/replay**
6. **Profiler correlation i synteza**

To jest porzadek, ktory usuwa skroty myslowe i dobrze nadaje sie do:

- panelu graficznego,
- planu eksperymentow,
- rozdzialu metodologicznego,
- i finalnej narracji rozprawy.
