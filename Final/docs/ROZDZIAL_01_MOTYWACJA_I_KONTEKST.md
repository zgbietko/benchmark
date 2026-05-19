# Rozdzial 1. Motywacja i kontekst badan

## 1.1. Problem ogolny

Wspolczesne obliczenia naukowe i inzynierskie sa coraz silniej uzaleznione od wydajnosci sprzetu komputerowego. Ten sam model matematyczny moze byc uruchamiany na:
- klasycznym procesorze CPU,
- procesorze graficznym GPU,
- roznych warstwach wykonawczych i backendach,
- roznych sterownikach, runtime'ach i systemach operacyjnych.

W praktyce prowadzi to do sytuacji, w ktorej:
- to samo zadanie obliczeniowe daje rozne czasy wykonania,
- rozne platformy premiuja rozne strategie organizacji kernela,
- a interpretacja tych roznic nie jest oczywista, jesli patrzy sie wylacznie na czas koncowy.

To wlasnie jest podstawowy problem badawczy rozwiazywany przez `v3`.

## 1.2. Dlaczego sama analiza czasu jest niewystarczajaca

W wielu pracach wydajnosciowych stosuje sie prosty schemat:
- uruchomic program,
- zmierzyc czas,
- porownac wyniki miedzy platformami.

Taki schemat jest uzyteczny, ale ma ograniczenia.
Sam czas nie odpowiada na pytania:
- dlaczego dana architektura jest szybsza lub wolniejsza,
- czy w obu przypadkach wykonywane jest rzeczywiscie to samo obliczenie,
- czy przewaga wydajnosci wynika z mocy obliczeniowej, organizacji pamieci czy narzutu wykonania,
- czy obserwowany wynik ma charakter stabilny, czy jest tylko przypadkowym artefaktem srodowiska.

W konsekwencji analiza oparta wylacznie na czasach bywa zbyt slaba interpretacyjnie.

## 1.3. Potrzeba warstwy wyjasniajacej

Aby porownanie wydajnosci mialo wartosc naukowa, potrzebna jest warstwa, ktora pozwala zrozumiec mechanizm stojacy za wynikiem.
W `v3` te role pelnia:
- mikrobenchmarki architektoniczne,
- warstwa `fem_option_validation`,
- profiler correlation,
- correctness replay.

Ich wspolnym celem nie jest tylko wygenerowanie kolejnych wykresow, ale zbudowanie logicznego ciagu dowodowego:
1. architektura ma okreslone ograniczenia,
2. ograniczenia te mozna zmierzyc,
3. ograniczenia te powinny wplywac na realistyczny kernel w okreslony sposob,
4. wynik obliczenia pozostaje poprawny mimo zmiany backendu,
5. zatem interpretacja wydajnosci nie jest przypadkowa.

## 1.4. Rola benchmarku realistycznego

W pracy wykorzystuje sie realistyczny kernel FEM jako warstwe walidacji aplikacyjnej. Jego rola nie polega na byciu glownym wkladem pracy, lecz na sprawdzeniu, czy obserwacje wynikajace z mikrobenchmarkow rzeczywiscie maja znaczenie dla obliczenia zblizonego do zastosowan praktycznych.

To rozroznienie jest bardzo wazne:
- mikrobenchmarki odpowiadaja za diagnoze architektury,
- realistyczny kernel odpowiada za sprawdzenie, czy diagnoza ma sens poza prostym testem syntetycznym.

## 1.5. Glowna idea metodologiczna v3

`v3` opiera sie na nastepujacym zalozeniu:
- wiarygodne badanie wydajnosci nie powinno oddzielac czasu od poprawnosci i od interpretacji.

Dlatego platforma rozdziela prace na kilka warstw:
1. **mikrobenchmarki** - opis podstawowych mozliwosci sprzetu,
2. **FEM option validation** - warstwa pomostowa bliska realistycznemu kernelowi,
3. **reference exact** - referencyjny tor wykonania,
4. **correctness replay** - walidacja, czy inne backendy licza to samo,
5. **profiler correlation** - polaczenie danych pomiarowych z rzeczywistym zachowaniem kernela.

## 1.6. Wklad metodologiczny

Wklad pracy nie polega na prostym uruchomieniu istniejacego benchmarku. Wklad polega na zbudowaniu metodologii, w ktorej:
- mikrobenchmarki nie sa odizolowane od aplikacji,
- benchmark realistyczny nie jest traktowany jako jedyny dowod,
- correctness replay wymusza porownanie tego samego obliczenia na tych samych danych,
- a profiler sluzy jako narzedzie laczace obserwacje niskopoziomowe z zachowaniem realnego kernela.

## 1.7. Znaczenie dla rozprawy

W takim ukladzie doktorat nie jest tylko raportem z wynikow wydajnosciowych. Staje sie praca o metodzie badania zaleznosci miedzy:
- architektura sprzetu,
- organizacja obliczenia,
- poprawnoscia wyniku,
- i interpretacja zachowania realistycznego kernela numerycznego.

To nadaje pracy silniejszy charakter naukowy i lepiej uzasadnia jej wartosc poznawcza.
