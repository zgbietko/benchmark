# Rozdzial 5. Metryki, profiler i interpretacja wynikow

## 5.1. Czas jako metryka podstawowa, ale niewystarczajaca

Czas wykonania jest najbardziej intuicyjna metryka. Pokazuje, ile trwa wykonanie zadania. Nie odpowiada jednak na pytania:
- co bylo ograniczeniem,
- czy wynik jest poprawny,
- czy roznice maja charakter stabilny,
- czy szybszy backend osiagnal przewage dzieki lepszej architekturze czy dzieki zmianie semantyki obliczenia.

Dlatego czas powinien byc traktowany jako punkt wyjscia, a nie jedyne kryterium interpretacji.

## 5.2. Throughput i metryki pochodne

W analizach wydajnosciowych stosuje sie rowniez metryki typu:
- GFLOP/s,
- GB/s,
- ns na jednostke pracy,
- energia na jednostke pracy,
- energy-delay product.

Kazda z nich patrzy na problem z innej strony:
- GFLOP/s pyta o wydajnosc obliczeniowa,
- GB/s pyta o intensywnosc ruchu danych,
- ns na jednostke pracy upraszcza porownanie kosztu lokalnego,
- energia pokazuje koszt fizyczny uzyskanego wyniku.

## 5.3. Stabilnosc wynikow

W badaniach nie wystarczy pokazac jednego najlepszego przebiegu. Konieczne jest uwzglednienie:
- powtorzen,
- rozrzutu,
- wspolczynnika zmiennosci,
- czasem przedzialow ufnosci.

Dopiero wtedy da sie odroznic realny efekt od szumu pomiarowego.

## 5.4. Rola profilera

Profiler jest narzedziem diagnostycznym. Pokazuje, gdzie program spedza czas i jakie sa najbardziej prawdopodobne zrodla strat wydajnosci. W kontekscie tej pracy profiler moze wspierac odpowiedzi na pytania:
- czy kernel jest memory-bound czy compute-bound,
- czy problemem jest odczyt, zapis lub wykorzystanie grup roboczych,
- czy zachowanie realistycznego kernela zgadza sie z przewidywaniem wynikajacym z mikrobenchmarkow.

## 5.5. Od korelacji do interpretacji

Sama korelacja nie jest jeszcze dowodem przyczynowym. Jest jednak bardzo cenna, gdy jest osadzona w szerszej metodologii.

Jesli jednoczesnie:
- mikrobenchmarki wskazuja ograniczenie pamieci,
- walidacja FEM pokazuje wrazliwosc na uporzadkowany odczyt,
- profiler wskazuje istotne oczekiwanie na pamiec,
- a replay potwierdza zgodnosc obliczen,

to interpretacja zyskuje duzo silniejsze podstawy.

## 5.6. Ograniczenia interpretacyjne

Nalezy przy tym zachowac ostroznosc. Z samych metryk nie wolno wyciagac zbyt daleko idacych wnioskow. W szczegolnosci:
- podobny czas nie dowodzi poprawnosci,
- przewaga jednej metryki nie opisuje calego zachowania systemu,
- jeden zestaw danych nie gwarantuje pelnej ogolnosci,
- a silna korelacja nadal powinna byc wspierana logika metodologiczna.

## 5.7. Znaczenie dla rozprawy

Rozdzial metryczny i profilerowy jest potrzebny po to, aby przejsc od wynikow liczbowych do sensownej interpretacji. To wlasnie w nim spina sie:
- warstwa pomiaru,
- warstwa walidacji,
- i warstwa wyjasnienia mechanizmu.

Dzieki temu rozprawa nie ogranicza sie do listy tabel i wykresow, lecz pokazuje, dlaczego wyniki maja taki, a nie inny ksztalt.
